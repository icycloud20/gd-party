#pragma once

#include "../core/PartyTypes.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/General.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

using namespace geode::prelude;

namespace gdparty::partyui {

inline CCScale9Sprite* addPanel(
    CCNode* parent,
    CCPoint position,
    CCSize size,
    GLubyte opacity = 65
) {
    auto panel = CCScale9Sprite::create("square02_001.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(opacity);
    parent->addChild(panel);
    return panel;
}

inline CCLayerColor* addDivider(CCNode* parent, float x, float y, float width) {
    auto divider = CCLayerColor::create(ccc4(255, 255, 255, 26), width, 1.0f);
    divider->setPosition({x, y});
    parent->addChild(divider);
    return divider;
}

inline CCLabelBMFont* addLabel(
    CCNode* parent,
    std::string const& text,
    char const* font,
    float scale,
    CCPoint position,
    CCPoint anchor = {0.0f, 0.5f}
) {
    auto label = CCLabelBMFont::create(text.c_str(), font);
    label->setScale(scale);
    label->setAnchorPoint(anchor);
    label->setPosition(position);
    parent->addChild(label);
    return label;
}

inline CCMenuItemSpriteExtra* addTextButton(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* text,
    CCPoint position,
    float width = 88.0f,
    char const* texture = "GJ_button_01.png",
    float textScale = 0.55f,
    float height = 28.0f
) {
    auto sprite = ButtonSprite::create(text, width, true, "bigFont.fnt", texture, height, textScale);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

inline CCMenuItemSpriteExtra* addArrowButton(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    bool right,
    CCPoint position,
    float scale = 0.54f
) {
    auto sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    sprite->setFlipX(right);
    sprite->setScale(scale);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

inline CCMenuItemToggler* addToggle(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    CCPoint position,
    float scale = 0.68f
) {
    auto toggle = CCMenuItemToggler::createWithStandardSprites(target, callback, scale);
    toggle->setPosition(position);
    menu->addChild(toggle);
    return toggle;
}

inline CCSprite* loadGlobedSprite(char const* fileName) {
    if (!fileName || !*fileName) {
        return nullptr;
    }

    auto globed = Loader::get()->getInstalledMod("dankmeme.globed2");
    if (!globed) {
        return nullptr;
    }

    auto resources = globed->getResourcesDir();
    std::array<std::filesystem::path, 2> candidates = {
        resources / fileName,
        resources / "menu" / fileName
    };

    for (auto const& path : candidates) {
        if (auto sprite = CCSprite::create(path.string().c_str())) {
            return sprite;
        }
    }

    return nullptr;
}

inline CCSprite* makeSizedSprite(CCSprite* sprite, float targetSize) {
    if (!sprite) {
        return nullptr;
    }

    auto size = sprite->getContentSize();
    auto largestSide = std::max(size.width, size.height);
    if (largestSide > 0.0f) {
        sprite->setScale(targetSize / largestSide);
    }
    return sprite;
}

inline CCSprite* makeGlobedSprite(
    char const* globedTexture,
    char const* fallbackFrame,
    float targetSize
) {
    auto sprite = loadGlobedSprite(globedTexture);
    if (!sprite && fallbackFrame && *fallbackFrame) {
        sprite = CCSprite::createWithSpriteFrameName(fallbackFrame);
    }
    return makeSizedSprite(sprite, targetSize);
}

inline CCMenuItemSpriteExtra* addIconButton(
    CCNode* menu,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* globedTexture,
    char const* fallbackFrame,
    CCPoint position,
    float targetSize
) {
    auto sprite = makeGlobedSprite(globedTexture, fallbackFrame, targetSize);
    if (!sprite) {
        return nullptr;
    }

    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

inline void addBlueListFrame(CCNode* parent, CCPoint bottomLeft, CCSize size) {
    auto background = CCLayerColor::create(ccc4(0x33, 0x44, 0x99, 255), size.width, size.height);
    background->setPosition(bottomLeft);
    parent->addChild(background, 0);

    auto border = ListBorders::create();
    if (!border) {
        return;
    }

    border->setSpriteFrames("GJ_commentTop2_001.png", "GJ_commentSide2_001.png");
    border->setContentSize({size.width + 4.0f, size.height + 6.0f});
    border->setPosition({bottomLeft.x + size.width / 2.0f, bottomLeft.y + size.height / 2.0f});
    parent->addChild(border, 6);
}

inline char const* difficultyName(DifficultyFilter difficulty) {
    switch (difficulty) {
        case DifficultyFilter::RandomNonDemon: return "Random non-demon";
        case DifficultyFilter::RandomDemon: return "Random demon";
        case DifficultyFilter::RandomAllExceptAuto: return "Random";
        case DifficultyFilter::Easy: return "Easy";
        case DifficultyFilter::Normal: return "Normal";
        case DifficultyFilter::Hard: return "Hard";
        case DifficultyFilter::Harder: return "Harder";
        case DifficultyFilter::Insane: return "Insane";
        case DifficultyFilter::EasyDemon: return "Easy Demon";
        case DifficultyFilter::MediumDemon: return "Medium Demon";
        case DifficultyFilter::HardDemon: return "Hard Demon";
        case DifficultyFilter::InsaneDemon: return "Insane Demon";
        case DifficultyFilter::ExtremeDemon: return "Extreme Demon";
        default: return "Random";
    }
}

inline char const* difficultyIconFrame(DifficultyFilter difficulty) {
    switch (difficulty) {
        case DifficultyFilter::Easy: return "difficulty_01_btn_001.png";
        case DifficultyFilter::Normal: return "difficulty_02_btn_001.png";
        case DifficultyFilter::Hard: return "difficulty_03_btn_001.png";
        case DifficultyFilter::Harder: return "difficulty_04_btn_001.png";
        case DifficultyFilter::Insane: return "difficulty_05_btn_001.png";
        case DifficultyFilter::RandomDemon: return "difficulty_06_btn_001.png";
        case DifficultyFilter::HardDemon: return "difficulty_06_btn_001.png";
        case DifficultyFilter::EasyDemon: return "difficulty_07_btn_001.png";
        case DifficultyFilter::MediumDemon: return "difficulty_08_btn_001.png";
        case DifficultyFilter::InsaneDemon: return "difficulty_09_btn_001.png";
        case DifficultyFilter::ExtremeDemon: return "difficulty_10_btn_001.png";
        case DifficultyFilter::RandomNonDemon:
        case DifficultyFilter::RandomAllExceptAuto:
        default:
            return "difficulty_00_btn_001.png";
    }
}

inline CCSprite* makeDifficultyIcon(DifficultyFilter difficulty, float scale = 0.34f) {
    auto icon = CCSprite::createWithSpriteFrameName(difficultyIconFrame(difficulty));
    if (icon) {
        icon->setScale(scale);
    }
    return icon;
}

} // namespace gdparty::partyui
