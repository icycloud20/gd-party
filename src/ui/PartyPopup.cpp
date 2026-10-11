#include "PartyPopup.hpp"

#include "LeaderboardPopup.hpp"
#include "PartySettingsPopup.hpp"
#include "PartyUI.hpp"
#include "../core/PartyLevelLoader.hpp"
#include "../core/PartyManager.hpp"

#include <Geode/binding/ListBorders.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <vector>

using namespace geode::prelude;

namespace gdparty {
namespace {

constexpr float ListHeight = 198.0f;
constexpr float SummaryHeight = 34.0f;
constexpr float PlayerRowHeight = 30.0f;

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

CCSprite* loadGlobedSprite(char const* fileName) {
    auto globed = Loader::get()->getInstalledMod("dankmeme.globed2");
    if (!globed) {
        return nullptr;
    }

    auto resources = globed->getResourcesDir();
    std::array<std::filesystem::path, 2> candidates = {
        resources / fileName,
        resources / "menu" / fileName
    };

    for (auto const& path : candidates) {
        if (auto sprite = CCSprite::create(path.string().c_str())) {
            return sprite;
        }
    }

    return nullptr;
}

CCSprite* makeSizedSprite(CCSprite* sprite, float targetSize) {
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

CCSprite* makeSideSprite(char const* globedTexture, char const* fallbackFrame, float targetSize) {
    auto sprite = loadGlobedSprite(globedTexture);
    if (!sprite) {
        sprite = CCSprite::createWithSpriteFrameName(fallbackFrame);
    }
    return makeSizedSprite(sprite, targetSize);
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

void addGlobedListFrame(CCNode* parent, CCPoint bottomLeft, CCSize size) {
    auto background = CCLayerColor::create(ccc4(0x33, 0x44, 0x99, 255), size.width, size.height);
    background->setPosition(bottomLeft);
    parent->addChild(background, 0);

    auto border = ListBorders::create();
    if (border) {
        border->setSpriteFrames("GJ_commentTop2_001.png", "GJ_commentSide2_001.png");
        border->setContentSize({size.width + 4.0f, size.height + 6.0f});
        border->setPosition({bottomLeft.x + size.width / 2.0f, bottomLeft.y + size.height / 2.0f});
        parent->addChild(border, 6);
    }
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

    addGlobedListFrame(m_shell, {listLeft, ListBottom}, listSize);

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
        {centerX, 43.0f},
        106.0f,
        "GJ_button_01.png",
        0.67f
    );
    m_unreadyButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onUnready),
        "Unready",
        {centerX, 43.0f},
        106.0f,
        "GJ_button_06.png",
        0.58f
    );
    m_startButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onStart),
        "Start",
        {centerX, 43.0f},
        106.0f,
        "GJ_button_01.png",
        0.67f
    );
    m_openLevelButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onOpenLevel),
        "Play Level",
        {centerX, 43.0f},
        122.0f,
        "GJ_button_01.png",
        0.61f
    );
    m_leaveButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartyPopup::onLeaveParty),
        "Leave Party",
        {centerX, 14.0f},
        150.0f,
        "GJ_button_06.png",
        0.60f
    );

    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(sideMenu, 20);

    constexpr float SideButtonSize = 30.0f;
    m_settingsButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onSettings),
        "settings01.png",
        "GJ_optionsBtn_001.png",
        {21.0f, 216.0f},
        SideButtonSize
    );
    m_leaderboardButton = makeIconButton(
        sideMenu,
        this,
        menu_selector(PartyPopup::onLeaderboard),
        "list01.png",
        "GJ_statsBtn_001.png",
        {shellSize.width - 21.0f, 216.0f},
        SideButtonSize
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
        auto primaryY = m_leaveButton && m_leaveButton->isVisible() ? 43.0f : 28.0f;

        for (auto button : visible) {
            button->setPositionY(primaryY);
        }

        if (visible.size() == 1) {
            visible[0]->setPositionX(centerX);
        } else if (visible.size() == 2) {
            visible[0]->setPositionX(centerX - 59.0f);
            visible[1]->setPositionX(centerX + 59.0f);
        } else if (visible.size() == 3) {
            visible[0]->setPositionX(centerX - 103.0f);
            visible[1]->setPositionX(centerX);
            visible[2]->setPositionX(centerX + 103.0f);
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
        layoutPrimaryButtons();
        return;
    }

    if (!transport->connected()) {
        m_emptyLabel->setString("CONNECT TO GLOBED FIRST");
        layoutPrimaryButtons();
        return;
    }

    if (!transport->inRoom()) {
        m_emptyLabel->setString("JOIN A PRIVATE ROOM FIRST");
        layoutPrimaryButtons();
        return;
    }

    if (!state.active) {
        m_emptyLabel->setString(
            transport->isRoomOwner() ? "CREATE A PARTY TO BEGIN" : "WAITING FOR THE ROOM OWNER"
        );
        setVisible(m_createButton, transport->isRoomOwner());
        setVisible(m_joinButton, !transport->isRoomOwner());
        layoutPrimaryButtons();
        return;
    }

    auto localPlayer = manager.localPlayer();
    if (!localPlayer) {
        m_emptyLabel->setString("JOIN THE PARTY WHEN READY");
        setVisible(m_joinButton, !transport->isRoomOwner());
        layoutPrimaryButtons();
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
        layoutPrimaryButtons();
        return;
    }

    if (state.phase == PartyPhase::Playing) {
        setVisible(m_openLevelButton, localPlayer->currentLevelId > 0 && !PartyLevelLoader::get().isLoading());
        layoutPrimaryButtons();
        return;
    }

    layoutPrimaryButtons();
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
        0.37f,
        {12.0f, SummaryHeight / 2.0f}
    );
    modeLabel->limitLabelWidth(listSize.width * 0.38f, 0.37f, 0.27f);

    auto difficultyX = listSize.width * 0.56f;
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.30f)) {
        icon->setPosition({difficultyX - 18.0f, SummaryHeight / 2.0f});
        summaryRow->addChild(icon, 3);
    }

    auto difficultyLabel = partyui::addLabel(
        summaryRow,
        partyui::difficultyName(settings.difficulty),
        "goldFont.fnt",
        0.29f,
        {difficultyX, SummaryHeight / 2.0f},
        {0.0f, 0.5f}
    );
    difficultyLabel->limitLabelWidth(listSize.width * 0.24f, 0.29f, 0.21f);

    partyui::addLabel(
        summaryRow,
        fmt::format("{}%", settings.targetProgress),
        "goldFont.fnt",
        0.38f,
        {listSize.width - 10.0f, SummaryHeight / 2.0f},
        {1.0f, 0.5f}
    );

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
        icon->setScale(0.36f);
        icon->setPosition({19.0f, PlayerRowHeight / 2.0f});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        row->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            row,
            player.name,
            "bigFont.fnt",
            0.36f,
            {39.0f, PlayerRowHeight / 2.0f}
        );
        nameLabel->limitLabelWidth(listSize.width * 0.43f, 0.36f, 0.25f);

        if (player.accountId == state.hostAccountId) {
            auto crown = makeSizedSprite(loadGlobedSprite("icon-crown-small.png"), 17.0f);
            if (crown) {
                auto nameWidth = nameLabel->getContentSize().width * nameLabel->getScale();
                crown->setPosition({
                    std::min(listSize.width * 0.57f, 39.0f + nameWidth + 11.0f),
                    PlayerRowHeight / 2.0f + 1.0f
                });
                row->addChild(crown, 4);
            }
        }

        std::string status;
        char const* font = "bigFont.fnt";
        float statusScale = 0.28f;

        if (state.phase == PartyPhase::Lobby) {
            status = player.ready ? "READY" : "WAITING";
            font = player.ready ? "goldFont.fnt" : "bigFont.fnt";
        } else if (player.finished) {
            status = "FINISHED";
            font = "goldFont.fnt";
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
        statusLabel->limitLabelWidth(listSize.width * 0.34f, statusScale, 0.20f);
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
