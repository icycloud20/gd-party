#include "PartyPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartySettingsPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace gdparty {
namespace {

char const* modeName(GameModeType mode) {
    return mode == GameModeType::RecentRush ? "Recent Rush" : "Roulette Race";
}

CCScale9Sprite* makePanel(CCNode* parent, CCPoint position, CCSize size, char const* texture, GLubyte opacity = 255) {
    auto panel = CCScale9Sprite::create(texture, CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(opacity);
    parent->addChild(panel);
    return panel;
}

CCMenuItemSpriteExtra* makeIconButton(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* frame,
    CCPoint position,
    float scale = 0.78f
) {
    auto sprite = CCSprite::createWithSpriteFrameName(frame);
    sprite->setScale(scale);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

} // namespace

PartyPopup* PartyPopup::create() {
    auto layer = new PartyPopup();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }

    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool PartyPopup::init() {
    if (!initBase({37, 50, 167})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = windowSize / 2.0f;
    auto shellSize = CCSize{
        std::clamp(windowSize.width - 118.0f, 390.0f, 430.0f),
        std::clamp(windowSize.height - 34.0f, 270.0f, 292.0f)
    };

    m_shell = CCNode::create();
    m_shell->setContentSize(shellSize);
    m_shell->setAnchorPoint({0.5f, 0.5f});
    m_shell->setPosition(center);
    addChild(m_shell, 2);

    auto shellBackground = makePanel(
        m_shell,
        {shellSize.width / 2.0f, shellSize.height / 2.0f},
        shellSize,
        "GJ_square02.png"
    );
    shellBackground->setZOrder(-5);

    partyui::addLabel(
        m_shell,
        "GD PARTY",
        "goldFont.fnt",
        0.72f,
        {shellSize.width / 2.0f, shellSize.height - 20.0f},
        {0.5f, 0.5f}
    );

    m_statusLabel = partyui::addLabel(
        m_shell,
        "Checking Globed...",
        "chatFont.fnt",
        0.42f,
        {shellSize.width / 2.0f, shellSize.height - 39.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(180);

    auto summaryCenterY = shellSize.height - 69.0f;
    auto summaryPanel = makePanel(
        m_shell,
        {shellSize.width / 2.0f, summaryCenterY},
        {shellSize.width - 40.0f, 37.0f},
        "square02_001.png",
        78
    );
    summaryPanel->setColor({20, 30, 90});

    m_summaryLayer = CCNode::create();
    m_shell->addChild(m_summaryLayer, 3);

    m_modeLabel = partyui::addLabel(
        m_summaryLayer,
        "Roulette Race",
        "bigFont.fnt",
        0.38f,
        {34.0f, summaryCenterY}
    );
    m_modeLabel->limitLabelWidth(126.0f, 0.38f, 0.28f);

    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({shellSize.width - 190.0f, summaryCenterY});
    m_summaryLayer->addChild(m_difficultyIconNode, 3);

    m_difficultyLabel = partyui::addLabel(
        m_summaryLayer,
        "Random",
        "goldFont.fnt",
        0.29f,
        {shellSize.width - 174.0f, summaryCenterY}
    );
    m_difficultyLabel->limitLabelWidth(105.0f, 0.29f, 0.21f);

    auto separator = partyui::addLabel(
        m_summaryLayer,
        "|",
        "chatFont.fnt",
        0.38f,
        {shellSize.width - 72.0f, summaryCenterY},
        {0.5f, 0.5f}
    );
    separator->setOpacity(120);

    m_goalLabel = partyui::addLabel(
        m_summaryLayer,
        "50%",
        "goldFont.fnt",
        0.36f,
        {shellSize.width - 28.0f, summaryCenterY},
        {1.0f, 0.5f}
    );

    auto listCenterY = shellSize.height / 2.0f - 4.0f;
    auto listHeight = shellSize.height - 128.0f;
    auto listPanel = makePanel(
        m_shell,
        {shellSize.width / 2.0f, listCenterY},
        {shellSize.width - 40.0f, listHeight},
        "square02_001.png",
        95
    );
    listPanel->setColor({15, 26, 86});

    m_playerListLayer = CCNode::create();
    m_shell->addChild(m_playerListLayer, 4);

    m_emptyLabel = partyui::addLabel(
        m_shell,
        "No players yet",
        "chatFont.fnt",
        0.48f,
        {shellSize.width / 2.0f, listCenterY},
        {0.5f, 0.5f}
    );
    m_emptyLabel->setOpacity(190);

    m_footerLabel = partyui::addLabel(
        m_shell,
        "",
        "chatFont.fnt",
        0.36f,
        {shellSize.width / 2.0f, 48.0f},
        {0.5f, 0.5f}
    );
    m_footerLabel->setOpacity(180);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(buttonMenu, 10);

    auto firstX = shellSize.width / 2.0f - 66.0f;
    auto secondX = shellSize.width / 2.0f + 66.0f;
    constexpr float PrimaryY = 22.0f;

    m_createButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onCreateParty),
        "Create Party",
        {shellSize.width / 2.0f, PrimaryY},
        118.0f,
        "GJ_button_01.png",
        0.56f
    );
    m_joinButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onJoinParty),
        "Join Party",
        {shellSize.width / 2.0f, PrimaryY},
        112.0f,
        "GJ_button_01.png",
        0.56f
    );
    m_readyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onReady),
        "Ready",
        {firstX, PrimaryY},
        104.0f,
        "GJ_button_01.png",
        0.62f
    );
    m_unreadyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onUnready),
        "Unready",
        {firstX, PrimaryY},
        104.0f,
        "GJ_button_06.png",
        0.55f
    );
    m_startButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onStart),
        "Start",
        {secondX, PrimaryY},
        104.0f,
        "GJ_button_01.png",
        0.62f
    );
    m_openLevelButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onOpenLevel),
        "Open Level",
        {shellSize.width / 2.0f, PrimaryY},
        118.0f,
        "GJ_button_01.png",
        0.57f
    );

    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.0f, 0.0f});
    addChild(sideMenu, 20);

    auto sideX = center.x + shellSize.width / 2.0f + 30.0f;
    m_settingsButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onSettings),
        "GJ_optionsBtn_001.png",
        {sideX, center.y + 32.0f},
        0.78f
    );
    m_leaderboardButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaderboard),
        "GJ_statsBtn_001.png",
        {sideX, center.y - 24.0f},
        0.78f
    );

    m_leaveButton = partyui::addTextButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaveParty),
        "Leave",
        {sideX, center.y - 83.0f},
        62.0f,
        "GJ_button_06.png",
        0.45f
    );

    auto& manager = PartyManager::get();
    manager.refreshRoomContext();
    if (manager.canUseParty() && manager.transport() && !manager.transport()->isRoomOwner()) {
        manager.discoverParty();
    }

    schedule(schedule_selector(PartyPopup::tick), 0.15f);
    refresh();
    return true;
}

