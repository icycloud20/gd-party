#include "RouletteRace.hpp"

#include <algorithm>
#include <array>

namespace gdparty {
namespace {

// Temporary V0.1 pool used while the dynamic filtered level provider is built.
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
    if (player->progress >= player->rouletteTarget) {
        player->roundSecured = true;
    }
}

int RouletteRace::levelForTarget(std::uint64_t seed, int target) {
    auto mixed = mix(seed ^ (static_cast<std::uint64_t>(target) * 0x9e3779b97f4a7c15ULL));
    return DevelopmentLevelPool[mixed % DevelopmentLevelPool.size()];
}

void RouletteRace::initializePlayer(PartyPlayer& player, std::uint64_t seed) {
    player.finished = false;
    player.roundSecured = false;
    player.progress = 0;
    player.rouletteTarget = 1;
    player.currentLevelId = levelForTarget(seed, player.rouletteTarget);
    player.attemptCount = 0;
}

bool RouletteRace::advancePlayer(PartyPlayer& player, MatchSettings const& settings) {
    if (!player.roundSecured) {
        return false;
    }

    auto achieved = std::clamp(player.progress, 0, 100);
    auto reachedGoal = settings.overrunCounts
        ? achieved >= settings.targetProgress
        : player.rouletteTarget >= settings.targetProgress;

    if (reachedGoal) {
        return true;
    }

    auto nextTarget = player.rouletteTarget + 1;
    if (settings.overrunCounts) {
        nextTarget = std::max(nextTarget, achieved + 1);
    }

    player.rouletteTarget = std::clamp(nextTarget, 1, settings.targetProgress);
    player.progress = 0;
    player.roundSecured = false;
    player.attemptCount = 0;
    player.currentLevelId = levelForTarget(settings.seed, player.rouletteTarget);
    return false;
}

} // namespace gdparty
