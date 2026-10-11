#include "LeaderboardPopup.hpp"

#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace gdparty {

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
    if (!initBase({0, 102, 255})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = windowSize / 2.0f;
    auto listSize = CCSize{390.0f, 246.0f};

    partyui::addLabel(
        this,
        "LEADERBOARD",
        "goldFont.fnt",
        0.74f,
        {center.x, windowSize.height - 25.0f},
        {0.5f, 0.5f}
    );

    auto shell = CCNode::create();
    shell->setContentSize(listSize);
    shell->setAnchorPoint({0.5f, 0.5f});
    shell->setPosition({center.x, center.y - 5.0f});
    addChild(shell, 2);

    auto panel = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(listSize);
    panel->setPosition({listSize.width / 2.0f, listSize.height / 2.0f});
    shell->addChild(panel, -5);

    auto inner = CCScale9Sprite::create("square02_001.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    inner->setContentSize({listSize.width - 30.0f, listSize.height - 44.0f});
    inner->setPosition({listSize.width / 2.0f, listSize.height / 2.0f - 7.0f});
    inner->setColor({15, 26, 86});
    inner->setOpacity(110);
    shell->addChild(inner, -2);

    auto const& state = PartyManager::get().state();
    auto recentRush = state.settings.gameMode == GameModeType::RecentRush;

    auto modeLabel = partyui::addLabel(
        shell,
        recentRush ? "RECENT RUSH" : "ROULETTE RACE",
        "bigFont.fnt",
        0.30f,
        {20.0f, listSize.height - 20.0f}
    );
    modeLabel->setOpacity(220);

    auto goalLabel = partyui::addLabel(
        shell,
        fmt::format("GOAL {}%", state.settings.targetProgress),
        "goldFont.fnt",
        0.27f,
        {listSize.width - 20.0f, listSize.height - 20.0f},
        {1.0f, 0.5f}
    );
    goalLabel->setOpacity(230);

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

    auto gameManager = GameManager::sharedState();
    auto shown = std::min<std::size_t>(players.size(), 6);
    auto topY = listSize.height - 55.0f;
    constexpr float RowHeight = 29.0f;

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = *players[index];
        auto y = topY - static_cast<float>(index) * RowHeight;

        if (index % 2 == 1) {
            auto rowShade = CCLayerColor::create(ccc4(255, 255, 255, 11), listSize.width - 38.0f, RowHeight);
            rowShade->setPosition({19.0f, y - RowHeight / 2.0f});
            shell->addChild(rowShade, 0);
        }

        auto rankLabel = partyui::addLabel(
            shell,
            fmt::format("{}.", index + 1),
            index == 0 ? "goldFont.fnt" : "bigFont.fnt",
            0.31f,
            {35.0f, y},
            {1.0f, 0.5f}
        );
        rankLabel->limitLabelWidth(24.0f, 0.31f, 0.23f);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.34f);
        icon->setPosition({53.0f, y});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        shell->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            shell,
            player.name,
            "bigFont.fnt",
            0.30f,
            {72.0f, y + 1.0f}
        );
        nameLabel->limitLabelWidth(126.0f, 0.30f, 0.22f);

        std::string status;
        std::string detail;
        if (player.finished) {
            status = "FINISHED";
            detail = "winner";
        } else if (recentRush) {
            status = fmt::format("BEST {}%", player.progress);
            detail = fmt::format("LEVEL {}  •  {}/{}", player.rouletteTarget, player.attemptCount, state.settings.maxAttempts);
        } else {
            status = fmt::format("BEST {}%", player.progress);
            detail = fmt::format("TARGET {}%", player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            shell,
            status,
            player.finished ? "goldFont.fnt" : "bigFont.fnt",
            0.28f,
            {listSize.width - 25.0f, y + 5.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(110.0f, 0.28f, 0.21f);

        auto detailLabel = partyui::addLabel(
            shell,
            detail,
            "chatFont.fnt",
            0.26f,
            {listSize.width - 25.0f, y - 7.0f},
            {1.0f, 0.5f}
        );
        detailLabel->setOpacity(175);
        detailLabel->limitLabelWidth(145.0f, 0.26f, 0.19f);
    }

    if (players.empty()) {
        auto empty = partyui::addLabel(
            shell,
            "No players",
            "chatFont.fnt",
            0.48f,
            {listSize.width / 2.0f, listSize.height / 2.0f - 5.0f},
            {0.5f, 0.5f}
        );
        empty->setOpacity(180);
    } else if (players.size() > shown) {
        auto more = partyui::addLabel(
            shell,
            fmt::format("+{} more players", players.size() - shown),
            "chatFont.fnt",
            0.28f,
            {listSize.width / 2.0f, 19.0f},
            {0.5f, 0.5f}
        );
        more->setOpacity(150);
    }

    return true;
}

} // namespace gdparty
