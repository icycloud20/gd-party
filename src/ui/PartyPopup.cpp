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

} // namespace

PartyPopup* PartyPopup::create() {
    auto popup = new PartyPopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool PartyPopup::init() {
    if (!Popup::init(420.0f, 285.0f, "GJ_square01.png")) {
        return false;
    }

    setTitle("Party");

    m_statusLabel = partyui::addLabel(
        m_mainLayer,
        "Checking Globed...",
        "chatFont.fnt",
        0.43f,
        {210.0f, 247.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(200);

    partyui::addPanel(m_mainLayer, {210.0f, 211.0f}, {360.0f, 42.0f}, 60);
    m_modeLabel = partyui::addLabel(
        m_mainLayer,
        "Roulette Race",
        "bigFont.fnt",
        0.43f,
        {48.0f, 211.0f}
    );

    m_targetLabel = partyui::addLabel(
        m_mainLayer,
        "Goal 50%",
        "goldFont.fnt",
        0.34f,
        {364.0f, 211.0f},
        {1.0f, 0.5f}
    );

    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({285.0f, 211.0f});
    m_mainLayer->addChild(m_difficultyIconNode, 2);

    partyui::addPanel(m_mainLayer, {210.0f, 143.0f}, {360.0f, 88.0f}, 54);
    m_playersLabel = partyui::addLabel(
        m_mainLayer,
        "No players yet",
        "chatFont.fnt",
        0.48f,
        {210.0f, 143.0f},
        {0.5f, 0.5f}
    );
    m_playersLabel->setOpacity(210);

    m_playerListLayer = CCNode::create();
    m_mainLayer->addChild(m_playerListLayer, 2);

    m_roundLabel = partyui::addLabel(
        m_mainLayer,
        "",
        "chatFont.fnt",
        0.38f,
        {210.0f, 83.0f},
        {0.5f, 0.5f}
    );
    m_roundLabel->setOpacity(210);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu, 3);

    m_createButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onCreateParty),
        "Create Party",
        {210.0f, 45.0f},
        112.0f
    );
    m_joinButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onJoinParty),
        "Join Party",
        {210.0f, 45.0f},
        105.0f
    );

    m_readyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onReady),
        "Ready",
        {105.0f, 45.0f},
        82.0f
    );
    m_unreadyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onUnready),
        "Unready",
        {105.0f, 45.0f},
        88.0f,
        "GJ_button_06.png"
    );
    m_startButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onStart),
        "Start",
        {210.0f, 45.0f},
        82.0f
    );
    m_settingsButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onSettings),
        "Settings",
        {315.0f, 45.0f},
        88.0f,
        "GJ_button_05.png"
    );

    m_openLevelButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onOpenLevel),
        "Level",
        {155.0f, 45.0f},
        82.0f
    );
    m_leaderboardButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onLeaderboard),
        "Standings",
        {260.0f, 45.0f},
        96.0f,
        "GJ_button_02.png"
    );
    m_leaveButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onLeaveParty),
        "Leave",
        {374.0f, 20.0f},
        62.0f,
        "GJ_button_06.png",
        0.48f
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
    setVisible(m_settingsButton, false);
    setVisible(m_openLevelButton, false);
    setVisible(m_leaderboardButton, false);
    setVisible(m_leaveButton, false);
    setVisible(m_playerListLayer, false);
    setVisible(m_playersLabel, true);

    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(state.settings.difficulty, 0.22f)) {
        m_difficultyIconNode->addChild(icon);
    }

    m_modeLabel->setString(modeName(state.settings.gameMode));
    m_targetLabel->setString(
        fmt::format("{}  -  {}%", partyui::difficultyName(state.settings.difficulty), state.settings.targetProgress).c_str()
    );
    m_targetLabel->limitLabelWidth(120.0f, 0.34f, 0.24f);

    if (!transport || !transport->available()) {
        auto globed = Loader::get()->getInstalledMod("dankmeme.globed2");
        m_statusLabel->setString(globed && globed->isOrWillBeEnabled() ? "Update Globed to a supported build" : "Globed is required");
        m_playersLabel->setString("Connect through Globed first");
        m_roundLabel->setString("");
        return;
    }

    if (!transport->connected()) {
        m_statusLabel->setString("Not connected to Globed");
        m_playersLabel->setString("Connect to a Globed server first");
        m_roundLabel->setString("");
        return;
    }

    if (!transport->inRoom()) {
        m_statusLabel->setString("No private room");
        m_playersLabel->setString("Join a private Globed room to play");
        m_roundLabel->setString("");
        return;
    }

    if (!state.active) {
        m_statusLabel->setString(transport->isRoomOwner() ? "Private room ready" : "Waiting for the room owner");
        m_playersLabel->setString(transport->isRoomOwner() ? "Create a party to begin" : "The room owner has not created a party yet");
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_roundLabel->setString("");
        return;
    }

    setVisible(m_playersLabel, false);
    setVisible(m_playerListLayer, true);
    rebuildPlayerList();

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        setVisible(m_playersLabel, true);
        setVisible(m_playerListLayer, false);
        m_playersLabel->setString("Party found in this room");
        m_statusLabel->setString("Join when you're ready");
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_roundLabel->setString("");
        return;
    }

    setVisible(m_leaveButton, true);

    if (state.phase == PartyPhase::Lobby) {
        m_statusLabel->setString(manager.isHost() ? "Hosting" : "Lobby");
        setVisible(m_readyButton, !localPlayer->ready);
        setVisible(m_unreadyButton, localPlayer->ready);
        setVisible(m_startButton, manager.isHost());
        setVisible(m_settingsButton, manager.isHost());
        m_roundLabel->setString(localPlayer->ready ? "Ready - waiting for everyone else" : "Ready up when everyone is here");
        return;
    }

    if (state.phase == PartyPhase::Countdown) {
        m_statusLabel->setString("Starting match");
        m_roundLabel->setString("Finding a rated level...");
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        m_statusLabel->setString(modeName(state.settings.gameMode));

        if (localPlayer->currentLevelId <= 0) {
            m_roundLabel->setString("Finding your next level...");
        } else if (PartyLevelLoader::get().isLoading()) {
            m_roundLabel->setString("Loading level page...");
        } else if (state.settings.gameMode == GameModeType::RecentRush) {
            m_roundLabel->setString(
                fmt::format(
                    "Level {}  -  attempt {}/{}  -  best {}%",
                    localPlayer->rouletteTarget,
                    localPlayer->attemptCount + 1,
                    state.settings.maxAttempts,
                    localPlayer->progress
                ).c_str()
            );
            setVisible(m_openLevelButton, true);
        } else {
            m_roundLabel->setString(
                fmt::format("Target {}%  -  best {}%", localPlayer->rouletteTarget, localPlayer->progress).c_str()
            );
            setVisible(m_openLevelButton, true);
        }

        setVisible(m_leaderboardButton, true);
        return;
    }

    if (state.phase == PartyPhase::Results) {
        auto winner = manager.findPlayer(state.winnerAccountId);
        auto winnerName = winner ? winner->name : "Unknown";
        m_statusLabel->setString("Match finished");
        m_roundLabel->setString(fmt::format("{} wins", winnerName).c_str());
        setVisible(m_leaderboardButton, true);
    }
}

