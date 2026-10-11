#pragma once

#include "PartyTypes.hpp"
#include "../network/PartyTransport.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gdparty {

class PartyManager {
public:
    static PartyManager& get();

    void initialize(std::unique_ptr<PartyTransport> transport);

    PartyState const& state() const;
    PartyTransport const* transport() const;

    bool canUseParty() const;
    void refreshRoomContext();
    bool isHost() const;
    bool createParty();
    void discoverParty();
    bool joinParty();
    void leaveParty();
    void setReady(bool ready);

    void setGameMode(GameModeType gameMode);
    void setTargetProgress(int targetProgress);
    void setDifficulty(DifficultyFilter difficulty);
    void setOverrunCounts(bool enabled);
    void setPracticeModeEnabled(bool enabled);
    void setMaxAttempts(int maxAttempts);
    void setRecentPoolSize(int recentPoolSize);

    bool startMatch();
    void reportLocalProgress(int progress, int levelId);
    void reportAttemptEnded(int progress, int levelId);
    bool requestAdvanceRound(int achievedProgress);

    PartyPlayer const* localPlayer() const;
    PartyPlayer const* findPlayer(int accountId) const;

private:
    PartyManager() = default;

    void resetForCurrentRoom();
    void handleMessage(PartyMessage const& message);
    void handleHostMessage(PartyMessage const& message);
    void broadcastSnapshot();
    void sendSnapshotTo(int accountId);
    void applySnapshot(PartyMessage const& message);
    void addOrUpdatePlayer(
        int accountId,
        std::string const& name,
        int cubeIcon = 1,
        int color1 = 0,
        int color2 = 3,
        bool glow = false,
        int glowColor = 0
    );
    PartyPlayer* findMutablePlayer(int accountId);
    PartyMessage baseMessage(PartyMessageType type);

    void beginRouletteLevelAssignment();
    void assignPlayerToTarget(int accountId, int target);
    void resolveTargetLevel(int target, std::optional<int> levelId);
    std::uint64_t seedForTarget(int target) const;

    PartyState m_state;
    std::unique_ptr<PartyTransport> m_transport;
    std::uint32_t m_sequence = 0;
    int m_lastReportedTarget = 0;
    int m_lastReportedProgress = -1;
    int m_lastReportedLevelId = 0;
    bool m_pendingRoundAdvanceOpen = false;

    std::unordered_map<int, int> m_levelByTarget;
    std::unordered_map<int, std::vector<int>> m_waitingPlayersByTarget;
    std::unordered_set<int> m_targetsLoading;
    std::unordered_set<int> m_usedLevelIds;
};

} // namespace gdparty
