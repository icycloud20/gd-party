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

void addSettingName(CCNode* parent, char const* text, float y) {
    partyui::addLabel(parent, text, "bigFont.fnt", 0.36f, {43.0f, y});
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

    partyui::addPanel(m_mainLayer, {215.0f, 147.0f}, {370.0f, 218.0f}, 58);

    auto subtitle = partyui::addLabel(
        m_mainLayer,
        "Match rules",
        "chatFont.fnt",
        0.42f,
        {215.0f, 261.0f},
        {0.5f, 0.5f}
    );
    subtitle->setOpacity(185);

    constexpr std::array<float, 5> Rows = {225.0f, 184.0f, 143.0f, 102.0f, 61.0f};
    for (std::size_t index = 0; index + 1 < Rows.size(); ++index) {
        partyui::addDivider(m_mainLayer, 31.0f, Rows[index] - 20.5f, 368.0f);
    }

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_mainLayer->addChild(buttonMenu, 3);

    addSettingName(m_mainLayer, "Mode", Rows[0]);
    m_modeValue = partyui::addLabel(
        m_mainLayer,
        "Roulette Race",
        "goldFont.fnt",
        0.38f,
        {320.0f, Rows[0]},
        {0.5f, 0.5f}
    );
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModePrevious), false, {247.0f, Rows[0]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModeNext), true, {393.0f, Rows[0]});

    addSettingName(m_mainLayer, "Goal", Rows[1]);
    m_goalValue = partyui::addLabel(
        m_mainLayer,
        "50%",
        "goldFont.fnt",
        0.4f,
        {320.0f, Rows[1]},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalDown), "-", {274.0f, Rows[1]}, 32.0f, "GJ_button_05.png", 0.7f);
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalUp), "+", {366.0f, Rows[1]}, 32.0f, "GJ_button_05.png", 0.65f);

    addSettingName(m_mainLayer, "Difficulty", Rows[2]);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({266.0f, Rows[2]});
    m_mainLayer->addChild(m_difficultyIconNode, 2);

    m_difficultyValue = partyui::addLabel(
        m_mainLayer,
        "Random",
        "bigFont.fnt",
        0.31f,
        {321.0f, Rows[2]},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(96.0f, 0.31f, 0.23f);
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyPrevious), false, {224.0f, Rows[2]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyNext), true, {393.0f, Rows[2]});

    m_fourthRowLabel = partyui::addLabel(
        m_mainLayer,
        "Count extra progress",
        "bigFont.fnt",
        0.34f,
        {43.0f, Rows[3]}
    );
    m_overrunToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {371.0f, Rows[3]}
    );

    m_attemptsValue = partyui::addLabel(
        m_mainLayer,
        "1",
        "goldFont.fnt",
        0.4f,
        {320.0f, Rows[3]},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {274.0f, Rows[3]},
        32.0f,
        "GJ_button_05.png",
        0.7f
    );
    m_attemptsUpButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {366.0f, Rows[3]},
        32.0f,
        "GJ_button_05.png",
        0.65f
    );

    addSettingName(m_mainLayer, "Practice mode", Rows[4]);
    m_practiceToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {371.0f, Rows[4]}
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
    m_modeValue->limitLabelWidth(115.0f, 0.38f, 0.28f);
    m_goalValue->setString(fmt::format("{}%", settings.targetProgress).c_str());

    m_difficultyValue->setString(partyui::difficultyName(settings.difficulty));
    m_difficultyValue->limitLabelWidth(96.0f, 0.31f, 0.22f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.25f)) {
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