void PartyPopup::rebuildPlayerList() {
    if (!m_playerListLayer) {
        return;
    }

    m_playerListLayer->removeAllChildren();

    auto const& state = PartyManager::get().state();
    auto gameManager = GameManager::sharedState();
    auto shown = std::min<std::size_t>(state.players.size(), 3);

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = state.players[index];
        auto y = 168.0f - static_cast<float>(index) * 27.0f;

        if (index > 0) {
            partyui::addDivider(m_playerListLayer, 31.0f, y + 13.0f, 358.0f);
        }

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.34f);
        icon->setPosition({48.0f, y});

        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        m_playerListLayer->addChild(icon);

        auto nameLabel = partyui::addLabel(
            m_playerListLayer,
            player.name,
            "chatFont.fnt",
            0.48f,
            {68.0f, y + 1.0f}
        );
        nameLabel->limitLabelWidth(120.0f, 0.48f, 0.32f);

        if (player.accountId == state.hostAccountId) {
            auto hostLabel = partyui::addLabel(
                m_playerListLayer,
                "HOST",
                "goldFont.fnt",
                0.2f,
                {192.0f, y + 1.0f}
            );
            hostLabel->setOpacity(220);
        }

        std::string status;
        char const* statusFont = "chatFont.fnt";
        if (state.phase == PartyPhase::Lobby) {
            status = player.ready ? "READY" : "WAITING";
            statusFont = player.ready ? "goldFont.fnt" : "chatFont.fnt";
        } else if (player.finished) {
            status = "FINISHED";
            statusFont = "goldFont.fnt";
        } else if (state.settings.gameMode == GameModeType::RecentRush) {
            status = fmt::format("Best {}%  |  L{}", player.progress, player.rouletteTarget);
        } else {
            status = fmt::format("Best {}%  |  {}%", player.progress, player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            m_playerListLayer,
            status,
            statusFont,
            0.34f,
            {384.0f, y + 1.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(150.0f, 0.34f, 0.25f);
    }

    if (state.players.size() > shown) {
        auto moreLabel = partyui::addLabel(
            m_playerListLayer,
            fmt::format("+{} more", state.players.size() - shown),
            "chatFont.fnt",
            0.3f,
            {210.0f, 104.0f},
            {0.5f, 0.5f}
        );
        moreLabel->setOpacity(180);
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
    if (auto popup = PartySettingsPopup::create()) {
        popup->show();
    }
}

void PartyPopup::onLeaderboard(CCObject*) {
    if (auto popup = LeaderboardPopup::create()) {
        popup->show();
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
