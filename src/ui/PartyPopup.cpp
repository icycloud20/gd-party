#include "PartyPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartySettingsPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>
#include <array>
#include <vector>

using namespace geode::prelude;

namespace gdparty {
namespace {

char const* modeName(GameModeType mode) {
    return mode == GameModeType::RecentRush ? "Recent Rush" : "Roulette Race";
}

CCScale9Sprite* makePanel(
    CCNode* parent,
    CCPoint position,
    CCSize size,
    char const* texture,
    GLubyte opacity = 255
) {
    auto panel = CCScale9Sprite::create(texture, CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(opacity);
    parent->addChild(panel);
    return panel;
}

CCSprite* makeSideSprite(
    char const* globedTexture,
    char const* fallbackFrame,
    float targetSize
) {
    auto sprite = CCSprite::create(globedTexture);
    if (!sprite) {
        sprite = CCSprite::createWithSpriteFrameName(fallbackFrame);
    }
    if (!sprite) {
        return nullptr;
    }

    auto size = sprite->getContentSize();
    auto largestSide = std::max(size.width, size.height);
    if (largestSide > 0.0f) {
        sprite->setScale(targetSize / largestSide);
    }
    return sprite;
}

CCMenuItemSpriteExtra* makeIconButton(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* globedTexture,
    char const* fallbackFrame,
    CCPoint position,
    float targetSize
) {
    auto sprite = makeSideSprite(globedTexture, fallbackFrame, targetSize);
    if (!sprite) {
        return nullptr;
    }

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
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);

    // This is intentionally the same sizing rule used by Globed's room/player
    // list. The panel stays stable across aspect ratios instead of stretching
    // to nearly the whole screen.
    auto shellSize = CCSize{
        std::clamp(windowSize.width * 0.75f, 360.0f, 420.0f),
        290.0f
    };
    auto listSize = CCSize{shellSize.width * 0.8f, 144.0f};
    auto listLeft = (shellSize.width - listSize.width) / 2.0f;
    auto listRight = listLeft + listSize.width;

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
        "PARTY",
        "goldFont.fnt",
        0.72f,
        {shellSize.width / 2.0f, shellSize.height - 17.0f},
        {0.5f, 0.5f}
    );

    m_statusLabel = partyui::addLabel(
        m_shell,
        "Checking Globed...",
        "chatFont.fnt",
        0.38f,
        {shellSize.width / 2.0f, shellSize.height - 38.0f},
        {0.5f, 0.5f}
    );
    m_statusLabel->setOpacity(180);

    auto summaryCenterY = shellSize.height - 62.0f;
    auto summaryPanel = makePanel(
        m_shell,
        {shellSize.width / 2.0f, summaryCenterY},
        {listSize.width, 28.0f},
        "square02_001.png",
        150
    );
    summaryPanel->setColor({28, 43, 111});

    m_summaryLayer = CCNode::create();
    m_shell->addChild(m_summaryLayer, 3);

    m_modeLabel = partyui::addLabel(
        m_summaryLayer,
        "Roulette Race",
        "bigFont.fnt",
        0.31f,
        {listLeft + 11.0f, summaryCenterY}
    );
    m_modeLabel->limitLabelWidth(listSize.width * 0.39f, 0.31f, 0.23f);

    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({shellSize.width / 2.0f + 8.0f, summaryCenterY});
    m_summaryLayer->addChild(m_difficultyIconNode, 3);

    m_difficultyLabel = partyui::addLabel(
        m_summaryLayer,
        "Random",
        "goldFont.fnt",
        0.24f,
        {shellSize.width / 2.0f + 21.0f, summaryCenterY}
    );
    m_difficultyLabel->limitLabelWidth(listSize.width * 0.25f, 0.24f, 0.18f);

    m_goalLabel = partyui::addLabel(
        m_summaryLayer,
        "50%",
        "goldFont.fnt",
        0.31f,
        {listRight - 10.0f, summaryCenterY},
        {1.0f, 0.5f}
    );

    auto listCenterY = 139.0f;
    auto listPanel = makePanel(
        m_shell,
        {shellSize.width / 2.0f, listCenterY},
        listSize,
        "square02_001.png",
        225
    );
    listPanel->setColor({51, 68, 153});

