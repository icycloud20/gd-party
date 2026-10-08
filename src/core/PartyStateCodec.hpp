#pragma once

#include "PartyTypes.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gdparty {

std::vector<std::uint8_t> encodePartyState(PartyState const& state);
bool decodePartyState(std::span<const std::uint8_t> bytes, PartyState& state);

} // namespace gdparty
