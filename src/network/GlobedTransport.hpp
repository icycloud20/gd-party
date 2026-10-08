#pragma once

#include "PartyTransport.hpp"

namespace gdparty {

class GlobedTransport final : public PartyTransport {
public:
    bool available() const override;
    bool connected() const override;
    bool inRoom() const override;
    bool isRoomOwner() const override;
    std::uint32_t roomId() const override;
    int roomOwnerAccountId() const override;
    int localAccountId() const override;
    std::string localUsername() const override;

    void sendReliable(PartyMessage const& message) override;
    void sendReliableTo(PartyMessage const& message, int accountId) override;
    void setMessageHandler(PartyMessageHandler handler) override;

private:
    void ensureListener();

    PartyMessageHandler m_messageHandler;
    bool m_listenerRegistered = false;
    bool m_apiReady = false;
};

} // namespace gdparty
