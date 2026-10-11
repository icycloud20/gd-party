#pragma once

#include "PartyBaseLayer.hpp"

namespace gdparty {

class LeaderboardPopup : public PartyBaseLayer {
public:
    static LeaderboardPopup* create();

protected:
    bool init();
};

} // namespace gdparty
