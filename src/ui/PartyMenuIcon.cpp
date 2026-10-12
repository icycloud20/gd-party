#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

#include <algorithm>

using namespace geode::prelude;

class $modify(GDPartyMenuIconPatch, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        scheduleOnce(schedule_selector(GDPartyMenuIconPatch::applyPartyIcon), 0.0f);
        return true;
    }

    void applyPartyIcon(float) {
        auto bottomMenu = getChildByID("bottom-menu");
        if (!bottomMenu) {
            return;
        }

        auto partyButton = typeinfo_cast<CCMenuItemSpriteExtra*>(
            bottomMenu->getChildByID("party-button"_spr)
        );
        if (!partyButton) {
            return;
        }

        auto iconPath = Mod::get()->getResourcesDir() / "party-icon.png";
        auto icon = CCSprite::create(iconPath.string().c_str());
        if (!icon) {
            log::warn("GD Party could not load party-icon.png");
            return;
        }

        constexpr float IconSize = 29.0f;
        auto iconSize = icon->getContentSize();
        auto longestSide = std::max(iconSize.width, iconSize.height);
        if (longestSide > 0.0f) {
            icon->setScale(IconSize / longestSide);
        }

        auto buttonSprite = CircleButtonSprite::create(
            icon,
            CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );
        partyButton->setNormalImage(buttonSprite);
    }
};