void PartyPopup::tick(float) {
    PartyManager::get().refreshRoomContext();
    refresh();
}

void PartyPopup::refresh() {
    auto& manager = PartyManager::get();
    auto const& state = manager.state();
    auto transport = manager.transport();

    auto setVisible = [](CCNode* node, bool visible) {
        if (node) {
            node->setVisible(visible);
        }
    };

    setVisible(m_createButton, false);
    setVisible(m_joinButton, false);
    setVisible(m_readyButton, false);
    setVisible(m_unreadyButton, false);
    setVisible(m_startButton, false);
    setVisible(m_openLevelButton, false);
    setVisible(m_settingsButton, false);
    setVisible(m_leaderboardButton, false);
    setVisible(m_leaveButton, false);
    setVisible(m_playerListLayer, false);
    setVisible(m_emptyLabel, true);

    rebuildMatchSummary();

    if (!transport || !transport->available()) {
        auto globed = Loader::get()->getInstalledMod("dankmeme.globed2");
        m_statusLabel->setString(globed && globed->isOrWillBeEnabled() ? "Globed build is not compatible" : "Globed is required");
        m_emptyLabel->setString("Install or update Globed first");
        m_footerLabel->setString("");
        return;
    }

    if (!transport->connected()) {
        m_statusLabel->setString("Not connected");
        m_emptyLabel->setString("Connect to a Globed server first");
        m_footerLabel->setString("");
        return;
    }

    if (!transport->inRoom()) {
        m_statusLabel->setString("No room");
        m_emptyLabel->setString("Join a private Globed room first");
        m_footerLabel->setString("");
        return;
    }

    if (!state.active) {
        m_statusLabel->setString(transport->isRoomOwner() ? "Private room ready" : "Waiting for host");
        m_emptyLabel->setString(transport->isRoomOwner() ? "Create a party to begin" : "The room owner has not created a party yet");
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_footerLabel->setString("");
        return;
    }

    setVisible(m_emptyLabel, false);
    setVisible(m_playerListLayer, true);
    rebuildPlayerList();

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        setVisible(m_playerListLayer, false);
        setVisible(m_emptyLabel, true);
        m_statusLabel->setString("Party found");
        m_emptyLabel->setString("Join the party when you're ready");
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_footerLabel->setString("");
        return;
    }

    setVisible(m_leaveButton, true);
    setVisible(m_leaderboardButton, true);

    if (state.phase == PartyPhase::Lobby) {
        m_statusLabel->setString(manager.isHost() ? "Hosting" : "Lobby");
        setVisible(m_readyButton, !localPlayer->ready);
        setVisible(m_unreadyButton, localPlayer->ready);
        setVisible(m_startButton, manager.isHost());
        setVisible(m_settingsButton, manager.isHost());
        m_footerLabel->setString(localPlayer->ready ? "Ready - waiting for everyone else" : "Ready up when everyone is here");
        return;
    }

    if (state.phase == PartyPhase::Countdown) {
        m_statusLabel->setString("Starting match");
        m_footerLabel->setString("Finding the first level...");
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        m_statusLabel->setString(modeName(state.settings.gameMode));
        setVisible(m_openLevelButton, localPlayer->currentLevelId > 0 && !PartyLevelLoader::get().isLoading());

        if (localPlayer->currentLevelId <= 0) {
            m_footerLabel->setString("Finding your next level...");
        } else if (PartyLevelLoader::get().isLoading()) {
            m_footerLabel->setString("Loading level page...");
        } else if (state.settings.gameMode == GameModeType::RecentRush) {
            m_footerLabel->setString(
                fmt::format(
                    "Level {}  |  attempt {}/{}  |  best {}%",
                    localPlayer->rouletteTarget,
                    std::min(localPlayer->attemptCount + 1, state.settings.maxAttempts),
                    state.settings.maxAttempts,
                    localPlayer->progress
                ).c_str()
            );
        } else {
            m_footerLabel->setString(
                fmt::format("Target {}%  |  best {}%", localPlayer->rouletteTarget, localPlayer->progress).c_str()
            );
        }
        return;
    }

    if (state.phase == PartyPhase::Results) {
        auto winner = manager.findPlayer(state.winnerAccountId);
        m_statusLabel->setString("Match finished");
        m_footerLabel->setString(
            fmt::format("{} wins", winner ? winner->name : "Unknown").c_str()
        );
    }
}

