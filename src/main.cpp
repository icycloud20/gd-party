#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

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
        gdparty::PartyPopup::create()->show();
    }
};

class $modify(GDPartyPlayLayer, PlayLayer) {
    struct Fields {
        CCLayerColor* panel = nullptr;
        CCLabelBMFont* targetLabel = nullptr;
        int bestThisAttempt = 0;
        bool awaitingRoundChoice = false;
        bool decisionShown = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto windowSize = CCDirector::sharedDirector()->getWinSize();
        auto parent = this->m_uiLayer ? static_cast<CCNode*>(this->m_uiLayer) : static_cast<CCNode*>(this);

        m_fields->panel = CCLayerColor::create(ccc4(0, 0, 0, 135), 128.0f, 27.0f);
        m_fields->panel->setPosition({windowSize.width / 2.0f - 64.0f, windowSize.height - 34.0f});
        m_fields->panel->setZOrder(1000);
        m_fields->panel->setVisible(false);
        parent->addChild(m_fields->panel);

        m_fields->targetLabel = CCLabelBMFont::create("REACH 1%", "bigFont.fnt");
        m_fields->targetLabel->setScale(0.43f);
        m_fields->targetLabel->setPosition({64.0f, 13.5f});
        m_fields->panel->addChild(m_fields->targetLabel);

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
            if (m_fields->panel) {
                m_fields->panel->setVisible(false);
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
        auto achievedProgress = 0;

        if (isPartyRunActive() && !m_fields->decisionShown) {
            auto localPlayer = gdparty::PartyManager::get().localPlayer();
            achievedProgress = std::max(
                m_fields->bestThisAttempt,
                std::clamp(this->getCurrentPercentInt(), 0, 100)
            );

            gdparty::PartyManager::get().reportAttemptEnded(
                achievedProgress,
                this->m_level->m_levelID.value()
            );

            if (localPlayer && achievedProgress >= localPlayer->rouletteTarget) {
                m_fields->awaitingRoundChoice = true;
                m_fields->decisionShown = true;
                shouldShowDecision = true;
            }
        }

        PlayLayer::destroyPlayer(player, object);

        if (shouldShowDecision) {
            Loader::get()->queueInMainThread([achievedProgress] {
                if (auto popup = gdparty::RoundDecisionPopup::create(achievedProgress)) {
                    popup->show();
                }
            });
        }
    }

    void levelComplete() {
        auto shouldShowDecision = false;

        if (isPartyRunActive() && !m_fields->decisionShown) {
            auto localPlayer = gdparty::PartyManager::get().localPlayer();
            auto achievedProgress = 100;

            gdparty::PartyManager::get().reportLocalProgress(
                achievedProgress,
                this->m_level->m_levelID.value()
            );
            gdparty::PartyManager::get().reportAttemptEnded(
                achievedProgress,
                this->m_level->m_levelID.value()
            );

            if (localPlayer && achievedProgress >= localPlayer->rouletteTarget) {
                m_fields->awaitingRoundChoice = true;
                m_fields->decisionShown = true;
                shouldShowDecision = true;
            }
        }

        PlayLayer::levelComplete();

        if (shouldShowDecision) {
            Loader::get()->queueInMainThread([] {
                if (auto popup = gdparty::RoundDecisionPopup::create(100)) {
                    popup->show();
                }
            });
        }
    }

    void resetLevel() {
        if (m_fields->awaitingRoundChoice) {
            return;
        }

        m_fields->bestThisAttempt = 0;
        m_fields->decisionShown = false;
        PlayLayer::resetLevel();
    }

    void refreshPartyHud() {
        if (!m_fields->panel || !m_fields->targetLabel || !this->m_level) {
            return;
        }

        auto& partyManager = gdparty::PartyManager::get();
        auto localPlayer = partyManager.localPlayer();
        auto shouldShow = isPartyRunActive();

        m_fields->panel->setVisible(shouldShow);
        if (!shouldShow || !localPlayer) {
            return;
        }

        auto currentProgress = std::max(
            m_fields->bestThisAttempt,
            std::clamp(this->getCurrentPercentInt(), 0, 100)
        );
        auto secured = localPlayer->roundSecured || currentProgress >= localPlayer->rouletteTarget;

        m_fields->targetLabel->setString(
            fmt::format("REACH {}%", localPlayer->rouletteTarget).c_str()
        );
        m_fields->targetLabel->setColor(
            secured
                ? ccc3(255, 210, 70)
                : ccc3(255, 255, 255)
        );
    }
};
