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

constexpr float ListHeight = 198.0f;
constexpr float SummaryHeight = 46.0f;
constexpr float PlayerRowHeight = 42.0f;

char const* modeName(GameModeType mode) {
    return mode == GameModeType::RecentRush ? "Recent Rush" : "Roulette Race";
}

CCScale9Sprite* makePanel(CCNode* parent, CCPoint position, CCSize size, char const* texture) {
    auto panel = CCScale9Sprite::create(texture, CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    parent->addChild(panel);
    return panel;
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
    auto shellSize = CCSize{
        std::clamp(windowSize.width * 0.75f, 360.0f, 420.0f),
        290.0f
    };
    auto listSize = CCSize{shellSize.width * 0.8f, ListHeight};
    auto listLeft = (shellSize.width - listSize.width) / 2.0f;
    constexpr float ListBottom = 56.0f;

    m_shell = CCNode::create();
    m_shell->setContentSize(shellSize);
    m_shell->setAnchorPoint({0.5f, 0.5f});
    m_shell->setPosition(center);
    addChild(m_shell, 2);

    makePanel(
        m_shell,
        {shellSize.width / 2.0f, shellSize.height / 2.0f},
        shellSize,
        "GJ_square02.png"
    )->setZOrder(-5);

    m_titleLabel = partyui::addLabel(
        m_shell,
        "PARTY",
        "goldFont.fnt",
        0.70f,
        {shellSize.width / 2.0f, shellSize.height - 16.0f},
        {0.5f, 0.5f}
    );
    m_titleLabel->limitLabelWidth(320.0f, 0.70f, 0.40f);

    partyui::addBlueListFrame(m_shell, {listLeft, ListBottom}, listSize);

    m_playerScroll = ScrollLayer::create(listSize);
    m_playerScroll->setPosition({listLeft, ListBottom});
    m_playerScroll->setTouchEnabled(true);
    m_playerScroll->setZOrder(2);
    m_shell->addChild(m_playerScroll);

    m_playerListLayer = m_playerScroll->m_contentLayer;
    m_playerListLayer->setContentSize(listSize);

    m_emptyLabel = partyui::addLabel(
        m_shell,
        "NO PLAYERS",
        "bigFont.fnt",
        0.42f,
        {shellSize.width / 2.0f, ListBottom + listSize.height / 2.0f},
        {0.5f, 0.5f}
    );
    m_emptyLabel->limitLabelWidth(listSize.width - 34.0f, 0.42f, 0.28f);
    m_emptyLabel->setZOrder(8);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(buttonMenu, 10);

    auto centerX = shellSize.width / 2.0f;

    m_createButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onCreateParty),
        "Create Party",
        {centerX, 28.0f},
        126.0f,
        "GJ_button_01.png",
        0.60f
    );
    m_joinButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onJoinParty),
        "Join Party",
        {centerX, 28.0f},
        120.0f,
        "GJ_button_01.png",
        0.60f
    );
    m_readyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onReady),
        "Ready",
        {centerX, 28.0f},
        90.0f,
        "GJ_button_01.png",
        0.64f
    );
    m_unreadyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onUnready),
        "Unready",
        {centerX, 28.0f},
        90.0f,
        "GJ_button_06.png",
        0.55f
    );
    m_startButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onStart),
        "Start",
        {centerX, 28.0f},
        90.0f,
        "GJ_button_01.png",
        0.64f
    );

    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(sideMenu, 20);

    constexpr float SideButtonSize = 31.0f;
    m_settingsButton = partyui::addIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onSettings),
        "settings01.png",
        "GJ_optionsBtn_001.png",
        {21.0f, 216.0f},
        SideButtonSize
    );
    m_leaderboardButton = partyui::addIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaderboard),
        "list01.png",
        "GJ_statsBtn_001.png",
        {shellSize.width - 21.0f, 216.0f},
        SideButtonSize
    );
    m_openLevelButton = partyui::addIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onOpenLevel),
        "",
        "GJ_playBtn2_001.png",
        {shellSize.width - 21.0f, 176.0f},
        SideButtonSize
    );

    // Use the native high-resolution GD delete button instead of scaling the
    // small Globed exit texture, which looked blurry when enlarged.
    if (auto leaveSprite = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png")) {
        leaveSprite->setScale(0.82f);
        m_leaveButton = CCMenuItemSpriteExtra::create(
            leaveSprite,
            this,
            menu_selector(PartyPopup::onLeaveParty)
        );
        m_leaveButton->setPosition({21.0f, 28.0f});
        sideMenu->addChild(m_leaveButton);
    }

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

    auto layoutLobbyButtons = [this] {
        if (!m_shell) {
            return;
        }

        std::array<CCMenuItemSpriteExtra*, 3> candidates = {
            m_readyButton,
            m_unreadyButton,
            m_startButton
        };

        std::vector<CCMenuItemSpriteExtra*> visible;
        for (auto button : candidates) {
            if (button && button->isVisible()) {
                visible.push_back(button);
            }
        }

        auto centerX = m_shell->getContentSize().width / 2.0f;
        for (auto button : visible) {
            button->setPositionY(28.0f);
        }

        if (visible.size() == 1) {
            visible[0]->setPositionX(centerX);
        } else if (visible.size() == 2) {
            // 120px between centers with 90px buttons leaves a clean 30px gap.
            visible[0]->setPositionX(centerX - 60.0f);
            visible[1]->setPositionX(centerX + 60.0f);
        } else if (visible.size() == 3) {
            visible[0]->setPositionX(centerX - 100.0f);
            visible[1]->setPositionX(centerX);
            visible[2]->setPositionX(centerX + 100.0f);
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
    setVisible(m_playerScroll, false);
    setVisible(m_emptyLabel, true);

    rebuildMatchSummary();

    if (!transport || !transport->available()) {
        m_emptyLabel->setString("GLOBED IS REQUIRED");
        return;
    }

    if (!transport->connected()) {
        m_emptyLabel->setString("CONNECT TO GLOBED FIRST");
        return;
    }

    if (!transport->inRoom()) {
        m_emptyLabel->setString("JOIN A PRIVATE ROOM FIRST");
        return;
    }

    if (!state.active) {
        m_emptyLabel->setString(
            transport->isRoomOwner() ? "CREATE A PARTY TO BEGIN" : "WAITING FOR THE ROOM OWNER"
        );
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        return;
    }

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        m_emptyLabel->setString("JOIN THE PARTY WHEN READY");
        setVisible(m_joinButton, !transport->isRoomOwner());
        return;
    }

    setVisible(m_emptyLabel, false);
    setVisible(m_playerScroll, true);
    setVisible(m_leaveButton, true);
    setVisible(m_leaderboardButton, true);
    rebuildPlayerList();

    if (state.phase == PartyPhase::Lobby) {
        setVisible(m_readyButton, !localPlayer->ready);
        setVisible(m_unreadyButton, localPlayer->ready);
        setVisible(m_startButton, manager.isHost());
        setVisible(m_settingsButton, manager.isHost());
        layoutLobbyButtons();
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        setVisible(m_openLevelButton, localPlayer->currentLevelId > 0 && !PartyLevelLoader::get().isLoading());
        return;
    }
}

