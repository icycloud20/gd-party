#pragma once

#include "PartyTypes.hpp"
#include "../network/PartyTransport.hpp"

#include <memory>
#include <optional>

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
    void setTargetProgress(int targetProgress);
    bool startMatch();
    void reportLocalProgress(int progress, int levelId);

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
    void addOrUpdatePlayer(int accountId, std::string const& name);
    PartyPlayer* findMutablePlayer(int accountId);
    PartyMessage baseMessage(PartyMessageType type);

    PartyState m_state;
    std::unique_ptr<PartyTransport> m_transport;
    std::uint32_t m_sequence = 0;
    int m_lastReportedTarget = 0;
};

} // namespace gdparty