    m_playerListLayer = CCNode::create();
    m_shell->addChild(m_playerListLayer, 4);

    m_emptyLabel = partyui::addLabel(
        m_shell,
        "No players yet",
        "chatFont.fnt",
        0.44f,
        {shellSize.width / 2.0f, listCenterY},
        {0.5f, 0.5f}
    );
    m_emptyLabel->setOpacity(190);
    m_emptyLabel->limitLabelWidth(listSize.width - 30.0f, 0.44f, 0.28f);

    m_footerLabel = partyui::addLabel(
        m_shell,
        "",
        "chatFont.fnt",
        0.33f,
        {shellSize.width / 2.0f, 52.0f},
        {0.5f, 0.5f}
    );
    m_footerLabel->setOpacity(180);
    m_footerLabel->limitLabelWidth(shellSize.width - 70.0f, 0.33f, 0.23f);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(buttonMenu, 10);

    constexpr float PrimaryY = 25.0f;
    auto centerX = shellSize.width / 2.0f;

    m_createButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onCreateParty),
        "Create Party",
        {centerX, PrimaryY},
        118.0f,
        "GJ_button_01.png",
        0.56f
    );
    m_joinButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onJoinParty),
        "Join Party",
        {centerX, PrimaryY},
        112.0f,
        "GJ_button_01.png",
        0.56f
    );
    m_readyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onReady),
        "Ready",
        {centerX, PrimaryY},
        104.0f,
        "GJ_button_01.png",
        0.62f
    );
    m_unreadyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onUnready),
        "Unready",
        {centerX, PrimaryY},
        104.0f,
        "GJ_button_06.png",
        0.55f
    );
    m_startButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onStart),
        "Start",
        {centerX, PrimaryY},
        104.0f,
        "GJ_button_01.png",
        0.62f
    );
    m_openLevelButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onOpenLevel),
        "Open Level",
        {centerX, PrimaryY},
        118.0f,
        "GJ_button_01.png",
        0.57f
    );

    // Port Globed's responsive side-button sizing directly. We also reuse the
    // actual Globed menu textures when the dependency is installed, with stock
    // GD sprites as a fallback.
    auto sideSpace = (windowSize.width - shellSize.width) / 2.0f;
    auto sideButtonSize = std::min(45.0f, sideSpace * 0.7f);
    auto sideFreeSpace = sideSpace - sideButtonSize;
    auto sideButtonOffset = std::min(16.0f, sideFreeSpace / 2.0f);
    auto rightX = center.x + shellSize.width / 2.0f + sideButtonOffset + sideButtonSize / 2.0f;
    auto leftX = center.x - shellSize.width / 2.0f - sideButtonOffset - sideButtonSize / 2.0f;

    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.0f, 0.0f});
    addChild(sideMenu, 20);

    m_settingsButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onSettings),
        "dankmeme.globed2/settings01.png",
        "GJ_optionsBtn_001.png",
        {rightX, center.y + 45.0f},
        sideButtonSize
    );
    m_leaderboardButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaderboard),
        "dankmeme.globed2/list01.png",
        "GJ_statsBtn_001.png",
        {rightX, center.y - 16.0f},
        sideButtonSize
    );
    m_leaveButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaveParty),
        "dankmeme.globed2/exit01.png",
        "GJ_deleteBtn_001.png",
        {leftX, center.y - 67.0f},
        sideButtonSize
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

    auto layoutPrimaryButtons = [this] {
        if (!m_shell) {
            return;
        }

        std::array<CCMenuItemSpriteExtra*, 6> candidates = {
            m_createButton,
            m_joinButton,
            m_readyButton,
            m_unreadyButton,
            m_startButton,
            m_openLevelButton
        };

        std::vector<CCMenuItemSpriteExtra*> visible;
        for (auto button : candidates) {
            if (button && button->isVisible()) {
                visible.push_back(button);
            }
        }

        auto centerX = m_shell->getContentSize().width / 2.0f;
        if (visible.size() == 1) {
            visible[0]->setPositionX(centerX);
        } else if (visible.size() == 2) {
            visible[0]->setPositionX(centerX - 66.0f);
            visible[1]->setPositionX(centerX + 66.0f);
        } else if (visible.size() == 3) {
            visible[0]->setPositionX(centerX - 112.0f);
            visible[1]->setPositionX(centerX);
            visible[2]->setPositionX(centerX + 112.0f);
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
        layoutPrimaryButtons();
        return;
    }

    if (!transport->connected()) {
        m_statusLabel->setString("Not connected");
        m_emptyLabel->setString("Connect to a Globed server first");
        m_footerLabel->setString("");
        layoutPrimaryButtons();
        return;
    }

    if (!transport->inRoom()) {
        m_statusLabel->setString("No room");
        m_emptyLabel->setString("Join a private Globed room first");
        m_footerLabel->setString("");
        layoutPrimaryButtons();
        return;
    }

    if (!state.active) {
        m_statusLabel->setString(transport->isRoomOwner() ? "Private room ready" : "Waiting for host");
        m_emptyLabel->setString(transport->isRoomOwner() ? "Create a party to begin" : "The room owner has not created a party yet");
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        m_footerLabel->setString("");
        layoutPrimaryButtons();
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
        layoutPrimaryButtons();
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
        layoutPrimaryButtons();
        return;
    }

    if (state.phase == PartyPhase::Countdown) {
        m_statusLabel->setString("Starting match");
        m_footerLabel->setString("Finding the first level...");
        layoutPrimaryButtons();
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
        layoutPrimaryButtons();
        return;
    }

    if (state.phase == PartyPhase::Results) {
        auto winner = manager.findPlayer(state.winnerAccountId);
        m_statusLabel->setString("Match finished");
        m_footerLabel->setString(
            fmt::format("{} wins", winner ? winner->name : "Unknown").c_str()
        );
    }

    layoutPrimaryButtons();
}

