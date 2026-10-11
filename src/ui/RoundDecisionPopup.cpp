#include "RoundDecisionPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

using namespace geode::prelude;

namespace gdparty {

RoundDecisionPopup* RoundDecisionPopup::create(RoundDecisionContext const& context) {
    auto layer = new RoundDecisionPopup();
    if (layer && layer->init(context)) {
        layer->autorelease();
        return layer;
    }

    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool RoundDecisionPopup::init(RoundDecisionContext const& context) {
    if (!initBase({37, 50, 167})) {
        return false;
    }

    m_context = context;
    if (m_backMenu) {
        m_backMenu->setVisible(false);
    }

    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    setTouchPriority(-10000);
    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    PlatformToolbox::showCursor();

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);
    auto shellSize = CCSize{420.0f, 290.0f};

    auto shell = CCNode::create();
    shell->setID("round-shell");
    shell->setContentSize(shellSize);
    shell->setAnchorPoint({0.5f, 0.5f});
    shell->setPosition(center);
    addChild(shell, 2);

    auto outer = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    outer->setContentSize(shellSize);
    outer->setPosition({shellSize.width / 2.0f, shellSize.height / 2.0f});
    shell->addChild(outer, -5);

    m_titleLabel = partyui::addLabel(
        shell,
        context.recentRush
            ? (context.goalReached ? "GOAL REACHED!" : "LEVEL COMPLETE")
            : "ROUND SECURED",
        "goldFont.fnt",
        0.68f,
        {shellSize.width / 2.0f, shellSize.height - 18.0f},
        {0.5f, 0.5f}
    );
    m_titleLabel->limitLabelWidth(330.0f, 0.68f, 0.42f);

    constexpr float InfoLeft = 42.0f;
    constexpr float InfoBottom = 83.0f;
    auto infoSize = CCSize{336.0f, 132.0f};
    partyui::addBlueListFrame(shell, {InfoLeft, InfoBottom}, infoSize);

    auto header = CCLayerColor::create(ccc4(0x28, 0x35, 0x77, 255), infoSize.width, 40.0f);
    header->setPosition({InfoLeft, InfoBottom + infoSize.height - 40.0f});
    shell->addChild(header, 2);

    auto modeLabel = partyui::addLabel(
        header,
        context.recentRush ? "RECENT RUSH" : "ROULETTE RACE",
        "bigFont.fnt",
        0.39f,
        {12.0f, 20.0f}
    );
    modeLabel->limitLabelWidth(150.0f, 0.39f, 0.28f);

    auto goalLabel = partyui::addLabel(
        header,
        fmt::format("{} {}%", context.recentRush ? "GOAL" : "TARGET", context.requiredProgress),
        "goldFont.fnt",
        0.40f,
        {infoSize.width - 12.0f, 20.0f},
        {1.0f, 0.5f}
    );
    goalLabel->limitLabelWidth(130.0f, 0.40f, 0.28f);

    m_progressLabel = partyui::addLabel(
        shell,
        fmt::format("BEST {}%", context.achievedProgress),
        "goldFont.fnt",
        0.58f,
        {shellSize.width / 2.0f, 142.0f},
        {0.5f, 0.5f}
    );
    m_progressLabel->setZOrder(5);

    auto detail = context.recentRush
        ? fmt::format("ATTEMPT {} / {}", context.attemptNumber, context.maxAttempts)
        : fmt::format("ROULETTE TARGET {}%", context.sourceTarget);
    m_detailLabel = partyui::addLabel(
        shell,
        detail,
        "bigFont.fnt",
        0.29f,
        {shellSize.width / 2.0f, 115.0f},
        {0.5f, 0.5f}
    );
    m_detailLabel->setOpacity(220);
    m_detailLabel->setZOrder(5);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    shell->addChild(buttonMenu, 10);

    m_nextButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onNextLevel),
        "Next Level",
        {shellSize.width / 2.0f - 67.0f, 43.0f},
        108.0f,
        "GJ_button_01.png",
        0.57f
    );
    m_leaderboardButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onLeaderboard),
        "Leaderboard",
        {shellSize.width / 2.0f + 67.0f, 43.0f},
        108.0f,
        "GJ_button_02.png",
        0.50f
    );

    m_statusLabel = partyui::addLabel(
        shell,
        "",
        "bigFont.fnt",
        0.22f,
        {shellSize.width / 2.0f, 19.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(190);
    m_statusLabel->setZOrder(5);

    schedule(schedule_selector(RoundDecisionPopup::tick), 0.12f);
    refresh();
    return true;
}

void RoundDecisionPopup::show() {
    PlatformToolbox::showCursor();
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) {
        return;
    }

    scene->addChild(this, 100000);

    if (m_background) {
        m_background->setOpacity(0);
        m_background->runAction(CCFadeTo::create(0.16f, 255));
    }

    if (auto shell = getChildByID("round-shell")) {
        shell->setScale(0.96f);
        shell->runAction(CCEaseBackOut::create(CCScaleTo::create(0.18f, 1.0f)));
    }
}

bool RoundDecisionPopup::ccTouchBegan(CCTouch*, CCEvent*) {
    return true;
}

void RoundDecisionPopup::keyBackClicked() {
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
            m_leaderboardButton->setPositionX(210.0f);
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
    if (auto leaderboard = LeaderboardPopup::create()) {
        leaderboard->switchTo();
    }
}

} // namespace gdparty
