#include "PartyPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartySettingsPopup.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

#include <Geode/utils/cocos.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace gdparty {
namespace {

CCMenuItemSpriteExtra* makeButton(
    CCNode* parent,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* text,
    CCPoint position,
    float width = 92.0f,
    char const* texture = "GJ_button_01.png"
) {
    auto sprite = ButtonSprite::create(text, width, true, "bigFont.fnt", texture, 30.0f, 0.62f);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    parent->addChild(button);
    return button;
}

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
    if (!Popup::init(430.0f, 285.0f, "GJ_square01.png")) {
        return false;
    }

    setTitle("GD Party");

    m_statusLabel = CCLabelBMFont::create("Checking Globed...", "goldFont.fnt");
    m_statusLabel->setScale(0.46f);
    m_statusLabel->setPosition({215.0f, 232.0f});
    m_mainLayer->addChild(m_statusLabel);

    m_modeLabel = CCLabelBMFont::create("Roulette Race", "bigFont.fnt");
    m_modeLabel->setScale(0.58f);
    m_modeLabel->setPosition({215.0f, 204.0f});
    m_mainLayer->addChild(m_modeLabel);

    m_playersLabel = CCLabelBMFont::create("No players yet", "chatFont.fnt");
    m_playersLabel->setScale(0.58f);
    m_playersLabel->setAnchorPoint({0.0f, 1.0f});
    m_playersLabel->setPosition({35.0f, 175.0f});
    m_mainLayer->addChild(m_playersLabel);

    m_playerListLayer = CCNode::create();
    m_playerListLayer->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(m_playerListLayer);

    m_targetLabel = CCLabelBMFont::create("Goal: 50%", "bigFont.fnt");
    m_targetLabel->setScale(0.4f);
    m_targetLabel->setAnchorPoint({1.0f, 0.5f});
    m_targetLabel->setPosition({395.0f, 176.0f});
    m_mainLayer->addChild(m_targetLabel);

