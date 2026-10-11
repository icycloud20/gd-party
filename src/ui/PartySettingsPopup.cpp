#include "PartySettingsPopup.hpp"

#include "../core/PartyManager.hpp"

#include <array>

using namespace geode::prelude;

namespace gdparty {
namespace {

constexpr std::array<DifficultyFilter, 13> DifficultyValues = {
    DifficultyFilter::RandomNonDemon,
    DifficultyFilter::RandomDemon,
    DifficultyFilter::RandomAllExceptAuto,
    DifficultyFilter::Easy,
    DifficultyFilter::Normal,
    DifficultyFilter::Hard,
    DifficultyFilter::Harder,
    DifficultyFilter::Insane,
    DifficultyFilter::EasyDemon,
    DifficultyFilter::MediumDemon,
    DifficultyFilter::HardDemon,
    DifficultyFilter::InsaneDemon,
    DifficultyFilter::ExtremeDemon
};

char const* difficultyName(DifficultyFilter difficulty) {
    switch (difficulty) {
        case DifficultyFilter::RandomNonDemon: return "Random non-demon";
        case DifficultyFilter::RandomDemon: return "Random demon";
        case DifficultyFilter::RandomAllExceptAuto: return "Random except auto";
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
        default: return "Random except auto";
    }
}

DifficultyFilter cycleDifficulty(DifficultyFilter current, int direction) {
    auto index = 0;
    for (std::size_t currentIndex = 0; currentIndex < DifficultyValues.size(); ++currentIndex) {
        if (DifficultyValues[currentIndex] == current) {
            index = static_cast<int>(currentIndex);
            break;
        }
    }

    auto count = static_cast<int>(DifficultyValues.size());
    index = (index + direction + count) % count;
    return DifficultyValues[static_cast<std::size_t>(index)];
}

CCMenuItemSpriteExtra* makeSmallButton(
    CCNode* parent,
    CCObject* target,
    SEL_MenuHandler callback,
    char const* text,
    CCPoint position,
    float width = 34.0f
) {
    auto sprite = ButtonSprite::create(text, width, true, "bigFont.fnt", "GJ_button_05.png", 30.0f, 0.55f);
    auto button = CCMenuItemSpriteExtra::create(sprite, target, callback);
    button->setPosition(position);
    parent->addChild(button);
    return button;
}

CCLabelBMFont* addRowLabel(CCNode* parent, char const* text, float y) {
    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(0.38f);
    label->setAnchorPoint({0.0f, 0.5f});
    label->setPosition({35.0f, y});
    parent->addChild(label);
    return label;
}

CCLabelBMFont* addValueLabel(CCNode* parent, char const* text, float y) {
    auto label = CCLabelBMFont::create(text, "goldFont.fnt");
    label->setScale(0.38f);
    label->setPosition({285.0f, y});
    parent->addChild(label);
    return label;
}

} // namespace

PartySettingsPopup* PartySettingsPopup::create() {
    auto popup = new PartySettingsPopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }

    CC_SAFE_DELETE(popup);
    return nullptr;
}

bool PartySettingsPopup::init() {
    if (!Popup::init(430.0f, 300.0f, "GJ_square01.png")) {
        return false;
    }

    setTitle("Party Settings");

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu);

    addRowLabel(m_mainLayer, "Mode", 245.0f);
    m_modeValue = addValueLabel(m_mainLayer, "Roulette Race", 245.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModePrevious), "<", {205.0f, 245.0f});
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModeNext), ">", {365.0f, 245.0f});

    addRowLabel(m_mainLayer, "Goal", 205.0f);
    m_goalValue = addValueLabel(m_mainLayer, "50%", 205.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalDown), "-", {235.0f, 205.0f});
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalUp), "+", {335.0f, 205.0f});

    addRowLabel(m_mainLayer, "Level difficulty", 165.0f);
    m_difficultyValue = addValueLabel(m_mainLayer, "Random except auto", 165.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyPrevious), "<", {205.0f, 165.0f});
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyNext), ">", {365.0f, 165.0f});

    m_overrunRow = CCNode::create();
    m_overrunRow->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(m_overrunRow);
    addRowLabel(m_overrunRow, "Count passed %", 125.0f);
    m_overrunValue = addValueLabel(m_overrunRow, "ON", 125.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onToggleOverrun), "Toggle", {365.0f, 125.0f}, 62.0f);

    addRowLabel(m_mainLayer, "Practice mode", 85.0f);
    m_practiceValue = addValueLabel(m_mainLayer, "OFF", 85.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onTogglePractice), "Toggle", {365.0f, 85.0f}, 62.0f);

    m_attemptsRow = CCNode::create();
    m_attemptsRow->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(m_attemptsRow);
    addRowLabel(m_attemptsRow, "Max attempts", 125.0f);
    m_attemptsValue = addValueLabel(m_attemptsRow, "1", 125.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onAttemptsDown), "-", {235.0f, 125.0f});
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onAttemptsUp), "+", {335.0f, 125.0f});

    m_recentPoolRow = CCNode::create();
    m_recentPoolRow->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(m_recentPoolRow);
    addRowLabel(m_recentPoolRow, "Recent pool", 45.0f);
    m_recentPoolValue = addValueLabel(m_recentPoolRow, "Latest 100", 45.0f);
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onRecentPoolDown), "-", {235.0f, 45.0f});
    makeSmallButton(buttonMenu, this, menu_selector(PartySettingsPopup::onRecentPoolUp), "+", {335.0f, 45.0f});

    schedule(schedule_selector(PartySettingsPopup::tick), 0.2f);
    refresh();
    return true;
}

