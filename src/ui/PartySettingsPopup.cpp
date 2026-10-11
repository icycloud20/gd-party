#include "PartySettingsPopup.hpp"

#include "PartyUI.hpp"
#include "../core/PartyManager.hpp"

#include <algorithm>
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

CCScale9Sprite* makePanel(CCNode* parent, CCPoint position, CCSize size, GLubyte opacity = 255) {
    auto panel = CCScale9Sprite::create("square02_001.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    panel->setContentSize(size);
    panel->setPosition(position);
    panel->setOpacity(opacity);
    panel->setColor({51, 68, 153});
    parent->addChild(panel);
    return panel;
}

void addRowLabel(CCNode* parent, char const* text, float x, float y, float width) {
    auto label = partyui::addLabel(parent, text, "bigFont.fnt", 0.32f, {x, y});
    label->limitLabelWidth(width, 0.32f, 0.23f);
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
    if (!initBase({37, 50, 167})) {
        return false;
    }

    auto windowSize = CCDirector::sharedDirector()->getWinSize();
    auto center = ccp(windowSize.width / 2.0f, windowSize.height / 2.0f);
    auto shellSize = CCSize{
        std::clamp(windowSize.width * 0.75f, 360.0f, 420.0f),
        290.0f
    };
    auto innerWidth = shellSize.width * 0.8f;
    auto innerLeft = (shellSize.width - innerWidth) / 2.0f;
    auto innerRight = innerLeft + innerWidth;

    m_shell = CCNode::create();
    m_shell->setContentSize(shellSize);
    m_shell->setAnchorPoint({0.5f, 0.5f});
    m_shell->setPosition(center);
    addChild(m_shell, 2);

    auto outer = CCScale9Sprite::create("GJ_square02.png", CCRect{0.0f, 0.0f, 80.0f, 80.0f});
    outer->setContentSize(shellSize);
    outer->setPosition({shellSize.width / 2.0f, shellSize.height / 2.0f});
    m_shell->addChild(outer, -5);

    partyui::addLabel(
        m_shell,
        "PARTY SETTINGS",
        "goldFont.fnt",
        0.67f,
        {shellSize.width / 2.0f, shellSize.height - 18.0f},
        {0.5f, 0.5f}
    );

    auto sectionTitle = partyui::addLabel(
        m_shell,
        "MATCH RULES",
        "chatFont.fnt",
        0.32f,
        {shellSize.width / 2.0f, shellSize.height - 40.0f},
        {0.5f, 0.5f}
    );
    sectionTitle->setOpacity(170);

    auto panelSize = CCSize{innerWidth, 212.0f};
    makePanel(
        m_shell,
        {shellSize.width / 2.0f, 128.0f},
        panelSize,
        225
    );

    constexpr std::array<float, 5> Rows = {212.0f, 172.0f, 132.0f, 92.0f, 52.0f};
    for (std::size_t index = 0; index + 1 < Rows.size(); ++index) {
        auto divider = CCLayerColor::create(ccc4(255, 255, 255, 24), innerWidth - 10.0f, 1.0f);
        divider->setPosition({innerLeft + 5.0f, Rows[index] - 20.0f});
        m_shell->addChild(divider, 2);
    }

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0.0f, 0.0f});
    m_shell->addChild(buttonMenu, 10);

    auto labelX = innerLeft + 14.0f;
    auto labelWidth = innerWidth * 0.45f;
    auto controlCenterX = innerRight - 72.0f;
    auto leftControlX = innerRight - 130.0f;
    auto rightControlX = innerRight - 14.0f;

    addRowLabel(m_shell, "Mode", labelX, Rows[0], labelWidth);
    m_modeValue = partyui::addLabel(
        m_shell,
        "Roulette Race",
        "goldFont.fnt",
        0.34f,
        {controlCenterX, Rows[0]},
        {0.5f, 0.5f}
    );
    m_modeValue->limitLabelWidth(104.0f, 0.34f, 0.24f);
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModePrevious), false, {leftControlX, Rows[0]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onModeNext), true, {rightControlX, Rows[0]});

    addRowLabel(m_shell, "Goal", labelX, Rows[1], labelWidth);
    m_goalValue = partyui::addLabel(
        m_shell,
        "50%",
        "goldFont.fnt",
        0.38f,
        {controlCenterX, Rows[1]},
        {0.5f, 0.5f}
    );
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalDown), "-", {leftControlX + 13.0f, Rows[1]}, 31.0f, "GJ_button_05.png", 0.68f);
    partyui::addTextButton(buttonMenu, this, menu_selector(PartySettingsPopup::onGoalUp), "+", {rightControlX - 13.0f, Rows[1]}, 31.0f, "GJ_button_05.png", 0.63f);

    addRowLabel(m_shell, "Difficulty", labelX, Rows[2], labelWidth);
    m_difficultyIconNode = CCNode::create();
    m_difficultyIconNode->setPosition({controlCenterX - 39.0f, Rows[2]});
    m_shell->addChild(m_difficultyIconNode, 5);

    m_difficultyValue = partyui::addLabel(
        m_shell,
        "Random",
        "bigFont.fnt",
        0.28f,
        {controlCenterX + 13.0f, Rows[2]},
        {0.5f, 0.5f}
    );
    m_difficultyValue->limitLabelWidth(94.0f, 0.28f, 0.20f);
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyPrevious), false, {leftControlX, Rows[2]});
    partyui::addArrowButton(buttonMenu, this, menu_selector(PartySettingsPopup::onDifficultyNext), true, {rightControlX, Rows[2]});

    m_fourthRowLabel = partyui::addLabel(
        m_shell,
        "Count extra progress",
        "bigFont.fnt",
        0.30f,
        {labelX, Rows[3]}
    );
    m_fourthRowLabel->limitLabelWidth(labelWidth + 22.0f, 0.30f, 0.21f);

    m_overrunToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onToggleOverrun),
        {innerRight - 24.0f, Rows[3]},
        0.62f
    );

    m_attemptsValue = partyui::addLabel(
        m_shell,
        "1",
        "goldFont.fnt",
        0.38f,
        {controlCenterX, Rows[3]},
        {0.5f, 0.5f}
    );
    m_attemptsDownButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsDown),
        "-",
        {leftControlX + 13.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.68f
    );
    m_attemptsUpButton = partyui::addTextButton(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onAttemptsUp),
        "+",
        {rightControlX - 13.0f, Rows[3]},
        31.0f,
        "GJ_button_05.png",
        0.63f
    );

    addRowLabel(m_shell, "Practice mode", labelX, Rows[4], labelWidth);
    m_practiceToggle = partyui::addToggle(
        buttonMenu,
        this,
        menu_selector(PartySettingsPopup::onTogglePractice),
        {innerRight - 24.0f, Rows[4]},
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
    m_difficultyValue->limitLabelWidth(94.0f, 0.28f, 0.20f);
    m_difficultyIconNode->removeAllChildren();
    if (auto icon = partyui::makeDifficultyIcon(settings.difficulty, 0.20f)) {
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