    m_roundLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_roundLabel->setScale(0.42f);
    m_roundLabel->setPosition({215.0f, 103.0f});
    m_mainLayer->addChild(m_roundLabel);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu);

    m_createButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onCreateParty), "Create", {215.0f, 61.0f});
    m_joinButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onJoinParty), "Join", {215.0f, 61.0f});
    m_readyButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onReady), "Ready", {110.0f, 61.0f});
    m_unreadyButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onUnready), "Unready", {110.0f, 61.0f}, 100.0f, "GJ_button_06.png");
    m_startButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onStart), "Start", {215.0f, 61.0f});
    m_settingsButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onSettings), "Settings", {320.0f, 61.0f}, 100.0f, "GJ_button_05.png");
    m_openLevelButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onOpenLevel), "Retry Level", {155.0f, 61.0f}, 112.0f);
    m_leaderboardButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onLeaderboard), "Leaderboard", {275.0f, 61.0f}, 116.0f, "GJ_button_02.png");
    m_leaveButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onLeaveParty), "Leave", {370.0f, 28.0f}, 72.0f, "GJ_button_06.png");

    auto& manager = PartyManager::get();
    manager.refreshRoomContext();
    if (manager.canUseParty() && manager.transport() && !manager.transport()->isRoomOwner()) {
        manager.discoverParty();
    }

    schedule(schedule_selector(PartyPopup::tick), 0.2f);
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
    setVisible(m_targetLabel, false);
    setVisible(m_playerListLayer, false);
    setVisible(m_playersLabel, true);

    if (!transport || !transport->available()) {
        auto globed = Loader::get()->getInstalledMod("dankmeme.globed2");
        if (globed && globed->isOrWillBeEnabled()) {
            m_statusLabel->setString("API-fixed Globed build required");
            m_playersLabel->setString("Use Globed 2.2.3 beta 2+\nor the official nightly build.");
        } else {
            m_statusLabel->setString("Globed is required");
            m_playersLabel->setString("Install or enable Globed first.");
        }
        m_roundLabel->setString("");
        return;
    }

    if (!transport->connected()) {
        m_statusLabel->setString("Connect to Globed first");
        m_playersLabel->setString("GD Party uses your Globed connection.");
        m_roundLabel->setString("");
        return;
    }

    if (!transport->inRoom()) {
        m_statusLabel->setString("Join a private Globed room");
        m_playersLabel->setString("The Globed room owner hosts the party.");
        m_roundLabel->setString("");
        return;
    }

    m_modeLabel->setString(modeName(state.settings.gameMode));
    m_targetLabel->setString(fmt::format("Goal: {}%", state.settings.targetProgress).c_str());

    if (!state.active) {
        m_statusLabel->setString(transport->isRoomOwner() ? "Your Globed room - ready to host" : "No GD Party found yet");
        m_playersLabel->setString(transport->isRoomOwner() ? "Create a party for everyone in this room." : "The room owner must create the party.");
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_roundLabel->setString("");
        return;
    }

    setVisible(m_playersLabel, false);
    setVisible(m_playerListLayer, true);
    setVisible(m_targetLabel, true);
    rebuildPlayerList();

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        setVisible(m_playersLabel, true);
        setVisible(m_playerListLayer, false);
        m_playersLabel->setString("Party found. Join when you're ready.");
        m_statusLabel->setString("Party found in this Globed room");
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_roundLabel->setString("");
        return;
    }

    setVisible(m_leaveButton, true);

    if (state.phase == PartyPhase::Lobby) {
        m_statusLabel->setString(manager.isHost() ? "You are hosting" : "Waiting in lobby");
        setVisible(m_readyButton, !localPlayer->ready);
        setVisible(m_unreadyButton, localPlayer->ready);
        setVisible(m_startButton, manager.isHost());
        setVisible(m_settingsButton, manager.isHost());
        m_roundLabel->setString("Ready up when everyone is here.");
        return;
    }

    if (state.phase == PartyPhase::Countdown) {
        m_statusLabel->setString("Finding a level...");
        m_roundLabel->setString("Selecting a rated level that matches the filter.");
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        m_statusLabel->setString(modeName(state.settings.gameMode));

        if (localPlayer->currentLevelId <= 0) {
            m_roundLabel->setString("Finding your next level...");
        } else if (PartyLevelLoader::get().isLoading()) {
            m_roundLabel->setString(fmt::format("Loading level {}...", localPlayer->currentLevelId).c_str());
        } else {
            m_roundLabel->setString(fmt::format("Reach {}%  |  Level {}", localPlayer->rouletteTarget, localPlayer->currentLevelId).c_str());
            setVisible(m_openLevelButton, true);
        }

        setVisible(m_leaderboardButton, true);
        return;
    }

    if (state.phase == PartyPhase::Results) {
        auto winner = manager.findPlayer(state.winnerAccountId);
        auto winnerName = winner ? winner->name : "Unknown";
        m_statusLabel->setString("Match finished!");
        m_roundLabel->setString(fmt::format("{} wins!", winnerName).c_str());
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
    auto shown = std::min<std::size_t>(state.players.size(), 5);

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = state.players[index];
        auto y = 170.0f - static_cast<float>(index) * 23.0f;

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.36f);
        icon->setPosition({48.0f, y});

        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }

        m_playerListLayer->addChild(icon);

        auto nameLabel = CCLabelBMFont::create(player.name.c_str(), "chatFont.fnt");
        nameLabel->setScale(0.5f);
        nameLabel->setAnchorPoint({0.0f, 0.5f});
        nameLabel->setPosition({68.0f, y + 2.0f});
        nameLabel->limitLabelWidth(112.0f, 0.5f, 0.32f);
        m_playerListLayer->addChild(nameLabel);

        if (player.accountId == state.hostAccountId) {
            auto hostLabel = CCLabelBMFont::create("HOST", "goldFont.fnt");
            hostLabel->setScale(0.22f);
            hostLabel->setAnchorPoint({0.0f, 0.5f});
            hostLabel->setPosition({184.0f, y + 2.0f});
            m_playerListLayer->addChild(hostLabel);
        }

        std::string status;
        if (state.phase == PartyPhase::Lobby) {
            status = player.ready ? "READY" : "WAITING";
        } else if (player.finished) {
            status = "FINISHED";
        } else {
            status = fmt::format("Best {}%  |  Target {}%", player.progress, player.rouletteTarget);
        }

        auto statusLabel = CCLabelBMFont::create(status.c_str(), state.phase == PartyPhase::Lobby && player.ready ? "goldFont.fnt" : "chatFont.fnt");
        statusLabel->setScale(0.39f);
        statusLabel->setAnchorPoint({0.0f, 0.5f});
        statusLabel->setPosition({222.0f, y + 2.0f});
        statusLabel->limitLabelWidth(165.0f, 0.39f, 0.27f);
        m_playerListLayer->addChild(statusLabel);
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
