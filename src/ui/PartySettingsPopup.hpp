#pragma once

#include "PartyBaseLayer.hpp"

namespace gdparty {

class PartySettingsPopup : public PartyBaseLayer {
public:
    static PartySettingsPopup* create();

protected:
    bool init();

private:
    void tick(float deltaTime);
    void refresh();

    void onModePrevious(cocos2d::CCObject* sender);
    void onModeNext(cocos2d::CCObject* sender);
    void onGoalDown(cocos2d::CCObject* sender);
    void onGoalUp(cocos2d::CCObject* sender);
    void onDifficultyPrevious(cocos2d::CCObject* sender);
    void onDifficultyNext(cocos2d::CCObject* sender);
    void onToggleOverrun(cocos2d::CCObject* sender);
    void onTogglePractice(cocos2d::CCObject* sender);
    void onAttemptsDown(cocos2d::CCObject* sender);
    void onAttemptsUp(cocos2d::CCObject* sender);

    cocos2d::CCNode* m_shell = nullptr;
    cocos2d::CCLabelBMFont* m_modeValue = nullptr;
    cocos2d::CCLabelBMFont* m_goalValue = nullptr;
    cocos2d::CCLabelBMFont* m_difficultyValue = nullptr;
    cocos2d::CCLabelBMFont* m_fourthRowLabel = nullptr;
    cocos2d::CCLabelBMFont* m_attemptsValue = nullptr;
    cocos2d::CCNode* m_difficultyIconNode = nullptr;

    CCMenuItemToggler* m_overrunToggle = nullptr;
    CCMenuItemToggler* m_practiceToggle = nullptr;
    CCMenuItemSpriteExtra* m_attemptsDownButton = nullptr;
    CCMenuItemSpriteExtra* m_attemptsUpButton = nullptr;
};

} // namespace gdparty
