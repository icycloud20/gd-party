#include "RoundDecisionPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

using namespace geode::prelude;

namespace gdparty {

RoundDecisionPopup* RoundDecisionPopup::create(int achievedProgress) {
    auto popup = new RoundDecisionPopup();
    if (popup && popup->init(achievedProgress)) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool RoundDecisionPopup::init(int achievedProgress) {
    if (!Popup::init(310.0f, 154.0f, "GJ_square01.png")) {
        return false;
    }

    PlatformToolbox::showCursor();
    m_achievedProgress = achievedProgress;

    if (m_closeBtn) {
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);
    }

    auto player = PartyManager::get().localPlayer();
    auto target = player ? player->rouletteTarget : 0;

    setTitle("Round Clear");
    partyui::addPanel(m_mainLayer, {155.0f, 91.0f}, {250.0f, 45.0f}, 58);

    auto resultLabel = partyui::addLabel(
        m_mainLayer,
        fmt::format("{}%", achievedProgress),
        "goldFont.fnt",
        0.48f,
        {116.0f, 91.0f},
        {0.5f, 0.5f}
    );
    resultLabel->limitLabelWidth(72.0f, 0.48f, 0.35f);

    partyui::addLabel(
        m_mainLayer,
        fmt::format("target {}%", target),
        "chatFont.fnt",
        0.44f,
        {186.0f, 91.0f},
        {0.0f, 0.5f}
    );

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu, 3);

    m_nextButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onNextLevel),
        "Next",
        {105.0f, 43.0f},
        82.0f
    );
    partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onLeaderboard),
        "Standings",
        {207.0f, 43.0f},
        94.0f,
        "GJ_button_02.png",
        0.5f
    );

    return true;
}

void RoundDecisionPopup::keyDown(cocos2d::enumKeyCodes key, double timestamp) {
    if (key == cocos2d::enumKeyCodes::KEY_Escape) {
        return;
    }

    Popup::keyDown(key, timestamp);
}

void RoundDecisionPopup::onNextLevel(CCObject*) {
    if (!PartyManager::get().requestAdvanceRound(m_achievedProgress)) {
        Notification::create("Round is not ready to advance yet", NotificationIcon::Warning)->show();
        return;
    }

    if (m_nextButton) {
        m_nextButton->setEnabled(false);
    }

    auto const& settings = PartyManager::get().state().settings;
    auto player = PartyManager::get().localPlayer();
    auto willFinish = player && (
        (settings.overrunCounts && m_achievedProgress >= settings.targetProgress) ||
        (!settings.overrunCounts && player->rouletteTarget >= settings.targetProgress)
    );

    if (willFinish) {
        onClose(nullptr);
        PlatformToolbox::showCursor();
        if (auto leaderboard = LeaderboardPopup::create()) {
            leaderboard->show();
        }
        return;
    }

    auto loadingLabel = partyui::addLabel(
        m_mainLayer,
        "Loading next level...",
        "chatFont.fnt",
        0.35f,
        {155.0f, 18.0f},
        {0.5f, 0.5f}
    );
    loadingLabel->setOpacity(190);
}

void RoundDecisionPopup::onLeaderboard(CCObject*) {
    PlatformToolbox::showCursor();
    if (auto leaderboard = LeaderboardPopup::create()) {
        leaderboard->show();
    }
}

} // namespace gdparty
