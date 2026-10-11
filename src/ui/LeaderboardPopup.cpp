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
    if (!initBase({37, 50, 167})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);
    auto shellSize = CCSize{
        std::clamp(windowSize.width * 0.75f, 360.0f, 420.0f),
        290.0f
    };
    auto innerWidth = shellSize.width * 0.8f;
    auto innerLeft = (shellSize.width - innerWidth) / 2.0f;
    auto innerRight = innerLeft + innerWidth;

    auto shell = CCNode::create();
    shell->setContentSize(shellSize);
    shell->setAnchorPoint({0.5f, 0.5f});
    shell->setPosition(center);
    addChild(shell, 2);

    auto outer = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    outer->setContentSize(shellSize);
    outer->setPosition({shellSize.width / 2.0f, shellSize.height / 2.0f});
    shell->addChild(outer, -5);

    partyui::addLabel(
        shell,
        "LEADERBOARD",
        "goldFont.fnt",
        0.68f,
        {shellSize.width / 2.0f, shellSize.height - 18.0f},
        {0.5f, 0.5f}
    );

    auto const& state = PartyManager::get().state();
    auto recentRush = state.settings.gameMode == GameModeType::RecentRush;

    auto modeLabel = partyui::addLabel(
        shell,
        recentRush ? "RECENT RUSH" : "ROULETTE RACE",
        "bigFont.fnt",
        0.28f,
        {innerLeft + 8.0f, shellSize.height - 42.0f}
    );
    modeLabel->setOpacity(215);

    auto goalLabel = partyui::addLabel(
        shell,
        fmt::format("GOAL {}%", state.settings.targetProgress),
        "goldFont.fnt",
        0.27f,
        {innerRight - 8.0f, shellSize.height - 42.0f},
        {1.0f, 0.5f}
    );
    goalLabel->setOpacity(230);

    auto inner = CCScale9Sprite::create("square02_001.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    inner->setContentSize({innerWidth, 214.0f});
    inner->setPosition({shellSize.width / 2.0f, 125.0f});
    inner->setColor({51, 68, 153});
    inner->setOpacity(225);
    shell->addChild(inner, -2);

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
    auto topY = shellSize.height - 72.0f;
    constexpr float RowHeight = 30.0f;

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = *players[index];
        auto y = topY - static_cast<float>(index) * RowHeight;

        if (index % 2 == 1) {
            auto rowShade = CCLayerColor::create(ccc4(255, 255, 255, 12), innerWidth - 4.0f, RowHeight);
            rowShade->setPosition({innerLeft + 2.0f, y - RowHeight / 2.0f});
            shell->addChild(rowShade, 0);
        }

        auto rankLabel = partyui::addLabel(
            shell,
            fmt::format("{}.", index + 1),
            index == 0 ? "goldFont.fnt" : "bigFont.fnt",
            0.29f,
            {innerLeft + 23.0f, y},
            {1.0f, 0.5f}
        );
        rankLabel->limitLabelWidth(22.0f, 0.29f, 0.21f);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.33f);
        icon->setPosition({innerLeft + 42.0f, y});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        shell->addChild(icon, 3);

        auto nameLabel = partyui::addLabel(
            shell,
            player.name,
            "bigFont.fnt",
            0.29f,
            {innerLeft + 61.0f, y + 1.0f}
        );
        nameLabel->limitLabelWidth(innerWidth * 0.39f, 0.29f, 0.21f);

        std::string status;
        std::string detail;
        if (player.finished) {
            status = "FINISHED";
            detail = "winner";
        } else if (recentRush) {
            status = fmt::format("BEST {}%", player.progress);
            detail = fmt::format("LEVEL {}  |  {}/{}", player.rouletteTarget, player.attemptCount, state.settings.maxAttempts);
        } else {
            status = fmt::format("BEST {}%", player.progress);
            detail = fmt::format("TARGET {}%", player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            shell,
            status,
            player.finished ? "goldFont.fnt" : "bigFont.fnt",
            0.26f,
            {innerRight - 10.0f, y + 5.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(innerWidth * 0.31f, 0.26f, 0.19f);

        auto detailLabel = partyui::addLabel(
            shell,
            detail,
            "chatFont.fnt",
            0.24f,
            {innerRight - 10.0f, y - 7.0f},
            {1.0f, 0.5f}
        );
        detailLabel->setOpacity(170);
        detailLabel->limitLabelWidth(innerWidth * 0.39f, 0.24f, 0.17f);
    }

    if (players.empty()) {
        auto empty = partyui::addLabel(
            shell,
            "No players",
            "chatFont.fnt",
            0.46f,
            {shellSize.width / 2.0f, 125.0f},
            {0.5f, 0.5f}
        );
        empty->setOpacity(180);
    } else if (players.size() > shown) {
        auto more = partyui::addLabel(
            shell,
            fmt::format("+{} more players", players.size() - shown),
            "chatFont.fnt",
            0.26f,
            {shellSize.width / 2.0f, 23.0f},
            {0.5f, 0.5f}
        );
        more->setOpacity(150);
    }

    return true;
}

} // namespace gdparty
