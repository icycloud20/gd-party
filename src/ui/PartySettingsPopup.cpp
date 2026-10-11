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

CCLabelBMFont* addSettingLabel(CCNode* parent, char const* text, float y) {
    auto label = partyui::addLabel(parent, text, "bigFont.fnt", 0.36f, {28.0f, y});
    label->limitLabelWidth(145.0f, 0.36f, 0.27f);
    return label;
}

void addDivider(CCNode* parent, float y) {
    auto divider = CCLayerColor::create(ccc4(75, 32, 15, 90), 296.0f, 1.0f);
    divider->setPosition({22.0f, y});
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
    if (!geode::Popup::init(CCSize{340.0f, 220.0f})) {
        return false;
    }

    setTitle("Party Settings", "goldFont.fnt", 0.72f, 20.0f);

    auto rulesTitle = partyui::addLabel(
        m_mainLayer,
        "MATCH RULES",
        "goldFont.fnt",
        0.48f,
        {170.0f, 176.0f},
        {0.5f, 0.5f}
    );
    rulesTitle->setOpacity(245);

    auto rulesPanel = CCScale9Sprite::create(
        "square02_001.png",
        CCRect{0.0f, 0.0f, 80.0f, 80.0f}
    );
    rulesPanel->setContentSize({306.0f, 154.0f});
    rulesPanel->setPosition({170.0f, 91.0f});
    rulesPanel->setOpacity(80);
    m_mainLayer->addChild(rulesPanel, -1);

    constexpr std::array<float, 5> Rows = {145.0f, 116.0f, 87.0f, 58.0f, 29.0f};
    for (std::size_t index = 0; index + 1 < Rows.size(); ++index) {
        addDivider(m_mainLayer, Rows[index] - 14.5f);
    }

    addSettingLabel(m_mainLayer, "Mode", Rows[0]);
    m_modeValue = partyui::addLabel(
        m_mainLayer,
        "Roulette Race",
        "goldFont.fnt",
        0.34f,
        {259.0f, Rows[0]},
        {0.5f, 0.5f}
    );
    m_modeValue->limitLabelWidth(82.0f, 0.34f, 0.24f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModePrevious),
        false,
        {205.0f, Rows[0]},
        0.48f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onModeNext),
        true,
        {313.0f, Rows[0]},
        0.48f
    );

    addSettingLabel(m_mainLayer, "Goal", Rows[1]);
    m_goalValue = partyui::addLabel(
        m_mainLayer,
        "50%",
        "goldFont.fnt",
        0.42f,
        {259.0f, Rows[1]},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalDown),
        "-",
        {213.0f, Rows[1]},
        31.0f,
        "GJ_button_05.png",
        0.68f
    );
    partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onGoalUp),
        "+",
        {305.0f, Rows[1]},
        31.0f,
        "GJ_button_05.png",
        0.63f
    );

    addSettingLabel(m_mainLayer, "Difficulty", Rows[2]);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({229.0f, Rows[2]});
    m_mainLayer->addChild(m_difficultyIconNode, 5);

    m_difficultyValue = partyui::addLabel(
        m_mainLayer,
        "Random",
        "bigFont.fnt",
        0.33f,
        {274.0f, Rows[2]},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(72.0f, 0.33f, 0.23f);
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyPrevious),
        false,
        {190.0f, Rows[2]},
        0.45f
    );
    partyui::addArrowButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onDifficultyNext),
        true,
        {316.0f, Rows[2]},
        0.45f
    );

    m_fourthRowLabel = addSettingLabel(m_mainLayer, "Count extra progress", Rows[3]);
    m_fourthRowLabel->limitLabelWidth(178.0f, 0.36f, 0.25f);

    m_overrunToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {302.0f, Rows[3]},
        0.67f
    );

    m_attemptsValue = partyui::addLabel(
        m_mainLayer,
        "1",
        "goldFont.fnt",
        0.42f,
        {259.0f, Rows[3]},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {213.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.68f
    );
    m_attemptsUpButton = partyui::addTextButton(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {305.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.63f
    );

    addSettingLabel(m_mainLayer, "Practice mode", Rows[4]);
    m_practiceToggle = partyui::addToggle(
        m_buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {302.0f, Rows[4]},
        0.67f
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
    m_difficultyValue->limitLabelWidth(72.0f, 0.33f, 0.22f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.38f)) {
        icon->setPosition({0.0f, 0.0f});
        m_difficultyIconNode->addChild(icon);
    }

    m_practiceToggle->toggle(settings.practiceModeEnabled);

    if (recentRush) {
        m_fourthRowLabel->setString("Attempts per level");
        m_overrunToggle->setVisible(false);
        m_attemptsValue->setVisible(true);
        m_attemptsDownButton->setVisible(true);
        m_attemptsUpButton->setVisible(true);
        m_attemptsValue->setString(fmt::format("{}", settings.maxAttempts).c_str());
    } else {
        m_fourthRowLabel->setString("Count extra progress");
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
