#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gdparty {

enum class PartyMessageType : std::uint8_t {
    DiscoverParty = 1,
    JoinRequest = 2,
    PartySnapshot = 3,
    PlayerReady = 4,
    StartMatch = 5,
    ReachedTarget = 6,
    LeaveParty = 7
};

struct PartyMessage {
    PartyMessageType type = PartyMessageType::DiscoverParty;
    std::uint32_t roomId = 0;
    int senderAccountId = 0;
    std::uint32_t sequence = 0;
    int valueA = 0;
    int valueB = 0;
    std::uint64_t valueC = 0;
    std::string text;
    std::vector<std::uint8_t> data;
};

std::vector<std::uint8_t> encodePartyMessage(PartyMessage const& message);
bool decodePartyMessage(std::span<const std::uint8_t> bytes, PartyMessage& message);

} // namespace gdparty
