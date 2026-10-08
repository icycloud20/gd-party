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

    void onCreateParty(cocos2d::CCObject* sender);
    void onJoinParty(cocos2d::CCObject* sender);
    void onReady(cocos2d::CCObject* sender);
    void onUnready(cocos2d::CCObject* sender);
    void onStart(cocos2d::CCObject* sender);
    void onTargetDown(cocos2d::CCObject* sender);
    void onTargetUp(cocos2d::CCObject* sender);
    void onOpenLevel(cocos2d::CCObject* sender);
    void onLeaveParty(cocos2d::CCObject* sender);

    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_modeLabel = nullptr;
    cocos2d::CCLabelBMFont* m_playersLabel = nullptr;
    cocos2d::CCLabelBMFont* m_targetLabel = nullptr;
    cocos2d::CCLabelBMFont* m_roundLabel = nullptr;

    CCMenuItemSpriteExtra* m_createButton = nullptr;
    CCMenuItemSpriteExtra* m_joinButton = nullptr;
    CCMenuItemSpriteExtra* m_readyButton = nullptr;
    CCMenuItemSpriteExtra* m_unreadyButton = nullptr;
    CCMenuItemSpriteExtra* m_startButton = nullptr;
    CCMenuItemSpriteExtra* m_targetDownButton = nullptr;
    CCMenuItemSpriteExtra* m_targetUpButton = nullptr;
    CCMenuItemSpriteExtra* m_openLevelButton = nullptr;
    CCMenuItemSpriteExtra* m_leaveButton = nullptr;

public:
    static PartyPopup* create();
};

} // namespace gdparty
