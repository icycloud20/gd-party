#pragma once

#include "PartyBaseLayer.hpp"

#include <Geode/ui/ScrollLayer.hpp>

#include <cstddef>

namespace gdparty {

class PartyPopup : public PartyBaseLayer {
public:
    static PartyPopup* create();

protected:
    bool init();

private:
    void tick(float deltaTime);
    void refresh();
    void rebuildPlayerList();
    void rebuildMatchSummary();

    void onCreateParty(cocos2d::CCObject* sender);
    void onJoinParty(cocos2d::CCObject* sender);
    void onReady(cocos2d::CCObject* sender);
    void onUnready(cocos2d::CCObject* sender);
    void onStart(cocos2d::CCObject* sender);
    void onSettings(cocos2d::CCObject* sender);
    void onLeaderboard(cocos2d::CCObject* sender);
    void onOpenLevel(cocos2d::CCObject* sender);
    void onLeaveParty(cocos2d::CCObject* sender);

    cocos2d::CCNode* m_shell = nullptr;
    geode::ScrollLayer* m_playerScroll = nullptr;
    cocos2d::CCNode* m_playerListLayer = nullptr;
    cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;

    CCMenuItemSpriteExtra* m_createButton = nullptr;
    CCMenuItemSpriteExtra* m_joinButton = nullptr;
    CCMenuItemSpriteExtra* m_readyButton = nullptr;
    CCMenuItemSpriteExtra* m_unreadyButton = nullptr;
    CCMenuItemSpriteExtra* m_startButton = nullptr;
    CCMenuItemSpriteExtra* m_openLevelButton = nullptr;
    CCMenuItemSpriteExtra* m_settingsButton = nullptr;
    CCMenuItemSpriteExtra* m_leaderboardButton = nullptr;
    CCMenuItemSpriteExtra* m_leaveButton = nullptr;

    std::size_t m_lastPlayerCount = static_cast<std::size_t>(-1);
};

} // namespace gdparty