void PartyPopup::rebuildMatchSummary() {
    auto const& settings = PartyManager::get().state().settings;

    m_modeLabel->setString(modeName(settings.gameMode));
    m_difficultyLabel->setString(partyui::difficultyName(settings.difficulty));
    m_goalLabel->setString(fmt::format("{}%", settings.targetProgress).c_str());

    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.18f)) {
        icon->setPosition({0.0f, 0.0f});
        m_difficultyIconNode->addChild(icon);
    }
}

void PartyPopup::rebuildPlayerList() {
    if (!m_playerListLayer || !m_shell) {
        return;
    }

    m_playerListLayer->removeAllChildren();

    auto const& state = PartyManager::get().state();
    auto gameManager = GameManager::sharedState();
    auto shellSize = m_shell->getContentSize();
    auto shown = std::min<std::size_t>(state.players.size(), 5);
    auto topY = shellSize.height - 103.0f;
    constexpr float RowHeight = 25.0f;

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = state.players[index];
        auto y = topY - static_cast<float>(index) * RowHeight;

        if (index % 2 == 1) {
            auto rowShade = CCLayerColor::create(ccc4(255, 255, 255, 10), shellSize.width - 50.0f, RowHeight);
            rowShade->setPosition({25.0f, y - RowHeight / 2.0f});
            m_playerListLayer->addChild(rowShade);
        }

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.31f);
        icon->setPosition({43.0f, y});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        m_playerListLayer->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            m_playerListLayer,
            player.name,
            "bigFont.fnt",
            0.31f,
            {61.0f, y}
        );
        nameLabel->limitLabelWidth(126.0f, 0.31f, 0.22f);

        if (player.accountId == state.hostAccountId) {
            auto hostLabel = partyui::addLabel(
                m_playerListLayer,
                "HOST",
                "goldFont.fnt",
                0.18f,
                {191.0f, y}
            );
            hostLabel->setOpacity(220);
        }

        std::string status;
        char const* font = "chatFont.fnt";
        if (state.phase == PartyPhase::Lobby) {
            status = player.ready ? "READY" : "WAITING";
            font = player.ready ? "goldFont.fnt" : "chatFont.fnt";
        } else if (player.finished) {
            status = "FINISHED";
            font = "goldFont.fnt";
        } else if (state.settings.gameMode == GameModeType::RecentRush) {
            status = fmt::format("BEST {}%   LEVEL {}", player.progress, player.rouletteTarget);
        } else {
            status = fmt::format("BEST {}%   TARGET {}%", player.progress, player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            m_playerListLayer,
            status,
            font,
            0.27f,
            {shellSize.width - 31.0f, y},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(170.0f, 0.27f, 0.2f);
    }

    if (state.players.size() > shown) {
        auto more = partyui::addLabel(
            m_playerListLayer,
            fmt::format("+{} more", state.players.size() - shown),
            "chatFont.fnt",
            0.27f,
            {shellSize.width / 2.0f, topY - static_cast<float>(shown) * RowHeight + 2.0f},
            {0.5f, 0.5f}
        );
        more->setOpacity(150);
    }
}

