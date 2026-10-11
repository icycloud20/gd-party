#include "LeaderboardPopup.hpp"

#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace gdparty {
namespace {

constexpr float SummaryHeight = 40.0f;
constexpr float RowHeight = 36.0f;

char const* modeName(GameModeType mode) {
    return mode == GameModeType::RecentRush ? "Recent Rush" : "Roulette Race";
}

} // namespace

LeaderboardPopup* LeaderboardPopup::create() {
    auto layer = new LeaderboardPopup();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }

    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool LeaderboardPopup::init() {
    if (!initBase({37, 50, 167})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);
    auto shellSize = CCSize{
        std::clamp(windowSize.width * 0.75f, 360.0f, 420.0f),
        290.0f
    };
    auto listSize = CCSize{shellSize.width * 0.8f, 218.0f};
    auto listLeft = (shellSize.width - listSize.width) / 2.0f;
    constexpr float ListBottom = 25.0f;

    m_shell = CCNode::create();
    m_shell->setContentSize(shellSize);
    m_shell->setAnchorPoint({0.5f, 0.5f});
    m_shell->setPosition(center);
    addChild(m_shell, 2);

    auto outer = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    outer->setContentSize(shellSize);
    outer->setPosition({shellSize.width / 2.0f, shellSize.height / 2.0f});
    m_shell->addChild(outer, -5);

    auto title = partyui::addLabel(
        m_shell,
        "LEADERBOARD",
        "goldFont.fnt",
        0.68f,
        {shellSize.width / 2.0f, shellSize.height - 16.0f},
        {0.5f, 0.5f}
    );
    title->limitLabelWidth(320.0f, 0.68f, 0.42f);

    partyui::addBlueListFrame(m_shell, {listLeft, ListBottom}, listSize);

    m_scroll = ScrollLayer::create(listSize);
    m_scroll->setPosition({listLeft, ListBottom});
    m_scroll->setTouchEnabled(true);
    m_scroll->setZOrder(2);
    m_shell->addChild(m_scroll);

    m_contentLayer = m_scroll->m_contentLayer;
    m_contentLayer->setContentSize(listSize);

    m_emptyLabel = partyui::addLabel(
        m_shell,
        "NO PLAYERS",
        "bigFont.fnt",
        0.42f,
        {shellSize.width / 2.0f, ListBottom + listSize.height / 2.0f},
        {0.5f, 0.5f}
    );
    m_emptyLabel->setZOrder(8);

    schedule(schedule_selector(LeaderboardPopup::tick), 0.15f);
    rebuild();
    return true;
}

void LeaderboardPopup::tick(float) {
    PartyManager::get().refreshRoomContext();
    rebuild();
}

