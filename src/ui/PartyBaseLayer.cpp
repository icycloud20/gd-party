#include "PartyBaseLayer.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace gdparty {
namespace {

CCSprite* createBackgroundTile() {
    auto sprite = CCSprite::create("game_bg_01_001.png");
    if (!sprite) {
        sprite = CCSprite::createWithSpriteFrameName("game_bg_01_001.png");
    }
    return sprite;
}

} // namespace

bool PartyBaseLayer::initBase(ccColor3B backgroundColor) {
    if (!CCLayer::init()) {
        return false;
    }

    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    PlatformToolbox::showCursor();

    auto windowSize = CCDirector::sharedDirector()->getWinSize();

    // Keep the normal GD gradient underneath as a safe fallback, then layer the
    // same scrolling Geometry Dash background pattern Globed uses on top.
    m_background = CCSprite::create("GJ_gradientBG.png");
    if (m_background) {
        auto textureSize = m_background->getTextureRect().size;
        m_background->setAnchorPoint({0.0f, 0.0f});
        m_background->setPosition({-5.0f, -5.0f});
        m_background->setScaleX((windowSize.width + 10.0f) / textureSize.width);
        m_background->setScaleY((windowSize.height + 10.0f) / textureSize.height);
        m_background->setColor(backgroundColor);
        m_background->setZOrder(-20);
        addChild(m_background);
    }

    if (auto prototype = createBackgroundTile()) {
        auto sourceSize = prototype->getContentSize();
        if (sourceSize.width > 0.0f && sourceSize.height > 0.0f) {
            auto patternLayer = CCNode::create();
            patternLayer->setPosition({0.0f, 0.0f});
            patternLayer->setZOrder(-10);
            addChild(patternLayer);

            auto tileScale = windowSize.height / sourceSize.height;
            auto tileWidth = sourceSize.width * tileScale;
            auto tileCount = std::max(3, static_cast<int>(std::ceil(windowSize.width / tileWidth)) + 3);

            for (int index = -1; index < tileCount; ++index) {
                auto tile = index == -1 ? prototype : createBackgroundTile();
                if (!tile) {
                    continue;
                }

                tile->setAnchorPoint({0.0f, 0.0f});
                tile->setScale(tileScale);
                tile->setPosition({static_cast<float>(index) * tileWidth, 0.0f});
                tile->setColor(backgroundColor);
                patternLayer->addChild(tile);
            }

            auto scroll = CCSequence::create(
                CCMoveBy::create(18.0f, {-tileWidth, 0.0f}),
                CCMoveBy::create(0.001f, {tileWidth, 0.0f}),
                nullptr
            );
            patternLayer->runAction(CCRepeatForever::create(scroll));
        }
    }

    auto arrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    if (arrow) {
        m_backButton = CCMenuItemSpriteExtra::create(
            arrow,
            this,
            menu_selector(PartyBaseLayer::onBack)
        );

        m_backMenu = CCMenu::create();
        m_backMenu->setPosition({0.0f, 0.0f});
        m_backMenu->addChild(m_backButton);
        m_backButton->setPosition({25.0f, windowSize.height - 25.0f});
        m_backMenu->setZOrder(50);
        addChild(m_backMenu);
    }

    return true;
}

void PartyBaseLayer::switchTo() {
    PlatformToolbox::showCursor();

    auto scene = CCScene::create();
    scene->addChild(this);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void PartyBaseLayer::onBack(CCObject*) {
    keyBackClicked();
}

void PartyBaseLayer::keyBackClicked() {
    PlatformToolbox::showCursor();
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f,
        PopTransition::kPopTransitionFade
    );
}

} // namespace gdparty
