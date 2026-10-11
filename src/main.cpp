#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/PartyManager.hpp"
#include "network/GlobedTransport.hpp"
#include "ui/PartyPopup.hpp"

#include <algorithm>
#include <memory>
#include <string>

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
        CCLabelBMFont* standingsLabel = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto windowSize = CCDirector::sharedDirector()->getWinSize();
        auto parent = this->m_uiLayer ? static_cast<CCNode*>(this->m_uiLayer) : static_cast<CCNode*>(this);

        m_fields->panel = CCLayerColor::create(ccc4(0, 0, 0, 145), 220.0f, 39.0f);
        m_fields->panel->setPosition({windowSize.width / 2.0f - 110.0f, windowSize.height - 47.0f});
        m_fields->panel->setZOrder(1000);
        m_fields->panel->setVisible(false);
        parent->addChild(m_fields->panel);

        m_fields->targetLabel = CCLabelBMFont::create("GD PARTY", "bigFont.fnt");
        m_fields->targetLabel->setScale(0.45f);
        m_fields->targetLabel->setPosition({110.0f, 26.0f});
        m_fields->panel->addChild(m_fields->targetLabel);

        m_fields->standingsLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_fields->standingsLabel->setScale(0.5f);
        m_fields->standingsLabel->setPosition({110.0f, 10.0f});
        m_fields->panel->addChild(m_fields->standingsLabel);

        refreshPartyHud();
        return true;
    }

    void postUpdate(float deltaTime) {
        PlayLayer::postUpdate(deltaTime);

        auto& partyManager = gdparty::PartyManager::get();
        partyManager.refreshRoomContext();

        if (!this->m_level || this->m_isPracticeMode) {
            if (m_fields->panel) {
                m_fields->panel->setVisible(false);
            }
            return;
        }

        partyManager.reportLocalProgress(
            this->getCurrentPercentInt(),
            this->m_level->m_levelID.value()
        );

        refreshPartyHud();
    }

    void refreshPartyHud() {
        if (!m_fields->panel || !this->m_level) {
            return;
        }

        auto& partyManager = gdparty::PartyManager::get();
        auto const& state = partyManager.state();
        auto localPlayer = partyManager.localPlayer();
        auto levelId = this->m_level->m_levelID.value();

        auto shouldShow =
            state.active &&
            state.phase == gdparty::PartyPhase::Playing &&
            localPlayer &&
            !localPlayer->finished &&
            localPlayer->currentLevelId == levelId &&
            !this->m_isPracticeMode;

        m_fields->panel->setVisible(shouldShow);
        if (!shouldShow) {
            return;
        }

        m_fields->targetLabel->setString(
            fmt::format("GD PARTY  |  REACH {}%", localPlayer->rouletteTarget).c_str()
        );

        auto localCleared = std::max(0, localPlayer->rouletteTarget - 1);
        std::string standings = fmt::format("You: {}% cleared", localCleared);

        for (auto const& player : state.players) {
            if (player.accountId == localPlayer->accountId) {
                continue;
            }

            auto cleared = player.finished
                ? state.settings.targetProgress
                : std::max(0, player.rouletteTarget - 1);

            standings += fmt::format("   |   {}: {}%", player.name, cleared);
            break;
        }

        m_fields->standingsLabel->setString(standings.c_str());
        m_fields->standingsLabel->limitLabelWidth(205.0f, 0.5f, 0.25f);
    }
};
