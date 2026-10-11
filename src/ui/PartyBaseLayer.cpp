#include "PartyBaseLayer.hpp"

#include <cmath>

using namespace geode::prelude;

namespace gdparty {
namespace {

CCTexture2D* loadBackgroundTexture(char const* texture) {
    auto fullPath = CCFileUtils::get()->fullPathForFilename(texture, false);
    if (fullPath.empty()) {
        return nullptr;
    }

    auto cacheKey = fmt::format("duckydev.gd-party-bg-{}", texture);
    auto textureCache = CCTextureCache::get();
    auto cachedTexture = static_cast<CCTexture2D*>(textureCache->m_pTextures->objectForKey(cacheKey));
    if (cachedTexture) {
        return cachedTexture;
    }

    auto image = new CCImage();
    image->initWithImageFile(fullPath.c_str());

    auto newTexture = new CCTexture2D();
    if (newTexture->initWithImage(image)) {
        textureCache->m_pTextures->setObject(newTexture, cacheKey);
        newTexture->release();
    } else {
        newTexture->release();
        newTexture = nullptr;
    }

    image->release();
    return newTexture;
}

class PartyRepeatingBackground final : public CCSprite {
public:
    static PartyRepeatingBackground* create(char const* texture, ccColor3B color) {
        auto background = new PartyRepeatingBackground();
        if (background && background->init(texture, color)) {
            background->autorelease();
            return background;
        }

        CC_SAFE_DELETE(background);
        return nullptr;
    }

private:
    CCSize m_visibleSize;
    CCSize m_textureSize;
    float m_speed = 1.0f;

    bool init(char const* texture, ccColor3B color) {
        auto loadedTexture = loadBackgroundTexture(texture);
        if (!loadedTexture || !initWithTexture(loadedTexture)) {
            return false;
        }

        m_visibleSize = CCDirector::sharedDirector()->getWinSize();
        auto backgroundRect = getTextureRect();
        m_textureSize = getContentSize();

        auto minimumWidth = m_visibleSize.width * 3.0f;
        backgroundRect.size.width = 0.0f;
        while (backgroundRect.size.width < minimumWidth) {
            backgroundRect.size.width += m_textureSize.width;
        }

        ccTexParams textureParameters = {
            GL_LINEAR,
            GL_LINEAR,
            GL_REPEAT,
            GL_CLAMP_TO_EDGE
        };
        getTexture()->setTexParameters(&textureParameters);

        setTextureRect(backgroundRect);
        setAnchorPoint({0.0f, 0.0f});
        setPosition({0.0f, 0.0f});
        setColor(color);
        setZOrder(-10);
        scheduleUpdate();
        return true;
    }

    void update(float deltaTime) {
        auto moveByX = deltaTime * m_speed * m_textureSize.width / 12.0f;
        auto loopAfterX = 0.0f;
        while (loopAfterX < m_visibleSize.width) {
            loopAfterX += m_textureSize.width * getScale();
        }

        auto newPosition = getPosition() - CCPoint{moveByX, 0.0f};
        if (std::abs(newPosition.x) > loopAfterX) {
            newPosition.x += m_textureSize.width * getScale();
        }
        setPosition(newPosition);
    }
};

} // namespace

bool PartyBaseLayer::initBase(ccColor3B backgroundColor) {
    if (!CCLayer::init()) {
        return false;
    }

    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    PlatformToolbox::showCursor();

    auto windowSize = CCDirector::sharedDirector()->getWinSize();

    // This intentionally mirrors Globed/Cue's repeating background behavior:
    // native 1x vertical scale, horizontally repeated texture, and no extra
    // gradient/vignette layer sitting above or below it.
    if (auto repeatingBackground = PartyRepeatingBackground::create("game_bg_01_001.png", backgroundColor)) {
        m_background = repeatingBackground;
        addChild(m_background);
    } else {
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
