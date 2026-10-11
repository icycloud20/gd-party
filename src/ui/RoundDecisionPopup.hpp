#pragma once

#include <Geode/Geode.hpp>

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

class RoundDecisionPopup : public cocos2d::CCLayerColor {
public:
    static RoundDecisionPopup* create(RoundDecisionContext const& context);
    void show();

protected:
    bool init(RoundDecisionContext const& context);
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void keyBackClicked() override;

private:
    void tick(float deltaTime);
    void refresh();
    void closeOverlay();
    void onNextLevel(cocos2d::CCObject* sender);
    void onLeaderboard(cocos2d::CCObject* sender);

    RoundDecisionContext m_context;
    cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
    cocos2d::CCLabelBMFont* m_progressLabel = nullptr;
    cocos2d::CCLabelBMFont* m_detailLabel = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    CCMenuItemSpriteExtra* m_nextButton = nullptr;
    CCMenuItemSpriteExtra* m_leaderboardButton = nullptr;
    bool m_waitingForRouletteAdvance = false;
};

} // namespace gdparty
