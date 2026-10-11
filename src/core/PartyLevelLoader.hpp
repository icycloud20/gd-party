#pragma once

#include <Geode/Geode.hpp>

namespace gdparty {

class PartyLevelLoader final : public LevelDownloadDelegate {
public:
    static PartyLevelLoader& get();

    void openAssignedLevel();
    void openLevel(int levelId);
    bool isLoading() const;
    int requestedLevelId() const;

    void levelDownloadFinished(GJGameLevel* level) override;
    void levelDownloadFailed(int errorCode) override;

private:
    PartyLevelLoader() = default;

    void showLevelPage(GJGameLevel* level);
    bool isAlreadyShowing(int levelId) const;
    void clearRequest();

    bool m_loading = false;
    int m_requestedLevelId = 0;
};

} // namespace gdparty