void LeaderboardPopup::rebuild() {
    if (!m_scroll || !m_contentLayer || !m_emptyLabel) {
        return;
    }

    auto const& state = PartyManager::get().state();
    auto recentRush = state.settings.gameMode == GameModeType::RecentRush;

    std::vector<PartyPlayer const*> players;
    players.reserve(state.players.size());
    for (auto const& player : state.players) {
        players.push_back(&player);
    }

    std::sort(players.begin(), players.end(), [recentRush](PartyPlayer const* first, PartyPlayer const* second) {
        if (first->finished != second->finished) {
            return first->finished;
        }

        if (recentRush) {
            if (first->progress != second->progress) {
                return first->progress > second->progress;
            }
            if (first->rouletteTarget != second->rouletteTarget) {
                return first->rouletteTarget > second->rouletteTarget;
            }
        } else {
            auto firstScore = std::max(first->progress, std::max(0, first->rouletteTarget - 1));
            auto secondScore = std::max(second->progress, std::max(0, second->rouletteTarget - 1));
            if (firstScore != secondScore) {
                return firstScore > secondScore;
            }
        }

        return first->name < second->name;
    });

    m_emptyLabel->setVisible(players.empty());
    m_scroll->setVisible(!players.empty());
    if (players.empty()) {
        m_emptyLabel->setString("NO PLAYERS");
        return;
    }

    auto listSize = m_scroll->getContentSize();
    auto contentHeight = std::max(
        listSize.height,
        SummaryHeight + static_cast<float>(players.size()) * RowHeight
    );
    auto playerCountChanged = players.size() != m_lastPlayerCount;

    m_contentLayer->removeAllChildren();
    m_contentLayer->setContentSize({listSize.width, contentHeight});

    auto summaryRow = CCLayerColor::create(
        ccc4(0x28, 0x35, 0x77, 255),
        listSize.width,
        SummaryHeight
    );
    summaryRow->setPosition({0.0f, contentHeight - SummaryHeight});
    m_contentLayer->addChild(summaryRow);

    auto modeLabel = partyui::addLabel(
        summaryRow,
        modeName(state.settings.gameMode),
        "bigFont.fnt",
        0.40f,
        {12.0f, SummaryHeight / 2.0f}
    );
    modeLabel->limitLabelWidth(listSize.width * 0.38f, 0.40f, 0.29f);

    auto difficultyX = listSize.width * 0.58f;
    if (auto icon = partyui::makeDifficultyIcon(state.settings.difficulty, 0.44f)) {
        icon->setPosition({difficultyX - 25.0f, SummaryHeight / 2.0f});
        summaryRow->addChild(icon, 3);
    }

    auto difficultyLabel = partyui::addLabel(
        summaryRow,
        partyui::difficultyName(state.settings.difficulty),
        "goldFont.fnt",
        0.35f,
        {difficultyX - 5.0f, SummaryHeight / 2.0f},
        {0.0f, 0.5f}
    );
    difficultyLabel->limitLabelWidth(listSize.width * 0.24f, 0.35f, 0.25f);

    auto goalLabel = partyui::addLabel(
        summaryRow,
        fmt::format("{}%", state.settings.targetProgress),
        "goldFont.fnt",
        0.46f,
        {listSize.width - 10.0f, SummaryHeight / 2.0f},
        {1.0f, 0.5f}
    );
    goalLabel->limitLabelWidth(56.0f, 0.46f, 0.32f);

    auto gameManager = GameManager::sharedState();

    for (std::size_t index = 0; index < players.size(); ++index) {
        auto const& player = *players[index];
        auto rowTop = contentHeight - SummaryHeight - static_cast<float>(index) * RowHeight;
        auto rowBottom = rowTop - RowHeight;
        auto rowColor = index % 2 == 0
            ? ccc4(0x33, 0x44, 0x99, 255)
            : ccc4(0x28, 0x35, 0x77, 255);

        auto row = CCLayerColor::create(rowColor, listSize.width, RowHeight);
        row->setPosition({0.0f, rowBottom});
        m_contentLayer->addChild(row);

        auto rankLabel = partyui::addLabel(
            row,
            fmt::format("{}.", index + 1),
            index == 0 ? "goldFont.fnt" : "bigFont.fnt",
            0.35f,
            {24.0f, RowHeight / 2.0f},
            {1.0f, 0.5f}
        );
        rankLabel->limitLabelWidth(24.0f, 0.35f, 0.25f);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.44f);
        icon->setPosition({45.0f, RowHeight / 2.0f});
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
            {67.0f, RowHeight / 2.0f + 1.0f}
        );
        nameLabel->limitLabelWidth(listSize.width * 0.34f, 0.36f, 0.26f);

        if (player.accountId == state.hostAccountId) {
            auto crown = partyui::makeSizedSprite(partyui::loadGlobedSprite("icon-crown-small.png"), 17.0f);
            if (crown) {
                auto nameWidth = nameLabel->getContentSize().width * nameLabel->getScale();
                crown->setPosition({
                    std::min(listSize.width * 0.57f, 67.0f + nameWidth + 11.0f),
                    RowHeight / 2.0f + 1.0f
                });
                row->addChild(crown, 4);
            }
        }

        std::string status;
        std::string detail;
        char const* statusFont = "bigFont.fnt";

        if (recentRush) {
            if (player.finished) {
                status = "WINNER";
                detail = fmt::format("GOAL {}%", state.settings.targetProgress);
                statusFont = "goldFont.fnt";
            } else {
                status = fmt::format("BEST {}%", player.progress);
                detail = fmt::format(
                    "LEVEL {}  |  TRY {}/{}",
                    player.rouletteTarget,
                    std::min(player.attemptCount + 1, state.settings.maxAttempts),
                    state.settings.maxAttempts
                );
            }
        } else {
            auto score = std::max(player.progress, std::max(0, player.rouletteTarget - 1));
            if (player.finished) {
                status = "WINNER";
                detail = fmt::format("SCORE {}%", state.settings.targetProgress);
                statusFont = "goldFont.fnt";
            } else {
                status = fmt::format("SCORE {}%", score);
                detail = fmt::format(
                    "NEXT {}%  |  THIS LEVEL {}%",
                    player.rouletteTarget,
                    player.progress
                );
            }
        }

        auto statusLabel = partyui::addLabel(
            row,
            status,
            statusFont,
            0.31f,
            {listSize.width - 10.0f, RowHeight / 2.0f + 6.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(listSize.width * 0.32f, 0.31f, 0.22f);

        auto detailLabel = partyui::addLabel(
            row,
            detail,
            "bigFont.fnt",
            0.20f,
            {listSize.width - 10.0f, RowHeight / 2.0f - 7.0f},
            {1.0f, 0.5f}
        );
        detailLabel->setOpacity(190);
        detailLabel->limitLabelWidth(listSize.width * 0.42f, 0.20f, 0.15f);
    }

    if (playerCountChanged) {
        m_scroll->moveToTop();
        m_lastPlayerCount = players.size();
    }
}

} // namespace gdparty
