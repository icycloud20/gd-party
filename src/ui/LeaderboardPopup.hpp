#pragma once

#include "PartyBaseLayer.hpp"

#include <Geode/ui/ScrollLayer.hpp>

#include <cstddef>

namespace gdparty {

class LeaderboardPopup : public PartyBaseLayer {
public:
    static LeaderboardPopup* create();

protected:
    bool init();

private:
    void tick(float deltaTime);
    void rebuild();

    cocos2d::CCNode* m_shell = nullptr;
    geode::ScrollLayer* m_scroll = nullptr;
    cocos2d::CCNode* m_contentLayer = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;
    std::size_t m_lastPlayerCount = static_cast<std::size_t>(-1);
};

} // namespace gdparty
