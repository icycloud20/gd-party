#pragma once

#include "GameMode.hpp"

#include <cstdint>

namespace gdparty {

class RouletteRace final : public GameMode {
public:
    std::string_view name() const override;
    void begin(PartyState& partyState) override;
    void onPlayerProgress(PartyState& partyState, int accountId, int progress) override;

    static int levelForTarget(std::uint64_t seed, int target);
    static void initializePlayer(PartyPlayer& player, std::uint64_t seed);
    static bool advancePlayer(PartyPlayer& player, MatchSettings const& settings);
};

} // namespace gdparty
