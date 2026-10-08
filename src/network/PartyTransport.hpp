#pragma once

#include "PartyMessage.hpp"

#include <functional>
#include <string>

namespace gdparty {

using PartyMessageHandler = std::function<void(PartyMessage const&)>;

class PartyTransport {
public:
    virtual ~PartyTransport() = default;

    virtual bool available() const = 0;
    virtual bool connected() const = 0;
    virtual bool inRoom() const = 0;
    virtual bool isRoomOwner() const = 0;
    virtual std::uint32_t roomId() const = 0;
    virtual int roomOwnerAccountId() const = 0;
    virtual int localAccountId() const = 0;
    virtual std::string localUsername() const = 0;

    virtual void sendReliable(PartyMessage const& message) = 0;
    virtual void sendReliableTo(PartyMessage const& message, int accountId) = 0;
    virtual void setMessageHandler(PartyMessageHandler handler) = 0;
};

} // namespace gdparty
