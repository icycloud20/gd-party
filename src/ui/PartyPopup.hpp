#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace gdparty {

class PartyPopup : public geode::Popup {
protected:
    bool init() override;

private:
    void tick(float deltaTime);
    void refresh();
    void rebuildPlayerList();

    void onCreateParty(cocos2d::CCObject* sender);
    void onJoinParty(cocos2d::CCObject* sender);
    void onReady(cocos2d::CCObject* sender);
    void onUnready(cocos2d::CCObject* sender);
    void onStart(cocos2d::CCObject* sender);
    void onSettings(cocos2d::CCObject* sender);
    void onLeaderboard(cocos2d::CCObject* sender);
    void onOpenLevel(cocos2d::CCObject* sender);
    void onLeaveParty(cocos2d::CCObject* sender);

    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_modeLabel = nullptr;
    cocos2d::CCLabelBMFont* m_playersLabel = nullptr;
    cocos2d::CCLabelBMFont* m_targetLabel = nullptr;
    cocos2d::CCLabelBMFont* m_roundLabel = nullptr;
    cocos2d::CCNode* m_playerListLayer = nullptr;
    cocos2d::CCNode* m_difficultyIconNode = nullptr;

    CCMenuItemSpriteExtra* m_createButton = nullptr;
    CCMenuItemSpriteExtra* m_joinButton = nullptr;
    CCMenuItemSpriteExtra* m_readyButton = nullptr;
    CCMenuItemSpriteExtra* m_unreadyButton = nullptr;
    CCMenuItemSpriteExtra* m_startButton = nullptr;
    CCMenuItemSpriteExtra* m_settingsButton = nullptr;
    CCMenuItemSpriteExtra* m_leaderboardButton = nullptr;
    CCMenuItemSpriteExtra* m_openLevelButton = nullptr;
    CCMenuItemSpriteExtra* m_leaveButton = nullptr;

public:
    static PartyPopup* create();
};

} // namespace gdparty