void PartyPopup::rebuildMatchSummary() {
    auto const& settings = PartyManager::get().state().settings;

    m_modeLabel->setString(modeName(settings.gameMode));
    m_difficultyLabel->setString(partyui::difficultyName(settings.difficulty));
    m_goalLabel->setString(fmt::format("{}%", settings.targetProgress).c_str());

    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.16f)) {
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
    auto listWidth = shellSize.width * 0.8f;
    auto listLeft = (shellSize.width - listWidth) / 2.0f;
    auto listRight = listLeft + listWidth;
    auto shown = std::min<std::size_t>(state.players.size(), 5);
    auto topY = shellSize.height - 91.0f;
    constexpr float RowHeight = 27.0f;

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = state.players[index];
        auto y = topY - static_cast<float>(index) * RowHeight;

        if (index % 2 == 1) {
            auto rowShade = CCLayerColor::create(ccc4(255, 255, 255, 13), listWidth - 4.0f, RowHeight);
            rowShade->setPosition({listLeft + 2.0f, y - RowHeight / 2.0f});
            m_playerListLayer->addChild(rowShade);
        }

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.31f);
        icon->setPosition({listLeft + 18.0f, y});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        m_playerListLayer->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            m_playerListLayer,
            player.name,
            "bigFont.fnt",
            0.30f,
            {listLeft + 36.0f, y}
        );
        nameLabel->limitLabelWidth(listWidth * 0.39f, 0.30f, 0.21f);

        if (player.accountId == state.hostAccountId) {
            auto hostLabel = partyui::addLabel(
                m_playerListLayer,
                "HOST",
                "goldFont.fnt",
                0.17f,
                {listLeft + listWidth * 0.53f, y}
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
            status = fmt::format("BEST {}%  L{}", player.progress, player.rouletteTarget);
        } else {
            status = fmt::format("BEST {}%  T{}%", player.progress, player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            m_playerListLayer,
            status,
            font,
            0.25f,
            {listRight - 9.0f, y},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(listWidth * 0.37f, 0.25f, 0.18f);
    }

    if (state.players.size() > shown) {
        auto more = partyui::addLabel(
            m_playerListLayer,
            fmt::format("+{} more", state.players.size() - shown),
            "chatFont.fnt",
            0.25f,
            {shellSize.width / 2.0f, topY - static_cast<float>(shown) * RowHeight + 4.0f},
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
