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

CCScale9Sprite* addListPanel(CCNode* parent, CCPoint position, CCSize size) {
    auto panel = CCScale9Sprite::create("GJ_square01.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    parent->addChild(panel);
    return panel;
}

void addRowLabel(CCNode* parent, char const* text, float y) {
    auto label = partyui::addLabel(parent, text, "bigFont.fnt", 0.35f, {28.0f, y});
    label->limitLabelWidth(170.0f, 0.35f, 0.26f);
}

} // namespace

PartySettingsPopup* PartySettingsPopup::create() {
    auto layer = new PartySettingsPopup();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }

    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool PartySettingsPopup::init() {
    if (!initBase({0, 102, 255})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);
    auto shellSize = CCSize{382.0f, 244.0f};

    auto title = partyui::addLabel(
        this,
        "PARTY SETTINGS",
        "goldFont.fnt",
        0.72f,
        {center.x, windowSize.height - 25.0f},
        {0.5f, 0.5f}
    );
    title->setZOrder(10);

    m_shell = CCNode::create();
    m_shell->setContentSize(shellSize);
    m_shell->setAnchorPoint({0.5f, 0.5f});
    m_shell->setPosition({center.x, center.y - 4.0f});
    addChild(m_shell, 2);

    addListPanel(
        m_shell,
        {shellSize.width / 2.0f, shellSize.height / 2.0f},
        shellSize
    )->setZOrder(-4);

    auto tabBackground = ButtonSprite::create("MATCH", 92, true, "bigFont.fnt", "GJ_button_01.png", 28.0f, 0.52f);
    tabBackground->setPosition({73.0f, shellSize.height + 13.0f});
    m_shell->addChild(tabBackground, 5);

    auto sectionTitle = partyui::addLabel(
        m_shell,
        "Match Rules",
        "goldFont.fnt",
        0.39f,
        {shellSize.width / 2.0f, shellSize.height - 17.0f},
        {0.5f, 0.5f}
    );
    sectionTitle->setOpacity(235);

    constexpr std::array<float, 5> Rows = {190.0f, 151.0f, 112.0f, 73.0f, 34.0f};
    for (std::size_t index = 0; index + 1 < Rows.size(); ++index) {
        auto divider = CCLayerColor::create(ccc4(80, 35, 15, 110), shellSize.width - 22.0f, 1.0f);
        divider->setPosition({11.0f, Rows[index] - 19.5f});
        m_shell->addChild(divider, 2);
    }

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(buttonMenu, 10);

    addRowLabel(m_shell, "Mode", Rows[0]);
    m_modeValue = partyui::addLabel(
        m_shell,
        "Roulette Race",
        "goldFont.fnt",
        0.37f,
        {292.0f, Rows[0]},
        {0.5f, 0.5f}
    );
    m_modeValue->limitLabelWidth(104.0f, 0.37f, 0.27f);
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModePrevious), false, {230.0f, Rows[0]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModeNext), true, {354.0f, Rows[0]});

    addRowLabel(m_shell, "Goal", Rows[1]);
    m_goalValue = partyui::addLabel(
        m_shell,
        "50%",
        "goldFont.fnt",
        0.40f,
        {292.0f, Rows[1]},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalDown), "-", {247.0f, Rows[1]}, 31.0f, "GJ_button_05.png", 0.68f);
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalUp), "+", {337.0f, Rows[1]}, 31.0f, "GJ_button_05.png", 0.63f);

    addRowLabel(m_shell, "Difficulty", Rows[2]);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({258.0f, Rows[2]});
    m_shell->addChild(m_difficultyIconNode, 5);

    m_difficultyValue = partyui::addLabel(
        m_shell,
        "Random",
        "bigFont.fnt",
        0.30f,
        {300.0f, Rows[2]},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(93.0f, 0.30f, 0.22f);
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyPrevious), false, {213.0f, Rows[2]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyNext), true, {354.0f, Rows[2]});

    m_fourthRowLabel = partyui::addLabel(
        m_shell,
        "Count extra progress",
        "bigFont.fnt",
        0.33f,
        {28.0f, Rows[3]}
    );
    m_fourthRowLabel->limitLabelWidth(185.0f, 0.33f, 0.24f);

    m_overrunToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {342.0f, Rows[3]},
        0.62f
    );

    m_attemptsValue = partyui::addLabel(
        m_shell,
        "1",
        "goldFont.fnt",
        0.40f,
        {292.0f, Rows[3]},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {247.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.68f
    );
    m_attemptsUpButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {337.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.63f
    );

    addRowLabel(m_shell, "Practice mode", Rows[4]);
    m_practiceToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {342.0f, Rows[4]},
        0.62f
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
    m_difficultyValue->limitLabelWidth(93.0f, 0.30f, 0.21f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.22f)) {
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
