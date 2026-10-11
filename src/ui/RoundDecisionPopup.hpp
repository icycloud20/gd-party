#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace gdparty {

class RoundDecisionPopup : public geode::Popup {
protected:
    bool init(int achievedProgress);
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) override;

private:
    void onNextLevel(cocos2d::CCObject* sender);
    void onLeaderboard(cocos2d::CCObject* sender);

    int m_achievedProgress = 0;
    CCMenuItemSpriteExtra* m_nextButton = nullptr;

public:
    static RoundDecisionPopup* create(int achievedProgress);
};

} // namespace gdparty
