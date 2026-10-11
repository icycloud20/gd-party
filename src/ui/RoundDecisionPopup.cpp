#include "RoundDecisionPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

using namespace geode::prelude;

namespace gdparty {

RoundDecisionPopup* RoundDecisionPopup::create(RoundDecisionContext const& context) {
    auto popup = new RoundDecisionPopup();
    if (popup && popup->init(context)) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool RoundDecisionPopup::init(RoundDecisionContext const& context) {
    if (!geode::Popup::init(CCSize{300.0f, 148.0f})) {
        return false;
    }

    m_context = context;
    PlatformToolbox::showCursor();

    auto title = context.recentRush
        ? (context.goalReached ? "Goal Reached!" : "Level Complete")
        : "Round Secured";
    setTitle(title, "goldFont.fnt", 0.62f, 17.0f);

    auto infoPanel = CCScale9Sprite::create(
        "square02_001.png",
        CCRect{0.0f, 0.0f, 80.0f, 80.0f}
    );
    infoPanel->setContentSize({258.0f, 56.0f});
    infoPanel->setPosition({150.0f, 83.0f});
    infoPanel->setOpacity(72);
    m_mainLayer->addChild(infoPanel, -1);

    m_progressLabel = partyui::addLabel(
        m_mainLayer,
        fmt::format("PROGRESS {}%  |  GOAL {}%", context.achievedProgress, context.requiredProgress),
        "bigFont.fnt",
        0.37f,
        {150.0f, 87.0f},
        {0.5f, 0.5f}
    );
    m_progressLabel->limitLabelWidth(235.0f, 0.37f, 0.28f);

    m_attemptLabel = partyui::addLabel(
        m_mainLayer,
        context.recentRush
            ? fmt::format("ATTEMPT {} / {}", context.attemptNumber, context.maxAttempts)
            : "",
        "bigFont.fnt",
        0.24f,
        {150.0f, 66.0f},
        {0.5f, 0.5f}
    );
    m_attemptLabel->setOpacity(205);
    m_attemptLabel->setVisible(context.recentRush);

    m_nextButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onNextLevel),
        "Next Level",
        {88.0f, 30.0f},
        104.0f,
        "GJ_button_01.png",
        0.52f
    );
    m_leaderboardButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onLeaderboard),
        "Leaderboard",
        {212.0f, 30.0f},
        104.0f,
        "GJ_button_02.png",
        0.46f
    );

    m_statusLabel = partyui::addLabel(
        m_mainLayer,
        "",
        "bigFont.fnt",
        0.20f,
        {150.0f, 9.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(180);

    schedule(schedule_selector(RoundDecisionPopup::tick), 0.12f);
    refresh();
    return true;
}

void RoundDecisionPopup::tick(float) {
    PartyManager::get().refreshRoomContext();
    refresh();

    auto& manager = PartyManager::get();
    auto const& state = manager.state();
    auto player = manager.localPlayer();

    if (!m_waitingForRouletteAdvance) {
        return;
    }

    if (state.phase == PartyPhase::Results) {
        closeOverlay();
        if (auto leaderboard = LeaderboardPopup::create()) {
            leaderboard->switchTo();
        }
        return;
    }

    if (player && player->currentLevelId > 0 && player->currentLevelId != m_context.sourceLevelId) {
        closeOverlay();
        PartyLevelLoader::get().openAssignedLevelFromDecision();
    }
}

void RoundDecisionPopup::refresh() {
    auto const& state = PartyManager::get().state();
    auto player = PartyManager::get().localPlayer();

    if (m_context.recentRush) {
        if (m_context.goalReached || state.phase == PartyPhase::Results) {
            m_nextButton->setVisible(false);
            m_leaderboardButton->setPositionX(150.0f);
            m_statusLabel->setString("GOAL SECURED");
            return;
        }

        auto nextReady = player &&
            player->currentLevelId > 0 &&
            player->currentLevelId != m_context.sourceLevelId;
        m_nextButton->setVisible(true);
        m_nextButton->setEnabled(nextReady);
        m_statusLabel->setString(nextReady ? "NEXT LEVEL READY" : "FINDING NEXT LEVEL...");
        return;
    }

    if (m_waitingForRouletteAdvance) {
        m_nextButton->setEnabled(false);
        m_statusLabel->setString("FINDING NEXT LEVEL...");
    } else {
        m_nextButton->setEnabled(true);
        m_statusLabel->setString("");
    }
}

void RoundDecisionPopup::closeOverlay() {
    unschedule(schedule_selector(RoundDecisionPopup::tick));
    removeFromParentAndCleanup(true);
}

void RoundDecisionPopup::onNextLevel(CCObject*) {
    if (m_context.recentRush) {
        auto player = PartyManager::get().localPlayer();
        if (!player || player->currentLevelId <= 0 || player->currentLevelId == m_context.sourceLevelId) {
            Notification::create("The next level is still loading", NotificationIcon::Info)->show();
            return;
        }

        closeOverlay();
        PartyLevelLoader::get().openAssignedLevelFromDecision();
        return;
    }

    if (!PartyManager::get().requestAdvanceRound(m_context.achievedProgress)) {
        Notification::create("Round is not ready to advance yet", NotificationIcon::Warning)->show();
        return;
    }

    m_waitingForRouletteAdvance = true;
    refresh();
}

void RoundDecisionPopup::onLeaderboard(CCObject*) {
    PlatformToolbox::showCursor();
    closeOverlay();
    if (auto leaderboard = LeaderboardPopup::create()) {
        leaderboard->switchTo();
    }
}

} // namespace gdparty