void PartyPopup::rebuildMatchSummary() {
    auto const& state = PartyManager::get().state();
    if (!m_titleLabel) {
        return;
    }

    if (state.active && state.phase == PartyPhase::Results) {
        m_titleLabel->setString("RESULTS");
    } else {
        m_titleLabel->setString("PARTY");
    }
}

void PartyPopup::rebuildPlayerList() {
    if (!m_playerListLayer || !m_playerScroll || !m_shell) {
        return;
    }

    auto const& state = PartyManager::get().state();
    auto gameManager = GameManager::sharedState();
    auto listSize = m_playerScroll->getContentSize();
    auto playerCount = state.players.size();
    auto contentHeight = std::max(
        listSize.height,
        SummaryHeight + static_cast<float>(playerCount) * PlayerRowHeight
    );
    auto playerCountChanged = playerCount != m_lastPlayerCount;

    m_playerListLayer->removeAllChildren();
    m_playerListLayer->setContentSize({listSize.width, contentHeight});

    auto summaryRow = CCLayerColor::create(
        ccc4(0x28, 0x35, 0x77, 255),
        listSize.width,
        SummaryHeight
    );
    summaryRow->setPosition({0.0f, contentHeight - SummaryHeight});
    m_playerListLayer->addChild(summaryRow);

    auto const& settings = state.settings;
    auto modeLabel = partyui::addLabel(
        summaryRow,
        modeName(settings.gameMode),
        "bigFont.fnt",
        0.45f,
        {12.0f, SummaryHeight / 2.0f}
    );
    modeLabel->limitLabelWidth(listSize.width * 0.39f, 0.45f, 0.33f);

    auto difficultyX = listSize.width * 0.61f;
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.62f)) {
        icon->setPosition({difficultyX - 34.0f, SummaryHeight / 2.0f});
        summaryRow->addChild(icon, 3);
    }

    auto difficultyLabel = partyui::addLabel(
        summaryRow,
        partyui::difficultyName(settings.difficulty),
        "goldFont.fnt",
        0.42f,
        {difficultyX - 7.0f, SummaryHeight / 2.0f},
        {0.0f, 0.5f}
    );
    difficultyLabel->limitLabelWidth(listSize.width * 0.24f, 0.42f, 0.30f);

    auto goalLabel = partyui::addLabel(
        summaryRow,
        fmt::format("{}%", settings.targetProgress),
        "goldFont.fnt",
        0.54f,
        {listSize.width - 10.0f, SummaryHeight / 2.0f},
        {1.0f, 0.5f}
    );
    goalLabel->limitLabelWidth(62.0f, 0.54f, 0.38f);

    for (std::size_t index = 0; index < playerCount; ++index) {
        auto const& player = state.players[index];
        auto rowTop = contentHeight - SummaryHeight - static_cast<float>(index) * PlayerRowHeight;
        auto rowBottom = rowTop - PlayerRowHeight;
        auto rowColor = index % 2 == 0
            ? ccc4(0x33, 0x44, 0x99, 255)
            : ccc4(0x28, 0x35, 0x77, 255);

        auto row = CCLayerColor::create(rowColor, listSize.width, PlayerRowHeight);
        row->setPosition({0.0f, rowBottom});
        m_playerListLayer->addChild(row);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.64f);
        icon->setPosition({27.0f, PlayerRowHeight / 2.0f});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        row->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            row,
            player.name,
            "bigFont.fnt",
            0.44f,
            {55.0f, PlayerRowHeight / 2.0f}
        );
        nameLabel->limitLabelWidth(listSize.width * 0.41f, 0.44f, 0.32f);

        if (player.accountId == state.hostAccountId) {
            auto crown = partyui::makeSizedSprite(partyui::loadGlobedSprite("icon-crown-small.png"), 21.0f);
            if (crown) {
                auto nameWidth = nameLabel->getContentSize().width * nameLabel->getScale();
                crown->setPosition({
                    std::min(listSize.width * 0.59f, 55.0f + nameWidth + 13.0f),
                    PlayerRowHeight / 2.0f + 1.0f
                });
                row->addChild(crown, 4);
            }
        }

        std::string status;
        char const* font = "bigFont.fnt";
        float statusScale = 0.38f;

        if (state.phase == PartyPhase::Lobby) {
            status = player.ready ? "READY" : "WAITING";
            font = player.ready ? "goldFont.fnt" : "bigFont.fnt";
            statusScale = 0.40f;
        } else if (player.finished) {
            status = "FINISHED";
            font = "goldFont.fnt";
            statusScale = 0.36f;
        } else if (state.settings.gameMode == GameModeType::RecentRush) {
            status = fmt::format("{}%  L{}", player.progress, player.rouletteTarget);
        } else {
            status = fmt::format("{}% / {}%", player.progress, player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            row,
            status,
            font,
            statusScale,
            {listSize.width - 10.0f, PlayerRowHeight / 2.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(listSize.width * 0.34f, statusScale, 0.27f);
    }

    if (playerCountChanged) {
        m_playerScroll->moveToTop();
        m_lastPlayerCount = playerCount;
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
