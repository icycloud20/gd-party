#include "PartyBaseLayer.hpp"

using namespace geode::prelude;

namespace gdparty {

bool PartyBaseLayer::initBase(ccColor3B backgroundColor) {
    if (!CCLayer::init()) {
        return false;
    }

    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    PlatformToolbox::showCursor();

    auto windowSize = CCDirector::sharedDirector()->getWinSize();

    m_background = CCSprite::create("GJ_gradientBG.png");
    if (m_background) {
        auto textureSize = m_background->getTextureRect().size;
        m_background->setAnchorPoint({0.0f, 0.0f});
        m_background->setPosition({-5.0f, -5.0f});
        m_background->setScaleX((windowSize.width + 10.0f) / textureSize.width);
        m_background->setScaleY((windowSize.height + 10.0f) / textureSize.height);
        m_background->setColor(backgroundColor);
        m_background->setZOrder(-10);
        addChild(m_background);
    }

    auto arrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    if (arrow) {
        arrow->setScale(0.9f);
        m_backButton = CCMenuItemSpriteExtra::create(
            arrow,
            this,
            menu_selector(PartyBaseLayer::keyBackClicked)
        );

        m_backMenu = CCMenu::create();
        m_backMenu->setPosition({0.0f, 0.0f});
        m_backMenu->addChild(m_backButton);
        m_backButton->setPosition({27.0f, windowSize.height - 27.0f});
        m_backMenu->setZOrder(50);
        addChild(m_backMenu);
    }

    return true;
}

void PartyBaseLayer::switchTo() {
    PlatformToolbox::showCursor();

    auto scene = CCScene::create();
    scene->addChild(this);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.18f, scene));
}

void PartyBaseLayer::keyBackClicked() {
    PlatformToolbox::showCursor();
    CCDirector::sharedDirector()->popSceneWithTransition(0.18f, PopTransition::kPopTransitionFade);
}

} // namespace gdparty
