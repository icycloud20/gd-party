#include "PartyLevelLoader.hpp"

#include "PartyManager.hpp"

#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

namespace gdparty {

PartyLevelLoader& PartyLevelLoader::get() {
    static PartyLevelLoader instance;
    return instance;
}

void PartyLevelLoader::openAssignedLevel() {
    auto const& manager = PartyManager::get();
    if (manager.state().phase != PartyPhase::Playing) {
        return;
    }

    auto player = manager.localPlayer();
    if (!player || player->finished || player->currentLevelId <= 0) {
        return;
    }

    openLevel(player->currentLevelId);
}

void PartyLevelLoader::openLevel(int levelId) {
    if (levelId <= 0 || isAlreadyPlaying(levelId)) {
        return;
    }

    if (m_loading) {
        if (m_requestedLevelId != levelId) {
            log::warn(
                "GD Party ignored level {} while level {} is still loading",
                levelId,
                m_requestedLevelId
            );
        }
        return;
    }

    auto levelManager = GameLevelManager::sharedState();
    if (!levelManager) {
        Notification::create("Could not access Geometry Dash levels", NotificationIcon::Error)->show();
        return;
    }

    m_requestedLevelId = levelId;

    if (levelManager->hasDownloadedLevel(levelId)) {
        if (auto savedLevel = levelManager->getSavedLevel(levelId)) {
            playLevel(savedLevel);
            clearRequest();
            return;
        }
    }

    m_loading = true;
    levelManager->m_levelDownloadDelegate = this;
    levelManager->downloadLevel(levelId, true);

    log::info("GD Party downloading assigned level {}", levelId);
}

bool PartyLevelLoader::isLoading() const {
    return m_loading;
}

int PartyLevelLoader::requestedLevelId() const {
    return m_requestedLevelId;
}

void PartyLevelLoader::levelDownloadFinished(GJGameLevel* level) {
    auto expectedLevelId = m_requestedLevelId;

    if (!level || level->m_levelID.value() != expectedLevelId) {
        log::warn(
            "GD Party received an unexpected level download (expected {}, got {})",
            expectedLevelId,
            level ? level->m_levelID.value() : 0
        );
        clearRequest();
        return;
    }

    log::info("GD Party finished downloading level {}", expectedLevelId);
    playLevel(level);
    clearRequest();
}

void PartyLevelLoader::levelDownloadFailed(int errorCode) {
    auto failedLevelId = m_requestedLevelId;
    log::warn("GD Party failed to download level {} with error {}", failedLevelId, errorCode);
    clearRequest();

    Notification::create(
        fmt::format("Failed to load roulette level {}", failedLevelId),
        NotificationIcon::Error
    )->show();
}

void PartyLevelLoader::playLevel(GJGameLevel* level) {
    if (!level || isAlreadyPlaying(level->m_levelID.value())) {
        return;
    }

    auto audioEngine = FMODAudioEngine::sharedEngine();
    if (audioEngine) {
        audioEngine->stopAllMusic(true);
        audioEngine->playEffect("playSound_01.ogg", 1.0f, 0.0f, 0.3f);
    }

    if (auto gameManager = GameManager::sharedState()) {
        gameManager->m_sceneEnum = 3;
    }

    auto playScene = PlayLayer::scene(level, false, false);
    if (!playScene) {
        Notification::create("Could not open roulette level", NotificationIcon::Error)->show();
        return;
    }

    CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.25f, playScene));
}

bool PartyLevelLoader::isAlreadyPlaying(int levelId) const {
    auto director = CCDirector::sharedDirector();
    if (!director) {
        return false;
    }

    auto scene = director->getRunningScene();
    if (!scene) {
        return false;
    }

    auto playLayer = scene->getChildByType<PlayLayer>(0);
    return playLayer && playLayer->m_level && playLayer->m_level->m_levelID.value() == levelId;
}

void PartyLevelLoader::clearRequest() {
    if (auto levelManager = GameLevelManager::sharedState()) {
        if (levelManager->m_levelDownloadDelegate == this) {
            levelManager->m_levelDownloadDelegate = nullptr;
        }
    }

    m_loading = false;
    m_requestedLevelId = 0;
}

} // namespace gdparty
