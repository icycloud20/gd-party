#include "PartyPopup.hpp"

#include "../core/PartyManager.hpp"

#include <Geode/utils/cocos.hpp>

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
    auto sprite = ButtonSprite::create(text, width, true, "bigFont.fnt", texture, 30.0f, 0.65f);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    parent->addChild(button);
    return button;
}

std::string playerListText(PartyManager const& manager) {
    auto const& state = manager.state();
    if (state.players.empty()) {
        return "No players yet";
    }

    std::string text;
    std::size_t shown = 0;
    for (auto const& player : state.players) {
        if (shown >= 6) {
            text += "\n+ more";
            break;
        }

        if (!text.empty()) {
            text += "\n";
        }

        text += player.accountId == state.hostAccountId ? "[H] " : "    ";
        text += player.name;

        if (state.phase == PartyPhase::Lobby) {
            text += player.ready ? "  READY" : "  ...";
        } else if (state.phase == PartyPhase::Playing) {
            text += "  ";
            text += std::to_string(player.rouletteTarget);
            text += "%";
        }

        ++shown;
    }

    return text;
}

} // namespace

PartyPopup* PartyPopup::create() {
    auto popup = new PartyPopup();

    if (popup && popup->initAnchored(430.0f, 285.0f, "GJ_square01.png")) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool PartyPopup::setup() {
    setTitle("GD Party");

    m_statusLabel = CCLabelBMFont::create("Checking Globed...", "goldFont.fnt");
    m_statusLabel->setScale(0.48f);
    m_statusLabel->setPosition({215.0f, 230.0f});
    m_mainLayer->addChild(m_statusLabel);

    m_modeLabel = CCLabelBMFont::create("Roulette Race", "bigFont.fnt");
    m_modeLabel->setScale(0.6f);
    m_modeLabel->setPosition({215.0f, 202.0f});
    m_mainLayer->addChild(m_modeLabel);

    m_playersLabel = CCLabelBMFont::create("No players yet", "chatFont.fnt");
    m_playersLabel->setScale(0.62f);
    m_playersLabel->setAnchorPoint({0.0f, 1.0f});
    m_playersLabel->setPosition({35.0f, 175.0f});
    m_mainLayer->addChild(m_playersLabel);

    m_targetLabel = CCLabelBMFont::create("Goal: 50%", "bigFont.fnt");
    m_targetLabel->setScale(0.43f);
    m_targetLabel->setPosition({322.0f, 158.0f});
    m_mainLayer->addChild(m_targetLabel);

    m_roundLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_roundLabel->setScale(0.45f);
    m_roundLabel->setPosition({215.0f, 112.0f});
    m_mainLayer->addChild(m_roundLabel);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu);

    m_createButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onCreateParty), "Create", {215.0f, 70.0f});
    m_joinButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onJoinParty), "Join", {215.0f, 70.0f});
    m_readyButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onReady), "Ready", {165.0f, 70.0f});
    m_unreadyButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onUnready), "Unready", {165.0f, 70.0f}, 100.0f, "GJ_button_06.png");
    m_startButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onStart), "Start", {270.0f, 70.0f});
    m_openLevelButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onOpenLevel), "Open Level", {215.0f, 70.0f}, 120.0f);
    m_leaveButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onLeaveParty), "Leave", {368.0f, 34.0f}, 75.0f, "GJ_button_06.png");

    m_targetDownButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onTargetDown), "-", {292.0f, 132.0f}, 36.0f, "GJ_button_05.png");
    m_targetUpButton = makeButton(buttonMenu, this, menu_selector(PartyPopup::onTargetUp), "+", {352.0f, 132.0f}, 36.0f, "GJ_button_05.png");

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
    setVisible(m_targetDownButton, false);
    setVisible(m_targetUpButton, false);
    setVisible(m_openLevelButton, false);
    setVisible(m_leaveButton, false);

    if (!transport || !transport->available()) {
        m_statusLabel->setString("Globed 2.2+ is required");
        m_playersLabel->setString("Install or enable Globed first.");
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

    m_targetLabel->setString(fmt::format("Goal: {}%", state.settings.targetProgress).c_str());
    m_playersLabel->setString(playerListText(manager).c_str());

    if (!state.active) {
        m_statusLabel->setString(transport->isRoomOwner() ? "Your Globed room - ready to host" : "No GD Party found yet");
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_roundLabel->setString(transport->isRoomOwner() ? "Create a party for everyone in this room." : "The room owner must create the party.");
        return;
    }

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        m_statusLabel->setString("Party found in this Globed room");
        m_roundLabel->setString("Join when you're ready.");
        setVisible(m_joinButton, !transport->isRoomOwner());
        return;
    }

    setVisible(m_leaveButton, true);

    if (state.phase == PartyPhase::Lobby) {
        m_statusLabel->setString(manager.isHost() ? "You are hosting" : "Waiting in lobby");
        setVisible(m_readyButton, !localPlayer->ready);
        setVisible(m_unreadyButton, localPlayer->ready);
        setVisible(m_startButton, manager.isHost());
        setVisible(m_targetDownButton, manager.isHost());
        setVisible(m_targetUpButton, manager.isHost());
        m_roundLabel->setString("Everyone readies up, then the host starts.");
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        m_statusLabel->setString("Roulette Race in progress");
        m_roundLabel->setString(fmt::format("Reach {}%  |  Level {}", localPlayer->rouletteTarget, localPlayer->currentLevelId).c_str());
        setVisible(m_openLevelButton, true);
        return;
    }

    if (state.phase == PartyPhase::Results) {
        auto winner = manager.findPlayer(state.winnerAccountId);
        auto winnerName = winner ? winner->name : "Unknown";
        m_statusLabel->setString("Match finished!");
        m_roundLabel->setString(fmt::format("{} wins the roulette!", winnerName).c_str());
        return;
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

void PartyPopup::onTargetDown(CCObject*) {
    auto const target = PartyManager::get().state().settings.targetProgress;
    PartyManager::get().setTargetProgress(target - 5);
    refresh();
}

void PartyPopup::onTargetUp(CCObject*) {
    auto const target = PartyManager::get().state().settings.targetProgress;
    PartyManager::get().setTargetProgress(target + 5);
    refresh();
}

void PartyPopup::onOpenLevel(CCObject*) {
    auto player = PartyManager::get().localPlayer();
    if (!player || player->currentLevelId <= 0) {
        return;
    }

    auto searchObject = GJSearchObject::create(SearchType::Search, std::to_string(player->currentLevelId));
    auto scene = LevelBrowserLayer::scene(searchObject);
    CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.35f, scene));
}

void PartyPopup::onLeaveParty(CCObject*) {
    PartyManager::get().leaveParty();
    refresh();
}

} // namespace gdparty
