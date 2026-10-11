#include "PartySettingsPopup.hpp"

#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

#include <array>

using namespace geode::prelude;

namespace gdparty {
namespace {

constexpr std::array<DifficultyFilter, 13> DifficultyValues = {
    DifficultyFilter::RandomAllExceptAuto,
    DifficultyFilter::RandomNonDemon,
    DifficultyFilter::RandomDemon,
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

DifficultyFilter cycleDifficulty(DifficultyFilter current, int direction) {
    int index = 0;
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

CCLabelBMFont* addSettingLabel(CCNode* parent, char const* text, float y, float maxWidth = 158.0f) {
    auto label = partyui::addLabel(parent, text, "bigFont.fnt", 0.38f, {25.0f, y});
    label->limitLabelWidth(maxWidth, 0.38f, 0.27f);
    return label;
}

CCScale9Sprite* addSectionPanel(CCNode* parent, CCPoint position, CCSize size) {
    auto panel = CCScale9Sprite::create(
        "square02_001.png",
        CCRect{0.0f, 0.0f, 80.0f, 80.0f}
    );
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(72);
    parent->addChild(panel, -1);
    return panel;
}

void addBrownDivider(CCNode* parent, float y) {
    auto divider = CCLayerColor::create(ccc4(79, 37, 20, 105), 254.0f, 1.0f);
    divider->setPosition({28.0f, y});
    parent->addChild(divider, 2);
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
    if (!geode::Popup::init(CCSize{310.0f, 185.0f})) {
        return false;
    }

    setTitle("Party Settings", "goldFont.fnt", 0.68f, 18.0f);

    partyui::addLabel(
        m_mainLayer,
        "MATCH",
        "goldFont.fnt",
        0.43f,
        {155.0f, 143.0f},
        {0.5f, 0.5f}
    );
    partyui::addLabel(
        m_mainLayer,
        "RULES",
        "goldFont.fnt",
        0.43f,
        {155.0f, 83.0f},
        {0.5f, 0.5f}
    );

    addSectionPanel(m_mainLayer, {155.0f, 116.0f}, {278.0f, 48.0f});
    addSectionPanel(m_mainLayer, {155.0f, 43.0f}, {278.0f, 70.0f});

    constexpr float ModeY = 127.0f;
    constexpr float DifficultyY = 105.0f;
    constexpr float GoalY = 66.0f;
    constexpr float FourthY = 43.0f;
    constexpr float PracticeY = 20.0f;

    addBrownDivider(m_mainLayer, 116.0f);
    addBrownDivider(m_mainLayer, 54.5f);
    addBrownDivider(m_mainLayer, 31.5f);

    addSettingLabel(m_mainLayer, "Mode", ModeY);
    m_modeValue = partyui::addLabel(
        m_mainLayer,
        "Roulette Race",
        "goldFont.fnt",
        0.34f,
        {235.0f, ModeY},
        {0.5f, 0.5f}
    );
    m_modeValue->limitLabelWidth(84.0f, 0.34f, 0.24f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModePrevious),
        false,
        {178.0f, ModeY},
        0.40f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModeNext),
        true,
        {289.0f, ModeY},
        0.40f
    );

    addSettingLabel(m_mainLayer, "Difficulty", DifficultyY);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({211.0f, DifficultyY});
    m_mainLayer->addChild(m_difficultyIconNode, 5);

    m_difficultyValue = partyui::addLabel(
        m_mainLayer,
        "Random",
        "bigFont.fnt",
        0.34f,
        {244.0f, DifficultyY},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(77.0f, 0.34f, 0.24f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyPrevious),
        false,
        {174.0f, DifficultyY},
        0.39f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyNext),
        true,
        {289.0f, DifficultyY},
        0.39f
    );

    addSettingLabel(m_mainLayer, "Goal", GoalY);
    m_goalValue = partyui::addLabel(
        m_mainLayer,
        "50%",
        "goldFont.fnt",
        0.40f,
        {250.0f, GoalY},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalDown),
        "-",
        {211.0f, GoalY},
        24.0f,
        "GJ_button_05.png",
        0.50f,
        24.0f
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalUp),
        "+",
        {288.0f, GoalY},
        24.0f,
        "GJ_button_05.png",
        0.48f,
        24.0f
    );

    m_fourthRowLabel = addSettingLabel(m_mainLayer, "Count extra progress", FourthY, 174.0f);

    m_overrunToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {282.0f, FourthY},
        0.58f
    );

    m_attemptsValue = partyui::addLabel(
        m_mainLayer,
        "1",
        "goldFont.fnt",
        0.40f,
        {250.0f, FourthY},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {211.0f, FourthY},
        24.0f,
        "GJ_button_05.png",
        0.50f,
        24.0f
    );
    m_attemptsUpButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {288.0f, FourthY},
        24.0f,
        "GJ_button_05.png",
        0.48f,
        24.0f
    );

    addSettingLabel(m_mainLayer, "Practice mode", PracticeY);
    m_practiceToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {282.0f, PracticeY},
        0.58f
    );

    schedule(schedule_selector(PartySettingsPopup::tick), 0.15f);
    refresh();
    return true;
}

void PartySettingsPopup::tick(float) {
    refresh();
}

void PartySettingsPopup::refresh() {
    auto const& settings = PartyManager::get().state().settings;
    auto recentRush = settings.gameMode == GameModeType::RecentRush;

    m_modeValue->setString(recentRush ? "Recent Rush" : "Roulette Race");
    m_goalValue->setString(fmt::format("{}%", settings.targetProgress).c_str());

    m_difficultyValue->setString(partyui::difficultyName(settings.difficulty));
    m_difficultyValue->limitLabelWidth(77.0f, 0.34f, 0.23f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.40f)) {
        icon->setPosition({0.0f, 0.0f});
        m_difficultyIconNode->addChild(icon);
    }

    m_practiceToggle->toggle(settings.practiceModeEnabled);

    if (recentRush) {
        m_fourthRowLabel->setString("Attempts per level");
        m_fourthRowLabel->limitLabelWidth(174.0f, 0.38f, 0.27f);
        m_overrunToggle->setVisible(false);
        m_attemptsValue->setVisible(true);
        m_attemptsDownButton->setVisible(true);
        m_attemptsUpButton->setVisible(true);
        m_attemptsValue->setString(fmt::format("{}", settings.maxAttempts).c_str());
    } else {
        m_fourthRowLabel->setString("Count extra progress");
        m_fourthRowLabel->limitLabelWidth(174.0f, 0.38f, 0.27f);
        m_overrunToggle->setVisible(true);
        m_overrunToggle->toggle(settings.overrunCounts);
        m_attemptsValue->setVisible(false);
        m_attemptsDownButton->setVisible(false);
        m_attemptsUpButton->setVisible(false);
    }
}

void PartySettingsPopup::onModePrevious(CCObject*) {
    auto current = PartyManager::get().state().settings.gameMode;
    PartyManager::get().setGameMode(
        current == GameModeType::RouletteRace ? GameModeType::RecentRush : GameModeType::RouletteRace
    );
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

} // namespace gdparty