void PartySettingsPopup::tick(float) {
    refresh();
}

void PartySettingsPopup::refresh() {
    auto const& settings = PartyManager::get().state().settings;
    auto recentRush = settings.gameMode == GameModeType::RecentRush;

    m_modeValue->setString(recentRush ? "Recent Rush (WIP)" : "Roulette Race");
    m_goalValue->setString(fmt::format("{}%", settings.targetProgress).c_str());
    m_difficultyValue->setString(difficultyName(settings.difficulty));
    m_difficultyValue->limitLabelWidth(125.0f, 0.38f, 0.26f);
    m_overrunValue->setString(settings.overrunCounts ? "ON" : "OFF");
    m_practiceValue->setString(settings.practiceModeEnabled ? "ON" : "OFF");
    m_attemptsValue->setString(fmt::format("{}", settings.maxAttempts).c_str());
    m_recentPoolValue->setString(fmt::format("Latest {}", settings.recentPoolSize).c_str());

    m_overrunRow->setVisible(!recentRush);
    m_attemptsRow->setVisible(recentRush);
    m_recentPoolRow->setVisible(recentRush);
}

void PartySettingsPopup::onModePrevious(CCObject*) {
    auto current = PartyManager::get().state().settings.gameMode;
    PartyManager::get().setGameMode(current == GameModeType::RouletteRace ? GameModeType::RecentRush : GameModeType::RouletteRace);
    refresh();
}

void PartySettingsPopup::onModeNext(CCObject*) {
    onModePrevious(nullptr);
}

void PartySettingsPopup::onGoalDown(CCObject*) {
    auto goal = PartyManager::get().state().settings.targetProgress;
    PartyManager::get().setTargetProgress(goal - 5);
    refresh();
}

void PartySettingsPopup::onGoalUp(CCObject*) {
    auto goal = PartyManager::get().state().settings.targetProgress;
    PartyManager::get().setTargetProgress(goal + 5);
    refresh();
}

void PartySettingsPopup::onDifficultyPrevious(CCObject*) {
    auto current = PartyManager::get().state().settings.difficulty;
    PartyManager::get().setDifficulty(cycleDifficulty(current, -1));
    refresh();
}

void PartySettingsPopup::onDifficultyNext(CCObject*) {
    auto current = PartyManager::get().state().settings.difficulty;
    PartyManager::get().setDifficulty(cycleDifficulty(current, 1));
    refresh();
}

void PartySettingsPopup::onToggleOverrun(CCObject*) {
    auto enabled = PartyManager::get().state().settings.overrunCounts;
    PartyManager::get().setOverrunCounts(!enabled);
    refresh();
}

void PartySettingsPopup::onTogglePractice(CCObject*) {
    auto enabled = PartyManager::get().state().settings.practiceModeEnabled;
    PartyManager::get().setPracticeModeEnabled(!enabled);
    refresh();
}

void PartySettingsPopup::onAttemptsDown(CCObject*) {
    auto attempts = PartyManager::get().state().settings.maxAttempts;
    PartyManager::get().setMaxAttempts(attempts - 1);
    refresh();
}

void PartySettingsPopup::onAttemptsUp(CCObject*) {
    auto attempts = PartyManager::get().state().settings.maxAttempts;
    PartyManager::get().setMaxAttempts(attempts + 1);
    refresh();
}

void PartySettingsPopup::onRecentPoolDown(CCObject*) {
    auto pool = PartyManager::get().state().settings.recentPoolSize;
    PartyManager::get().setRecentPoolSize(pool - 10);
    refresh();
}

void PartySettingsPopup::onRecentPoolUp(CCObject*) {
    auto pool = PartyManager::get().state().settings.recentPoolSize;
    PartyManager::get().setRecentPoolSize(pool + 10);
    refresh();
}

} // namespace gdparty
