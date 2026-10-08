#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "core/PartyManager.hpp"
#include "network/GlobedTransport.hpp"
#include "ui/PartyPopup.hpp"

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
    void postUpdate(float deltaTime) {
        PlayLayer::postUpdate(deltaTime);

        auto& partyManager = gdparty::PartyManager::get();
        partyManager.refreshRoomContext();

        if (!this->m_level || this->m_isPracticeMode) {
            return;
        }

        partyManager.reportLocalProgress(
            this->getCurrentPercentInt(),
            this->m_level->m_levelID
        );
    }
};
