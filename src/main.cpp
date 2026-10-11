#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/PartyLevelLoader.hpp"
#include "core/PartyManager.hpp"
#include "network/GlobedTransport.hpp"
#include "ui/PartyPopup.hpp"
#include "ui/RoundDecisionPopup.hpp"

#include <algorithm>
#include <memory>

using namespace geode::prelude;

$on_mod(Loaded) {
    gdparty::PartyManager::get().initialize(std::make_unique<gdparty::GlobedTransport>());
    log::info("GD Party initialized");
}

class $modify(GDPartyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto bottomMenu = this->getChildByID("bottom-menu");
        if (!bottomMenu) {
            log::warn("GD Party could not find MenuLayer bottom-menu");
            return true;
        }

        auto buttonSprite = ButtonSprite::create("Party", 70, true, "bigFont.fnt", "GJ_button_01.png", 30.0f, 0.75f);
        auto partyButton = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(GDPartyMenuLayer::onPartyButton)
        );

        partyButton->setID("party-button"_spr);
        bottomMenu->addChild(partyButton);
        bottomMenu->updateLayout();

        return true;
    }

    void onPartyButton(CCObject*) {
        PlatformToolbox::showCursor();
        if (auto popup = gdparty::PartyPopup::create()) {
            popup->show();
        }
    }
};

class $modify(GDPartyPauseLayer, PauseLayer) {
    void onPracticeMode(CCObject* sender) {
        auto& manager = gdparty::PartyManager::get();
        auto const& state = manager.state();
        auto localPlayer = manager.localPlayer();
        auto playLayer = PlayLayer::get();

        auto isAssignedPartyLevel =
            state.active &&
            state.phase == gdparty::PartyPhase::Playing &&
            localPlayer &&
            !localPlayer->finished &&
            playLayer &&
            playLayer->m_level &&
            playLayer->m_level->m_levelID.value() == localPlayer->currentLevelId;

        if (isAssignedPartyLevel && !state.settings.practiceModeEnabled && !playLayer->m_isPracticeMode) {
            Notification::create("Practice mode is disabled for this match", NotificationIcon::Warning)->show();
            return;
        }

        PauseLayer::onPracticeMode(sender);
    }
};

