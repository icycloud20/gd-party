#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace gdparty {

class PartySettingsPopup : public geode::Popup {
protected:
    bool init() override;

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
    void onRecentPoolDown(cocos2d::CCObject* sender);
    void onRecentPoolUp(cocos2d::CCObject* sender);

    cocos2d::CCLabelBMFont* m_modeValue = nullptr;
    cocos2d::CCLabelBMFont* m_goalValue = nullptr;
    cocos2d::CCLabelBMFont* m_difficultyValue = nullptr;
    cocos2d::CCLabelBMFont* m_overrunValue = nullptr;
    cocos2d::CCLabelBMFont* m_practiceValue = nullptr;
    cocos2d::CCLabelBMFont* m_attemptsValue = nullptr;
    cocos2d::CCLabelBMFont* m_recentPoolValue = nullptr;

    cocos2d::CCNode* m_overrunRow = nullptr;
    cocos2d::CCNode* m_attemptsRow = nullptr;
    cocos2d::CCNode* m_recentPoolRow = nullptr;

public:
    static PartySettingsPopup* create();
};

} // namespace gdparty
