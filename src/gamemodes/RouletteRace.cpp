#include "RouletteRace.hpp"

#include <algorithm>
#include <array>

namespace gdparty {
namespace {

// Temporary V0.1 pool used to exercise the complete multiplayer loop while
// the dynamic GD level-query provider is built. These are public online levels.
constexpr std::array<int, 12> DevelopmentLevelPool = {
    13519,
    55520,
    2997354,
    3543219,
    3979721,
    4284013,
    4957691,
    5904109,
    7116121,
    10565740,
    34085027,
    61079355
};

std::uint64_t mix(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

} // namespace

std::string_view RouletteRace::name() const {
    return "Roulette Race";
}

void RouletteRace::begin(PartyState& partyState) {
    partyState.phase = PartyPhase::Playing;
    partyState.winnerAccountId = 0;

    for (auto& player : partyState.players) {
        initializePlayer(player, partyState.settings.seed);
    }
}

void RouletteRace::onPlayerProgress(PartyState& partyState, int accountId, int progress) {
    auto player = std::find_if(
        partyState.players.begin(),
        partyState.players.end(),
        [accountId](PartyPlayer const& partyPlayer) {
            return partyPlayer.accountId == accountId;
        }
    );

    if (player == partyState.players.end() || player->finished) {
        return;
    }

    player->progress = std::max(player->progress, progress);
    if (player->progress < player->rouletteTarget) {
        return;
    }

    if (!advancePlayer(*player, partyState.settings)) {
        return;
    }

    player->finished = true;
    partyState.winnerAccountId = player->accountId;
    partyState.phase = PartyPhase::Results;
}

int RouletteRace::levelForTarget(std::uint64_t seed, int target) {
    auto mixed = mix(seed ^ (static_cast<std::uint64_t>(target) * 0x9e3779b97f4a7c15ULL));
    return DevelopmentLevelPool[mixed % DevelopmentLevelPool.size()];
}

void RouletteRace::initializePlayer(PartyPlayer& player, std::uint64_t seed) {
    player.finished = false;
    player.progress = 0;
    player.rouletteTarget = 1;
    player.currentLevelId = levelForTarget(seed, player.rouletteTarget);
}

bool RouletteRace::advancePlayer(PartyPlayer& player, MatchSettings const& settings) {
    if (player.rouletteTarget >= settings.targetProgress) {
        return true;
    }

    ++player.rouletteTarget;
    player.progress = 0;
    player.currentLevelId = levelForTarget(settings.seed, player.rouletteTarget);
    return false;
}

} // namespace gdparty