class $modify(GDPartyPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* targetLabel = nullptr;
        CCLabelBMFont* attemptLabel = nullptr;
        int bestThisAttempt = 0;
        bool blockRestart = false;
        bool decisionShown = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto windowSize = CCDirector::sharedDirector()->getWinSize();
        auto parent = this->m_uiLayer ? static_cast<CCNode*>(this->m_uiLayer) : static_cast<CCNode*>(this);

        m_fields->targetLabel = CCLabelBMFont::create("REACH 1%", "bigFont.fnt");
        m_fields->targetLabel->setScale(0.34f);
        m_fields->targetLabel->setPosition({windowSize.width / 2.0f, windowSize.height - 30.0f});
        m_fields->targetLabel->setZOrder(1000);
        m_fields->targetLabel->setVisible(false);
        parent->addChild(m_fields->targetLabel);

        m_fields->attemptLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_fields->attemptLabel->setScale(0.38f);
        m_fields->attemptLabel->setPosition({windowSize.width / 2.0f, windowSize.height - 42.0f});
        m_fields->attemptLabel->setZOrder(1000);
        m_fields->attemptLabel->setOpacity(210);
        m_fields->attemptLabel->setVisible(false);
        parent->addChild(m_fields->attemptLabel);

        refreshPartyHud();
        return true;
    }

    bool isPartyRunActive() {
        if (!this->m_level) {
            return false;
        }

        auto& manager = gdparty::PartyManager::get();
        auto const& state = manager.state();
        auto localPlayer = manager.localPlayer();
        auto levelId = this->m_level->m_levelID.value();

        if (!state.active || state.phase != gdparty::PartyPhase::Playing || !localPlayer || localPlayer->finished) {
            return false;
        }

        if (localPlayer->currentLevelId != levelId) {
            return false;
        }

        if (this->m_isPracticeMode && !state.settings.practiceModeEnabled) {
            return false;
        }

        return true;
    }

    void postUpdate(float deltaTime) {
        PlayLayer::postUpdate(deltaTime);

        auto& partyManager = gdparty::PartyManager::get();
        partyManager.refreshRoomContext();

        if (!isPartyRunActive()) {
            if (m_fields->targetLabel) {
                m_fields->targetLabel->setVisible(false);
            }
            if (m_fields->attemptLabel) {
                m_fields->attemptLabel->setVisible(false);
            }

            auto const& state = partyManager.state();
            auto localPlayer = partyManager.localPlayer();
            if (
                m_fields->blockRestart &&
                state.phase == gdparty::PartyPhase::Playing &&
                localPlayer &&
                localPlayer->currentLevelId > 0 &&
                this->m_level &&
                localPlayer->currentLevelId != this->m_level->m_levelID.value()
            ) {
                m_fields->blockRestart = false;
                gdparty::PartyLevelLoader::get().openAssignedLevel();
            }
            return;
        }

        auto currentProgress = std::clamp(this->getCurrentPercentInt(), 0, 100);
        m_fields->bestThisAttempt = std::max(m_fields->bestThisAttempt, currentProgress);

        partyManager.reportLocalProgress(
            currentProgress,
            this->m_level->m_levelID.value()
        );

        refreshPartyHud();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto shouldShowDecision = false;
        auto shouldShowResults = false;
        auto achievedProgress = 0;

        if (isPartyRunActive() && !m_fields->decisionShown) {
            auto& manager = gdparty::PartyManager::get();
            auto const& state = manager.state();
            auto localPlayer = manager.localPlayer();

            achievedProgress = std::max(
                m_fields->bestThisAttempt,
                std::clamp(this->getCurrentPercentInt(), 0, 100)
            );

            manager.reportAttemptEnded(
                achievedProgress,
                this->m_level->m_levelID.value()
            );

            if (state.settings.gameMode == gdparty::GameModeType::RecentRush) {
                auto reachedGoal = achievedProgress >= state.settings.targetProgress;
                auto exhaustedLevel = localPlayer &&
                    localPlayer->attemptCount + 1 >= state.settings.maxAttempts;

                if (reachedGoal || exhaustedLevel) {
                    m_fields->blockRestart = true;
                    m_fields->decisionShown = true;
                }
                shouldShowResults = reachedGoal;
            } else if (localPlayer && achievedProgress >= localPlayer->rouletteTarget) {
                m_fields->blockRestart = true;
                m_fields->decisionShown = true;
                shouldShowDecision = true;
            }
        }

        PlayLayer::destroyPlayer(player, object);

        if (shouldShowDecision) {
            Loader::get()->queueInMainThread([achievedProgress] {
                PlatformToolbox::showCursor();
                if (auto popup = gdparty::RoundDecisionPopup::create(achievedProgress)) {
                    popup->show();
                }
            });
        } else if (shouldShowResults) {
            Loader::get()->queueInMainThread([] {
                PlatformToolbox::showCursor();
                if (auto popup = gdparty::PartyPopup::create()) {
                    popup->show();
                }
            });
        }
    }

    void levelComplete() {
        auto shouldShowDecision = false;
        auto shouldShowResults = false;

        if (isPartyRunActive() && !m_fields->decisionShown) {
            auto& manager = gdparty::PartyManager::get();
            auto const& state = manager.state();
            auto localPlayer = manager.localPlayer();
            constexpr int AchievedProgress = 100;

            manager.reportLocalProgress(
                AchievedProgress,
                this->m_level->m_levelID.value()
            );
            manager.reportAttemptEnded(
                AchievedProgress,
                this->m_level->m_levelID.value()
            );

            if (state.settings.gameMode == gdparty::GameModeType::RecentRush) {
                m_fields->blockRestart = true;
                m_fields->decisionShown = true;
                shouldShowResults = true;
            } else if (localPlayer && AchievedProgress >= localPlayer->rouletteTarget) {
                m_fields->blockRestart = true;
                m_fields->decisionShown = true;
                shouldShowDecision = true;
            }
        }

        PlayLayer::levelComplete();

        if (shouldShowDecision) {
            Loader::get()->queueInMainThread([] {
                PlatformToolbox::showCursor();
                if (auto popup = gdparty::RoundDecisionPopup::create(100)) {
                    popup->show();
                }
            });
        } else if (shouldShowResults) {
            Loader::get()->queueInMainThread([] {
                PlatformToolbox::showCursor();
                if (auto popup = gdparty::PartyPopup::create()) {
                    popup->show();
                }
            });
        }
    }

    void resetLevel() {
        if (m_fields->blockRestart) {
            return;
        }

        m_fields->bestThisAttempt = 0;
        m_fields->decisionShown = false;
        PlayLayer::resetLevel();
    }

    void refreshPartyHud() {
        if (!m_fields->targetLabel || !m_fields->attemptLabel || !this->m_level) {
            return;
        }

        auto& partyManager = gdparty::PartyManager::get();
        auto const& state = partyManager.state();
        auto localPlayer = partyManager.localPlayer();
        auto shouldShow = isPartyRunActive();

        m_fields->targetLabel->setVisible(shouldShow);
        m_fields->attemptLabel->setVisible(shouldShow && state.settings.gameMode == gdparty::GameModeType::RecentRush);
        if (!shouldShow || !localPlayer) {
            return;
        }

        auto currentProgress = std::max(
            m_fields->bestThisAttempt,
            std::clamp(this->getCurrentPercentInt(), 0, 100)
        );

        if (state.settings.gameMode == gdparty::GameModeType::RecentRush) {
            auto secured = currentProgress >= state.settings.targetProgress;
            m_fields->targetLabel->setString(
                fmt::format("GOAL {}%", state.settings.targetProgress).c_str()
            );
            m_fields->targetLabel->setColor(
                secured ? ccc3(255, 210, 70) : ccc3(255, 255, 255)
            );
            m_fields->attemptLabel->setString(
                fmt::format(
                    "ATTEMPT {}/{}",
                    std::min(localPlayer->attemptCount + 1, state.settings.maxAttempts),
                    state.settings.maxAttempts
                ).c_str()
            );
        } else {
            auto secured = localPlayer->roundSecured || currentProgress >= localPlayer->rouletteTarget;
            m_fields->targetLabel->setString(
                fmt::format("REACH {}%", localPlayer->rouletteTarget).c_str()
            );
            m_fields->targetLabel->setColor(
                secured ? ccc3(255, 210, 70) : ccc3(255, 255, 255)
            );
            m_fields->attemptLabel->setVisible(false);
        }
    }
};
