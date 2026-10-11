#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace gdparty {

struct RoundDecisionContext {
    int achievedProgress = 0;
    int requiredProgress = 0;
    int sourceLevelId = 0;
    int sourceTarget = 0;
    int attemptNumber = 1;
    int maxAttempts = 1;
    bool recentRush = false;
    bool goalReached = false;
};

class RoundDecisionPopup : public geode::Popup {
public:
    static RoundDecisionPopup* create(RoundDecisionContext const& context);

protected:
    bool init(RoundDecisionContext const& context);

private:
    void tick(float deltaTime);
    void refresh();
    void closeOverlay();
    void onNextLevel(cocos2d::CCObject* sender);
    void onLeaderboard(cocos2d::CCObject* sender);

    RoundDecisionContext m_context;
    cocos2d::CCLabelBMFont* m_progressLabel = nullptr;
    cocos2d::CCLabelBMFont* m_attemptLabel = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    CCMenuItemSpriteExtra* m_nextButton = nullptr;
    CCMenuItemSpriteExtra* m_leaderboardButton = nullptr;
    bool m_waitingForRouletteAdvance = false;
};

} // namespace gdparty
