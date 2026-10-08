#include "PartyMessage.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>

namespace gdparty {
namespace {

constexpr std::uint8_t ProtocolVersion = 1;

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

void writeBytes(std::vector<std::uint8_t>& output, std::span<const std::uint8_t> bytes) {
    auto size = static_cast<std::uint16_t>(std::min<std::size_t>(bytes.size(), std::numeric_limits<std::uint16_t>::max()));
    writeInteger(output, size);
    output.insert(output.end(), bytes.begin(), bytes.begin() + size);
}

bool readBytes(std::span<const std::uint8_t> bytes, std::size_t& offset, std::vector<std::uint8_t>& output) {
    std::uint16_t size = 0;
    if (!readInteger(bytes, offset, size) || offset + size > bytes.size()) {
        return false;
    }

    output.assign(bytes.begin() + offset, bytes.begin() + offset + size);
    offset += size;
    return true;
}

} // namespace

std::vector<std::uint8_t> encodePartyMessage(PartyMessage const& message) {
    std::vector<std::uint8_t> output;
    output.reserve(32 + message.text.size() + message.data.size());

    output.push_back(ProtocolVersion);
    output.push_back(static_cast<std::uint8_t>(message.type));
    writeInteger(output, message.roomId);
    writeInteger(output, message.senderAccountId);
    writeInteger(output, message.sequence);
    writeInteger(output, message.valueA);
    writeInteger(output, message.valueB);
    writeInteger(output, message.valueC);

    std::vector<std::uint8_t> textBytes(message.text.begin(), message.text.end());
    writeBytes(output, textBytes);
    writeBytes(output, message.data);

    return output;
}

bool decodePartyMessage(std::span<const std::uint8_t> bytes, PartyMessage& message) {
    if (bytes.size() < 2 || bytes[0] != ProtocolVersion) {
        return false;
    }

    std::size_t offset = 2;
    message.type = static_cast<PartyMessageType>(bytes[1]);

    if (!readInteger(bytes, offset, message.roomId) ||
        !readInteger(bytes, offset, message.senderAccountId) ||
        !readInteger(bytes, offset, message.sequence) ||
        !readInteger(bytes, offset, message.valueA) ||
        !readInteger(bytes, offset, message.valueB) ||
        !readInteger(bytes, offset, message.valueC)) {
        return false;
    }

    std::vector<std::uint8_t> textBytes;
    if (!readBytes(bytes, offset, textBytes) || !readBytes(bytes, offset, message.data)) {
        return false;
    }

    message.text.assign(textBytes.begin(), textBytes.end());
    return offset == bytes.size();
}

} // namespace gdparty
