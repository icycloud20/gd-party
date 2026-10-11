#pragma once

#include <Geode/Geode.hpp>

namespace gdparty {

class PartyBaseLayer : public cocos2d::CCLayer {
public:
    bool initBase(cocos2d::ccColor3B backgroundColor = {37, 50, 167});
    void switchTo();
    void keyBackClicked() override;

protected:
    void onBack(cocos2d::CCObject* sender);

    cocos2d::CCSprite* m_background = nullptr;
    cocos2d::CCMenu* m_backMenu = nullptr;
    CCMenuItemSpriteExtra* m_backButton = nullptr;
};

} // namespace gdparty
