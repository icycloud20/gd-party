#include "LeaderboardPopup.hpp"

#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace gdparty {

LeaderboardPopup* LeaderboardPopup::create() {
    auto popup = new LeaderboardPopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool LeaderboardPopup::init() {
    if (!Popup::init(380.0f, 250.0f, "GJ_square01.png")) {
        return false;
    }

    PlatformToolbox::showCursor();
    setTitle("Standings");

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

    partyui::addPanel(m_mainLayer, {190.0f, 130.0f}, {330.0f, 168.0f}, 58);

    auto gameManager = GameManager::sharedState();
    auto shown = std::min<std::size_t>(players.size(), 5);

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = *players[index];
        auto y = 194.0f - static_cast<float>(index) * 32.0f;

        if (index > 0) {
            partyui::addDivider(m_mainLayer, 25.0f, y + 16.0f, 330.0f);
        }

        auto rankLabel = partyui::addLabel(
            m_mainLayer,
            fmt::format("{}", index + 1),
            index == 0 ? "goldFont.fnt" : "bigFont.fnt",
            0.34f,
            {43.0f, y},
            {0.5f, 0.5f}
        );
        rankLabel->limitLabelWidth(24.0f, 0.34f, 0.25f);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.36f);
        icon->setPosition({69.0f, y});
        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }
        m_mainLayer->addChild(icon);

        auto nameLabel = partyui::addLabel(
            m_mainLayer,
            player.name,
            "chatFont.fnt",
            0.5f,
            {91.0f, y + 3.0f}
        );
        nameLabel->limitLabelWidth(125.0f, 0.5f, 0.32f);

        std::string mainStatus;
        std::string subStatus;
        if (player.finished) {
            mainStatus = "FINISHED";
            subStatus = "winner";
        } else if (recentRush) {
            mainStatus = fmt::format("Best {}%", player.progress);
            subStatus = fmt::format(
                "Level {}  -  {}/{} attempts",
                player.rouletteTarget,
                player.attemptCount,
                state.settings.maxAttempts
            );
        } else {
            mainStatus = fmt::format("Best {}%", player.progress);
            subStatus = fmt::format("Target {}%", player.rouletteTarget);
        }

        auto statusLabel = partyui::addLabel(
            m_mainLayer,
            mainStatus,
            player.finished ? "goldFont.fnt" : "bigFont.fnt",
            0.32f,
            {350.0f, y + 5.0f},
            {1.0f, 0.5f}
        );
        statusLabel->limitLabelWidth(105.0f, 0.32f, 0.24f);

        auto subLabel = partyui::addLabel(
            m_mainLayer,
            subStatus,
            "chatFont.fnt",
            0.31f,
            {350.0f, y - 7.0f},
            {1.0f, 0.5f}
        );
        subLabel->setOpacity(190);
        subLabel->limitLabelWidth(125.0f, 0.31f, 0.22f);
    }

    if (players.empty()) {
        auto emptyLabel = partyui::addLabel(
            m_mainLayer,
            "No players",
            "chatFont.fnt",
            0.48f,
            {190.0f, 130.0f},
            {0.5f, 0.5f}
        );
        emptyLabel->setOpacity(200);
    } else if (players.size() > shown) {
        auto moreLabel = partyui::addLabel(
            m_mainLayer,
            fmt::format("+{} more players", players.size() - shown),
            "chatFont.fnt",
            0.31f,
            {190.0f, 34.0f},
            {0.5f, 0.5f}
        );
        moreLabel->setOpacity(180);
    }

    return true;
}

} // namespace gdparty
