#pragma once

#include "PartyTypes.hpp"

#include <Geode/Geode.hpp>

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <unordered_set>

namespace gdparty {

class RandomLevelProvider final : public LevelManagerDelegate {
public:
    using Callback = std::function<void(std::optional<int>)>;

    static RandomLevelProvider& get();

    void request(
        DifficultyFilter difficulty,
        bool recentOnly,
        int recentPoolSize,
        std::uint64_t seed,
        std::unordered_set<int> excludedLevelIds,
        Callback callback
    );

    bool busy() const;

private:
    struct Request {
        DifficultyFilter difficulty = DifficultyFilter::RandomAllExceptAuto;
        bool recentOnly = false;
        int recentPoolSize = 100;
        std::uint64_t seed = 0;
        std::unordered_set<int> excludedLevelIds;
        Callback callback;
        int retryCount = 0;
    };

    RandomLevelProvider() = default;

    void processNext();
    void startCurrentRequest();
    GJSearchObject* makeSearchObject(Request const& request) const;
    bool matchesDifficulty(GJGameLevel* level, DifficultyFilter difficulty) const;
    void finishCurrent(std::optional<int> levelId);

    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int responseCode) override;
    void loadLevelsFailed(char const* key, int responseCode) override;

    std::deque<Request> m_requests;
    bool m_busy = false;
};

} // namespace gdparty