void PartyPopup::onCreateParty(CCObject*) {
    if (!PartyManager::get().createParty()) {
        Notification::create("You must own a private Globed room", NotificationIcon::Warning)->show();
    }
    refresh();
}

void PartyPopup::onJoinParty(CCObject*) {
    if (PartyManager::get().joinParty()) {
        Notification::create("Join request sent", NotificationIcon::Info)->show();
    }
}

void PartyPopup::onReady(CCObject*) {
    PartyManager::get().setReady(true);
    refresh();
}

void PartyPopup::onUnready(CCObject*) {
    PartyManager::get().setReady(false);
    refresh();
}

void PartyPopup::onStart(CCObject*) {
    PartyManager::get().startMatch();
    refresh();
}

void PartyPopup::onSettings(CCObject*) {
    if (auto layer = PartySettingsPopup::create()) {
        layer->switchTo();
    }
}

void PartyPopup::onLeaderboard(CCObject*) {
    if (auto layer = LeaderboardPopup::create()) {
        layer->switchTo();
    }
}

void PartyPopup::onOpenLevel(CCObject*) {
    PartyLevelLoader::get().openAssignedLevel();
    refresh();
}

void PartyPopup::onLeaveParty(CCObject*) {
    PartyManager::get().leaveParty();
    refresh();
}

} // namespace gdparty
