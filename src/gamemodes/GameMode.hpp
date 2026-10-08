#pragma once

#include "../core/PartyTypes.hpp"

#include <string_view>

namespace gdparty {

class GameMode {
public:
    virtual ~GameMode() = default;

    virtual std::string_view name() const = 0;
    virtual void begin(PartyState& partyState) = 0;
    virtual void onPlayerProgress(PartyState& partyState, int accountId, int progress) = 0;
};

} // namespace gdparty
