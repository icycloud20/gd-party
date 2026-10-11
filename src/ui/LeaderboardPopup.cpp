#include "LeaderboardPopup.hpp"

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
    if (!Popup::init(370.0f, 250.0f, "GJ_square01.png")) {
        return false;
    }

    setTitle("Leaderboard");

    auto const& state = PartyManager::get().state();
    std::vector<PartyPlayer const*> players;
    players.reserve(state.players.size());

    for (auto const& player : state.players) {
        players.push_back(&player);
    }

    std::sort(players.begin(), players.end(), [](PartyPlayer const* first, PartyPlayer const* second) {
        auto firstScore = std::max(first->progress, std::max(0, first->rouletteTarget - 1));
        auto secondScore = std::max(second->progress, std::max(0, second->rouletteTarget - 1));

        if (first->finished != second->finished) {
            return first->finished;
        }
        if (firstScore != secondScore) {
            return firstScore > secondScore;
        }
        return first->name < second->name;
    });

    auto gameManager = GameManager::sharedState();
    auto shown = std::min<std::size_t>(players.size(), 6);

    for (std::size_t index = 0; index < shown; ++index) {
        auto const& player = *players[index];
        auto y = 195.0f - static_cast<float>(index) * 29.0f;

        auto rankLabel = CCLabelBMFont::create(fmt::format("{}.", index + 1).c_str(), "bigFont.fnt");
        rankLabel->setScale(0.38f);
        rankLabel->setAnchorPoint({1.0f, 0.5f});
        rankLabel->setPosition({42.0f, y});
        m_mainLayer->addChild(rankLabel);

        auto icon = SimplePlayer::create(std::max(1, player.cubeIcon));
        icon->setScale(0.48f);
        icon->setPosition({63.0f, y});

        if (gameManager) {
            icon->m_firstLayer->setColor(gameManager->colorForIdx(player.color1));
            icon->m_secondLayer->setColor(gameManager->colorForIdx(player.color2));
        }

        m_mainLayer->addChild(icon);

        auto nameLabel = CCLabelBMFont::create(player.name.c_str(), "chatFont.fnt");
        nameLabel->setScale(0.56f);
        nameLabel->setAnchorPoint({0.0f, 0.5f});
        nameLabel->setPosition({85.0f, y + 4.0f});
        nameLabel->limitLabelWidth(130.0f, 0.56f, 0.35f);
        m_mainLayer->addChild(nameLabel);

        auto statusText = player.finished
            ? std::string("FINISHED")
            : fmt::format("Best {}%", player.progress);

        auto bestLabel = CCLabelBMFont::create(statusText.c_str(), player.finished ? "goldFont.fnt" : "bigFont.fnt");
        bestLabel->setScale(0.36f);
        bestLabel->setAnchorPoint({1.0f, 0.5f});
        bestLabel->setPosition({338.0f, y + 4.0f});
        m_mainLayer->addChild(bestLabel);

        if (!player.finished) {
            auto targetLabel = CCLabelBMFont::create(
                fmt::format("Target {}%", player.rouletteTarget).c_str(),
                "goldFont.fnt"
            );
            targetLabel->setScale(0.27f);
            targetLabel->setAnchorPoint({1.0f, 0.5f});
            targetLabel->setPosition({338.0f, y - 7.0f});
            m_mainLayer->addChild(targetLabel);
        }
    }

    if (players.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No players", "bigFont.fnt");
        emptyLabel->setScale(0.45f);
        emptyLabel->setPosition({185.0f, 125.0f});
        m_mainLayer->addChild(emptyLabel);
    }

    return true;
}

} // namespace gdparty
