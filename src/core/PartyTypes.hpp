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
    RecentRush,
    LevelRace,
    ProgressRush,
    Consistency,
    Lives,
    SuddenDeath,
    DemonSprint,
    HigherOrLower,
    Random
};

enum class DifficultyFilter : std::uint8_t {
    RandomNonDemon,
    RandomDemon,
    RandomAllExceptAuto,
    Easy,
    Normal,
    Hard,
    Harder,
    Insane,
    EasyDemon,
    MediumDemon,
    HardDemon,
    InsaneDemon,
    ExtremeDemon
};

struct PartyPlayer {
    int accountId = 0;
    std::string name;
    bool ready = false;
    bool finished = false;
    bool roundSecured = false;
    int progress = 0;
    int rouletteTarget = 1;
    int currentLevelId = 0;
    int attemptCount = 0;

    int cubeIcon = 1;
    int color1 = 0;
    int color2 = 3;
    bool glow = false;
    int glowColor = 0;
};

struct MatchSettings {
    GameModeType gameMode = GameModeType::RouletteRace;
    DifficultyFilter difficulty = DifficultyFilter::RandomAllExceptAuto;
    int targetProgress = 50;
    int maxAttempts = 1;
    int recentPoolSize = 200;
    bool overrunCounts = true;
    bool practiceModeEnabled = false;
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
