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

CCLabelBMFont* addSettingLabel(CCNode* parent, char const* text, float y, float maxWidth = 150.0f) {
    auto label = partyui::addLabel(parent, text, "bigFont.fnt", 0.39f, {22.0f, y});
    label->limitLabelWidth(maxWidth, 0.39f, 0.28f);
    return label;
}

CCScale9Sprite* addSectionPanel(CCNode* parent, CCPoint position, CCSize size) {
    auto panel = CCScale9Sprite::create(
        "square02_001.png",
        CCRect{0.0f, 0.0f, 80.0f, 80.0f}
    );
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(68);
    parent->addChild(panel, -1);
    return panel;
}

void addBrownDivider(CCNode* parent, float y) {
    auto divider = CCLayerColor::create(ccc4(79, 37, 20, 105), 260.0f, 1.0f);
    divider->setPosition({20.0f, y});
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
    if (!geode::Popup::init(CCSize{300.0f, 176.0f})) {
        return false;
    }

    setTitle("Party Settings", "goldFont.fnt", 0.64f, 17.0f);

    partyui::addLabel(
        m_mainLayer,
        "MATCH",
        "goldFont.fnt",
        0.40f,
        {150.0f, 136.0f},
        {0.5f, 0.5f}
    );
    partyui::addLabel(
        m_mainLayer,
        "RULES",
        "goldFont.fnt",
        0.40f,
        {150.0f, 76.0f},
        {0.5f, 0.5f}
    );

    addSectionPanel(m_mainLayer, {150.0f, 109.0f}, {270.0f, 48.0f});
    addSectionPanel(m_mainLayer, {150.0f, 38.0f}, {270.0f, 62.0f});

    constexpr float ModeY = 120.0f;
    constexpr float DifficultyY = 98.0f;
    constexpr float GoalY = 58.0f;
    constexpr float FourthY = 37.0f;
    constexpr float PracticeY = 16.0f;

    addBrownDivider(m_mainLayer, 109.0f);
    addBrownDivider(m_mainLayer, 47.5f);
    addBrownDivider(m_mainLayer, 26.5f);

    addSettingLabel(m_mainLayer, "Mode", ModeY);
    m_modeValue = partyui::addLabel(
        m_mainLayer,
        "Roulette Race",
        "goldFont.fnt",
        0.35f,
        {241.0f, ModeY},
        {0.5f, 0.5f}
    );
    m_modeValue->limitLabelWidth(96.0f, 0.35f, 0.27f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModePrevious),
        false,
        {178.0f, ModeY},
        0.36f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModeNext),
        true,
        {288.0f, ModeY},
        0.36f
    );

    addSettingLabel(m_mainLayer, "Difficulty", DifficultyY);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({200.0f, DifficultyY});
    m_mainLayer->addChild(m_difficultyIconNode, 5);

    m_difficultyValue = partyui::addLabel(
        m_mainLayer,
        "Random",
        "bigFont.fnt",
        0.35f,
        {249.0f, DifficultyY},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(82.0f, 0.35f, 0.26f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyPrevious),
        false,
        {174.0f, DifficultyY},
        0.35f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyNext),
        true,
        {289.0f, DifficultyY},
        0.35f
    );

    addSettingLabel(m_mainLayer, "Goal", GoalY);
    m_goalValue = partyui::addLabel(
        m_mainLayer,
        "50%",
        "goldFont.fnt",
        0.40f,
        {255.0f, GoalY},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalDown),
        "-",
        {220.0f, GoalY},
        18.0f,
        "GJ_button_05.png",
        0.38f,
        18.0f
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalUp),
        "+",
        {290.0f, GoalY},
        18.0f,
        "GJ_button_05.png",
        0.36f,
        18.0f
    );

    m_fourthRowLabel = addSettingLabel(m_mainLayer, "Count extra progress", FourthY, 166.0f);

    m_overrunToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {286.0f, FourthY},
        0.50f
    );

    m_attemptsValue = partyui::addLabel(
        m_mainLayer,
        "1",
        "goldFont.fnt",
        0.40f,
        {255.0f, FourthY},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {220.0f, FourthY},
        18.0f,
        "GJ_button_05.png",
        0.38f,
        18.0f
    );
    m_attemptsUpButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {290.0f, FourthY},
        18.0f,
        "GJ_button_05.png",
        0.36f,
        18.0f
    );

    addSettingLabel(m_mainLayer, "Practice mode", PracticeY);
    m_practiceToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {286.0f, PracticeY},
        0.50f
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
    m_modeValue->limitLabelWidth(96.0f, 0.35f, 0.27f);
    m_goalValue->setString(fmt::format("{}%", settings.targetProgress).c_str());

    m_difficultyValue->setString(partyui::difficultyName(settings.difficulty));
    m_difficultyValue->limitLabelWidth(82.0f, 0.35f, 0.26f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.50f)) {
        icon->setPosition({0.0f, 0.0f});
        m_difficultyIconNode->addChild(icon);
    }

    m_practiceToggle->toggle(settings.practiceModeEnabled);

    if (recentRush) {
        m_fourthRowLabel->setString("Attempts per level");
        m_fourthRowLabel->limitLabelWidth(166.0f, 0.39f, 0.28f);
        m_overrunToggle->setVisible(false);
        m_attemptsValue->setVisible(true);
        m_attemptsDownButton->setVisible(true);
        m_attemptsUpButton->setVisible(true);
        m_attemptsValue->setString(fmt::format("{}", settings.maxAttempts).c_str());
    } else {
        m_fourthRowLabel->setString("Count extra progress");
        m_fourthRowLabel->limitLabelWidth(166.0f, 0.39f, 0.28f);
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
