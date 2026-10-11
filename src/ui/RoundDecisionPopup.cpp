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
    if (!CCLayerColor::initWithColor(ccc4(0, 0, 0, 178))) {
        return false;
    }

    m_context = context;
    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    setTouchPriority(-10000);
    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    PlatformToolbox::showCursor();

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = windowSize / 2.0f;
    auto cardSize = CCSize{392.0f, 178.0f};

    auto card = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    card->setContentSize(cardSize);
    card->setPosition(center);
    addChild(card, 1);

    m_titleLabel = partyui::addLabel(
        this,
        context.recentRush
            ? (context.goalReached ? "GOAL REACHED!" : "LEVEL FINISHED")
            : "ROUND CLEAR",
        "goldFont.fnt",
        0.68f,
        {center.x, center.y + 63.0f},
        {0.5f, 0.5f}
    );
    m_titleLabel->setZOrder(5);

    auto infoPanel = CCScale9Sprite::create("square02_001.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    infoPanel->setContentSize({cardSize.width - 48.0f, 56.0f});
    infoPanel->setPosition({center.x, center.y + 14.0f});
    infoPanel->setColor({15, 26, 86});
    infoPanel->setOpacity(100);
    addChild(infoPanel, 2);

    m_progressLabel = partyui::addLabel(
        this,
        fmt::format("{}%", context.achievedProgress),
        "goldFont.fnt",
        0.54f,
        {center.x - 72.0f, center.y + 17.0f},
        {0.5f, 0.5f}
    );
    m_progressLabel->setZOrder(5);

    auto detail = context.recentRush
        ? fmt::format("GOAL {}%", context.requiredProgress)
        : fmt::format("TARGET {}%", context.requiredProgress);
    m_detailLabel = partyui::addLabel(
        this,
        detail,
        "bigFont.fnt",
        0.31f,
        {center.x + 58.0f, center.y + 18.0f},
        {0.5f, 0.5f}
    );
    m_detailLabel->setZOrder(5);

    auto subDetail = context.recentRush
        ? fmt::format("attempt {}/{}", context.attemptNumber, context.maxAttempts)
        : fmt::format("roulette target {}", context.sourceTarget);
    auto subLabel = partyui::addLabel(
        this,
        subDetail,
        "chatFont.fnt",
        0.30f,
        {center.x + 58.0f, center.y + 3.0f},
        {0.5f, 0.5f}
    );
    subLabel->setOpacity(170);
    subLabel->setZOrder(5);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    addChild(buttonMenu, 10);

    m_nextButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onNextLevel),
        "Next Level",
        {center.x - 76.0f, center.y - 50.0f},
        120.0f,
        "GJ_button_01.png",
        0.55f
    );
    m_leaderboardButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(RoundDecisionPopup::onLeaderboard),
        "Leaderboard",
        {center.x + 76.0f, center.y - 50.0f},
        120.0f,
        "GJ_button_02.png",
        0.52f
    );

    m_statusLabel = partyui::addLabel(
        this,
        "",
        "chatFont.fnt",
        0.30f,
        {center.x, center.y - 76.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(165);
    m_statusLabel->setZOrder(5);

    schedule(schedule_selector(RoundDecisionPopup::tick), 0.12f);
    refresh();
    return true;
}

void RoundDecisionPopup::show() {
    PlatformToolbox::showCursor();
    if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
        scene->addChild(this, 100000);
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
        PartyLevelLoader::get().openAssignedLevel();
    }
}

void RoundDecisionPopup::refresh() {
    auto const& state = PartyManager::get().state();
    auto player = PartyManager::get().localPlayer();

    if (m_context.recentRush) {
        if (m_context.goalReached || state.phase == PartyPhase::Results) {
            m_nextButton->setVisible(false);
            m_leaderboardButton->setPositionX(CCDirector::sharedDirector()->getWinSize().width / 2.0f);
            m_statusLabel->setString("Goal secured");
            return;
        }

        auto nextReady = player &&
            player->currentLevelId > 0 &&
            player->currentLevelId != m_context.sourceLevelId;
        m_nextButton->setVisible(true);
        m_nextButton->setEnabled(nextReady);
        m_statusLabel->setString(nextReady ? "Next level ready" : "Finding next level...");
        return;
    }

    if (m_waitingForRouletteAdvance) {
        m_nextButton->setEnabled(false);
        m_statusLabel->setString("Finding next level...");
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
        PartyLevelLoader::get().openAssignedLevel();
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
