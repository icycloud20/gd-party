#include "RandomLevelProvider.hpp"

#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace gdparty {
namespace {

std::uint64_t mix(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

char const* difficultyQuery(DifficultyFilter difficulty) {
    switch (difficulty) {
        case DifficultyFilter::RandomNonDemon:
            return "1,2,3,4,5";
        case DifficultyFilter::RandomDemon:
        case DifficultyFilter::EasyDemon:
        case DifficultyFilter::MediumDemon:
        case DifficultyFilter::HardDemon:
        case DifficultyFilter::InsaneDemon:
        case DifficultyFilter::ExtremeDemon:
            return "-2";
        case DifficultyFilter::Easy:
            return "1";
        case DifficultyFilter::Normal:
            return "2";
        case DifficultyFilter::Hard:
            return "3";
        case DifficultyFilter::Harder:
            return "4";
        case DifficultyFilter::Insane:
            return "5";
        case DifficultyFilter::RandomAllExceptAuto:
        default:
            return "1,2,3,4,5,-2";
    }
}

int demonFilterValue(DifficultyFilter difficulty) {
    switch (difficulty) {
        case DifficultyFilter::EasyDemon:
            return 1;
        case DifficultyFilter::MediumDemon:
            return 2;
        case DifficultyFilter::HardDemon:
            return 3;
        case DifficultyFilter::InsaneDemon:
            return 4;
        case DifficultyFilter::ExtremeDemon:
            return 5;
        default:
            return 0;
    }
}

} // namespace

RandomLevelProvider& RandomLevelProvider::get() {
    static RandomLevelProvider instance;
    return instance;
}

void RandomLevelProvider::request(
    DifficultyFilter difficulty,
    bool recentOnly,
    int recentPoolSize,
    std::uint64_t seed,
    std::unordered_set<int> excludedLevelIds,
    Callback callback
) {
    Request request;
    request.difficulty = difficulty;
    request.recentOnly = recentOnly;
    request.recentPoolSize = std::clamp(recentPoolSize, 10, 500);
    request.seed = seed;
    request.excludedLevelIds = std::move(excludedLevelIds);
    request.callback = std::move(callback);

    m_requests.push_back(std::move(request));
    processNext();
}

bool RandomLevelProvider::busy() const {
    return m_busy || !m_requests.empty();
}

void RandomLevelProvider::processNext() {
    if (m_busy || m_requests.empty()) {
        return;
    }

    m_busy = true;
    startCurrentRequest();
}

void RandomLevelProvider::startCurrentRequest() {
    if (m_requests.empty()) {
        m_busy = false;
        return;
    }

    auto levelManager = GameLevelManager::get();
    if (!levelManager) {
        finishCurrent(std::nullopt);
        return;
    }

    levelManager->m_levelManagerDelegate = this;
    auto searchObject = makeSearchObject(m_requests.front());
    levelManager->getOnlineLevels(searchObject);
}

GJSearchObject* RandomLevelProvider::makeSearchObject(Request const& request) const {
    auto searchObject = GJSearchObject::create(SearchType::Awarded);
    searchObject->m_difficulty = difficultyQuery(request.difficulty);
    searchObject->m_starFilter = true;
    searchObject->m_noStarFilter = false;
    searchObject->m_demonFilter = static_cast<GJDifficulty>(demonFilterValue(request.difficulty));

    int pageCount = request.recentOnly
        ? std::max(1, request.recentPoolSize / 10)
        : 200;

    auto mixedSeed = mix(request.seed ^ static_cast<std::uint64_t>(request.retryCount + 1));
    if (request.retryCount == 0) {
        searchObject->m_page = static_cast<int>(mixedSeed % static_cast<std::uint64_t>(pageCount));
    } else {
        searchObject->m_page = (request.retryCount - 1) % std::min(pageCount, 5);
    }

    return searchObject;
}

bool RandomLevelProvider::matchesDifficulty(GJGameLevel* level, DifficultyFilter difficulty) const {
    if (!level || level->m_stars <= 0 || level->m_autoLevel) {
        return false;
    }

    auto isDemon = level->m_demon == 1;
    if (difficulty == DifficultyFilter::RandomAllExceptAuto) {
        return true;
    }
    if (difficulty == DifficultyFilter::RandomNonDemon) {
        return !isDemon;
    }
    if (difficulty == DifficultyFilter::RandomDemon) {
        return isDemon;
    }

    if (difficulty == DifficultyFilter::EasyDemon ||
        difficulty == DifficultyFilter::MediumDemon ||
        difficulty == DifficultyFilter::HardDemon ||
        difficulty == DifficultyFilter::InsaneDemon ||
        difficulty == DifficultyFilter::ExtremeDemon) {
        if (!isDemon) {
            return false;
        }

        switch (difficulty) {
            case DifficultyFilter::EasyDemon:
                return level->m_demonDifficulty == 3;
            case DifficultyFilter::MediumDemon:
                return level->m_demonDifficulty == 4;
            case DifficultyFilter::HardDemon:
                return level->m_demonDifficulty == 0;
            case DifficultyFilter::InsaneDemon:
                return level->m_demonDifficulty == 5;
            case DifficultyFilter::ExtremeDemon:
                return level->m_demonDifficulty == 6;
            default:
                return false;
        }
    }

    if (isDemon) {
        return false;
    }

    auto averageDifficulty = level->getAverageDifficulty();
    switch (difficulty) {
        case DifficultyFilter::Easy:
            return averageDifficulty == 1;
        case DifficultyFilter::Normal:
            return averageDifficulty == 2;
        case DifficultyFilter::Hard:
            return averageDifficulty == 3;
        case DifficultyFilter::Harder:
            return averageDifficulty == 4;
        case DifficultyFilter::Insane:
            return averageDifficulty == 5;
        default:
            return false;
    }
}

void RandomLevelProvider::finishCurrent(std::optional<int> levelId) {
    if (m_requests.empty()) {
        m_busy = false;
        return;
    }

    auto request = std::move(m_requests.front());
    m_requests.pop_front();

    if (auto levelManager = GameLevelManager::get()) {
        if (levelManager->m_levelManagerDelegate == this) {
            levelManager->m_levelManagerDelegate = nullptr;
        }
    }

    m_busy = false;

    if (request.callback) {
        request.callback(levelId);
    }

    processNext();
}

void RandomLevelProvider::loadLevelsFinished(cocos2d::CCArray* levels, char const*, int) {
    if (m_requests.empty()) {
        m_busy = false;
        return;
    }

    auto& request = m_requests.front();
    std::vector<int> candidates;

    if (levels) {
        for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
            if (!matchesDifficulty(level, request.difficulty)) {
                continue;
            }

            auto levelId = level->m_levelID.value();
            if (levelId <= 0 || request.excludedLevelIds.contains(levelId)) {
                continue;
            }

            candidates.push_back(levelId);
        }
    }

    if (candidates.empty()) {
        ++request.retryCount;
        if (request.retryCount <= 4) {
            startCurrentRequest();
            return;
        }

        log::warn("GD Party could not find a rated level matching the selected filter");
        finishCurrent(std::nullopt);
        return;
    }

    auto mixedSeed = mix(request.seed ^ 0x6a09e667f3bcc909ULL);
    auto levelId = candidates[mixedSeed % candidates.size()];
    finishCurrent(levelId);
}

void RandomLevelProvider::loadLevelsFailed(char const*, int responseCode) {
    if (m_requests.empty()) {
        m_busy = false;
        return;
    }

    auto& request = m_requests.front();
    ++request.retryCount;

    if (request.retryCount <= 4) {
        log::warn("GD Party level query failed with code {}; retrying", responseCode);
        startCurrentRequest();
        return;
    }

    log::warn("GD Party level query failed with code {}", responseCode);
    finishCurrent(std::nullopt);
}

} // namespace gdparty
