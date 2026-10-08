#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gdparty {

enum class PartyPhase : std::uint8_t {
    Idle,
    Lobby,
    Countdown,
    Playing,
    Results
};

enum class GameModeType : std::uint8_t {
    RouletteRace,
    LevelRace,
    ProgressRush,
    Consistency,
    Lives,
    SuddenDeath,
    DemonSprint,
    HigherOrLower,
    Random
};

struct PartyPlayer {
    int accountId = 0;
    std::string name;
    bool ready = false;
    bool finished = false;
    int progress = 0;
    int rouletteTarget = 1;
    int currentLevelId = 0;
};

struct MatchSettings {
    GameModeType gameMode = GameModeType::RouletteRace;
    int targetProgress = 50;
    std::uint64_t seed = 0;
};

struct PartyState {
    PartyPhase phase = PartyPhase::Idle;
    std::uint32_t roomId = 0;
    int localAccountId = 0;
    int hostAccountId = 0;
    int winnerAccountId = 0;
    bool active = false;
    MatchSettings settings;
    std::vector<PartyPlayer> players;
};

} // namespace gdparty
