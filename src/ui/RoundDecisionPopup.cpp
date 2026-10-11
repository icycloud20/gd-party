#include "RoundDecisionPopup.hpp"

#include "LeaderboardPopup.hpp"
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
    if (!Popup::init(330.0f, 190.0f, "GJ_square01.png")) {
        return false;
    }

    m_achievedProgress = achievedProgress;

    auto manager = &PartyManager::get();
    auto player = manager->localPlayer();
    auto target = player ? player->rouletteTarget : 0;

    setTitle("Target secured!");

    auto resultLabel = CCLabelBMFont::create(
        fmt::format("{}% reached", achievedProgress).c_str(),
        "goldFont.fnt"
    );
    resultLabel->setScale(0.52f);
    resultLabel->setPosition({165.0f, 128.0f});
    m_mainLayer->addChild(resultLabel);

    auto targetLabel = CCLabelBMFont::create(
        fmt::format("Required: {}%", target).c_str(),
        "bigFont.fnt"
    );
    targetLabel->setScale(0.34f);
    targetLabel->setPosition({165.0f, 105.0f});
    m_mainLayer->addChild(targetLabel);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu);

    auto nextSprite = ButtonSprite::create("Next Level", 115, true, "bigFont.fnt", "GJ_button_01.png", 30.0f, 0.62f);
    m_nextButton = CCMenuItemSpriteExtra::create(
        nextSprite,
        this,
        menu_selector(RoundDecisionPopup::onNextLevel)
    );
    m_nextButton->setPosition({103.0f, 58.0f});
    buttonMenu->addChild(m_nextButton);

    auto leaderboardSprite = ButtonSprite::create("Leaderboard", 120, true, "bigFont.fnt", "GJ_button_02.png", 30.0f, 0.57f);
    auto leaderboardButton = CCMenuItemSpriteExtra::create(
        leaderboardSprite,
        this,
        menu_selector(RoundDecisionPopup::onLeaderboard)
    );
    leaderboardButton->setPosition({232.0f, 58.0f});
    buttonMenu->addChild(leaderboardButton);

    return true;
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
        if (auto leaderboard = LeaderboardPopup::create()) {
            leaderboard->show();
        }
        return;
    }

    auto label = CCLabelBMFont::create("Loading next level...", "goldFont.fnt");
    label->setScale(0.34f);
    label->setPosition({165.0f, 28.0f});
    m_mainLayer->addChild(label);
}

void RoundDecisionPopup::onLeaderboard(CCObject*) {
    if (auto leaderboard = LeaderboardPopup::create()) {
        leaderboard->show();
    }
}

} // namespace gdparty
