#include "PartyStateCodec.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>

namespace gdparty {
namespace {

constexpr std::uint8_t SnapshotVersion = 1;

template <typename Integer>
void writeInteger(std::vector<std::uint8_t>& output, Integer value) {
    using UnsignedInteger = std::make_unsigned_t<Integer>;
    auto unsignedValue = static_cast<UnsignedInteger>(value);

    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::uint8_t>((unsignedValue >> (index * 8)) & 0xff));
    }
}

template <typename Integer>
bool readInteger(std::span<const std::uint8_t> bytes, std::size_t& offset, Integer& value) {
    if (offset + sizeof(Integer) > bytes.size()) {
        return false;
    }

    using UnsignedInteger = std::make_unsigned_t<Integer>;
    UnsignedInteger unsignedValue = 0;

    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        unsignedValue |= static_cast<UnsignedInteger>(bytes[offset + index]) << (index * 8);
    }

    value = static_cast<Integer>(unsignedValue);
    offset += sizeof(Integer);
    return true;
}

void writeString(std::vector<std::uint8_t>& output, std::string const& value) {
    auto size = static_cast<std::uint16_t>(std::min<std::size_t>(value.size(), std::numeric_limits<std::uint16_t>::max()));
    writeInteger(output, size);
    output.insert(output.end(), value.begin(), value.begin() + size);
}

bool readString(std::span<const std::uint8_t> bytes, std::size_t& offset, std::string& value) {
    std::uint16_t size = 0;
    if (!readInteger(bytes, offset, size) || offset + size > bytes.size()) {
        return false;
    }

    value.assign(reinterpret_cast<char const*>(bytes.data() + offset), size);
    offset += size;
    return true;
}

} // namespace

std::vector<std::uint8_t> encodePartyState(PartyState const& state) {
    std::vector<std::uint8_t> output;
    output.reserve(64 + state.players.size() * 32);

    output.push_back(SnapshotVersion);
    output.push_back(static_cast<std::uint8_t>(state.phase));
    output.push_back(static_cast<std::uint8_t>(state.settings.gameMode));
    output.push_back(state.active ? 1 : 0);

    writeInteger(output, state.roomId);
    writeInteger(output, state.hostAccountId);
    writeInteger(output, state.winnerAccountId);
    writeInteger(output, state.settings.targetProgress);
    writeInteger(output, state.settings.seed);

    auto playerCount = static_cast<std::uint16_t>(std::min<std::size_t>(state.players.size(), std::numeric_limits<std::uint16_t>::max()));
    writeInteger(output, playerCount);

    for (std::size_t index = 0; index < playerCount; ++index) {
        auto const& player = state.players[index];
        writeInteger(output, player.accountId);
        output.push_back(player.ready ? 1 : 0);
        output.push_back(player.finished ? 1 : 0);
        writeInteger(output, player.progress);
        writeInteger(output, player.rouletteTarget);
        writeInteger(output, player.currentLevelId);
        writeString(output, player.name);
    }

    return output;
}

bool decodePartyState(std::span<const std::uint8_t> bytes, PartyState& state) {
    if (bytes.size() < 4 || bytes[0] != SnapshotVersion) {
        return false;
    }

    std::size_t offset = 4;
    PartyState decoded;
    decoded.phase = static_cast<PartyPhase>(bytes[1]);
    decoded.settings.gameMode = static_cast<GameModeType>(bytes[2]);
    decoded.active = bytes[3] != 0;

    if (!readInteger(bytes, offset, decoded.roomId) ||
        !readInteger(bytes, offset, decoded.hostAccountId) ||
        !readInteger(bytes, offset, decoded.winnerAccountId) ||
        !readInteger(bytes, offset, decoded.settings.targetProgress) ||
        !readInteger(bytes, offset, decoded.settings.seed)) {
        return false;
    }

    std::uint16_t playerCount = 0;
    if (!readInteger(bytes, offset, playerCount) || playerCount > 64) {
        return false;
    }

    decoded.players.reserve(playerCount);
    for (std::uint16_t index = 0; index < playerCount; ++index) {
        PartyPlayer player;
        if (!readInteger(bytes, offset, player.accountId) || offset + 2 > bytes.size()) {
            return false;
        }

        player.ready = bytes[offset++] != 0;
        player.finished = bytes[offset++] != 0;

        if (!readInteger(bytes, offset, player.progress) ||
            !readInteger(bytes, offset, player.rouletteTarget) ||
            !readInteger(bytes, offset, player.currentLevelId) ||
            !readString(bytes, offset, player.name)) {
            return false;
        }

        decoded.players.push_back(std::move(player));
    }

    if (offset != bytes.size()) {
        return false;
    }

    state = std::move(decoded);
    return true;
}

} // namespace gdparty
