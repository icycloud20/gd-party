#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace gdparty {

class LeaderboardPopup : public geode::Popup {
protected:
    bool init() override;

public:
    static LeaderboardPopup* create();
};

} // namespace gdparty
