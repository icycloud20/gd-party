#include "PartyManager.hpp"

#include "PartyLevelLoader.hpp"
#include "PartyStateCodec.hpp"
#include "../gamemodes/RouletteRace.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <chrono>

using namespace geode::prelude;

namespace gdparty {
namespace {

struct LocalAppearance {
    int cubeIcon = 1;
    int color1 = 0;
    int color2 = 3;
    bool glow = false;
    int glowColor = 0;
};

LocalAppearance getLocalAppearance() {
    LocalAppearance appearance;
    auto gameManager = GameManager::sharedState();
    if (!gameManager) {
        return appearance;
    }

    appearance.cubeIcon = gameManager->getPlayerFrame();
    appearance.color1 = gameManager->getPlayerColor();
    appearance.color2 = gameManager->getPlayerColor2();
    appearance.glow = gameManager->getPlayerGlow();
    appearance.glowColor = gameManager->getPlayerGlowColor();
    return appearance;
}

std::uint64_t packAppearance(LocalAppearance const& appearance) {
    auto color2 = static_cast<std::uint64_t>(static_cast<std::uint32_t>(appearance.color2));
    auto glowColor = static_cast<std::uint64_t>(static_cast<std::uint32_t>(appearance.glowColor));
    return (color2 & 0xffffULL) |
        ((glowColor & 0xffffULL) << 16) |
        (appearance.glow ? (1ULL << 32) : 0ULL);
}

LocalAppearance unpackAppearance(int cubeIcon, int color1, std::uint64_t packed) {
    LocalAppearance appearance;
    appearance.cubeIcon = cubeIcon;
    appearance.color1 = color1;
    appearance.color2 = static_cast<int>(packed & 0xffffULL);
    appearance.glowColor = static_cast<int>((packed >> 16) & 0xffffULL);
    appearance.glow = ((packed >> 32) & 1ULL) != 0;
    return appearance;
}

void queueAssignedLevelOpen() {
    Loader::get()->queueInMainThread([] {
        PartyLevelLoader::get().openAssignedLevel();
    });
}

} // namespace

PartyManager& PartyManager::get() {
    static PartyManager instance;
    return instance;
}

void PartyManager::initialize(std::unique_ptr<PartyTransport> transport) {
    m_transport = std::move(transport);
    if (!m_transport) {
        return;
    }

    m_transport->setMessageHandler([this](PartyMessage const& message) {
        handleMessage(message);
    });

    resetForCurrentRoom();
}

PartyState const& PartyManager::state() const {
    return m_state;
}

PartyTransport const* PartyManager::transport() const {
    return m_transport.get();
}

bool PartyManager::canUseParty() const {
    return m_transport && m_transport->available() && m_transport->connected() && m_transport->inRoom();
}

void PartyManager::refreshRoomContext() {
    if (!m_transport) {
        return;
    }

    auto currentRoomId = m_transport->roomId();
    if (currentRoomId != m_state.roomId) {
        resetForCurrentRoom();
    }
}

bool PartyManager::isHost() const {
    return canUseParty() && m_transport->isRoomOwner() && m_state.hostAccountId == m_transport->localAccountId();
}

bool PartyManager::createParty() {
    if (!canUseParty() || !m_transport->isRoomOwner()) {
        return false;
    }

    resetForCurrentRoom();
    m_state.active = true;
    m_state.phase = PartyPhase::Lobby;
    m_state.hostAccountId = m_transport->localAccountId();

    auto appearance = getLocalAppearance();
    addOrUpdatePlayer(
        m_transport->localAccountId(),
        m_transport->localUsername(),
        appearance.cubeIcon,
        appearance.color1,
        appearance.color2,
        appearance.glow,
        appearance.glowColor
    );

    broadcastSnapshot();
    return true;
}

void PartyManager::discoverParty() {
    if (!canUseParty() || m_transport->isRoomOwner()) {
        return;
    }

    auto message = baseMessage(PartyMessageType::DiscoverParty);
    m_transport->sendReliableTo(message, m_transport->roomOwnerAccountId());
}

bool PartyManager::joinParty() {
    if (!canUseParty() || m_transport->isRoomOwner()) {
        return false;
    }

    auto appearance = getLocalAppearance();
    auto message = baseMessage(PartyMessageType::JoinRequest);
    message.text = m_transport->localUsername();
    message.valueA = appearance.cubeIcon;
    message.valueB = appearance.color1;
    message.valueC = packAppearance(appearance);
    m_transport->sendReliableTo(message, m_transport->roomOwnerAccountId());
    return true;
}

void PartyManager::leaveParty() {
    if (!m_transport || !m_state.active) {
        resetForCurrentRoom();
        return;
    }

    auto localAccountId = m_transport->localAccountId();
    if (isHost()) {
        m_state.active = false;
        m_state.phase = PartyPhase::Idle;
        m_state.players.clear();
        broadcastSnapshot();
    } else if (canUseParty()) {
        auto message = baseMessage(PartyMessageType::LeaveParty);
        m_transport->sendReliableTo(message, m_state.hostAccountId);
    }

    resetForCurrentRoom();
    m_state.localAccountId = localAccountId;
}

void PartyManager::setReady(bool ready) {
    if (!canUseParty() || !m_state.active) {
        return;
    }

    auto localAccountId = m_transport->localAccountId();
    if (isHost()) {
        if (auto player = findMutablePlayer(localAccountId)) {
            player->ready = ready;
            broadcastSnapshot();
        }
        return;
    }

    auto message = baseMessage(PartyMessageType::PlayerReady);
    message.valueA = ready ? 1 : 0;
    m_transport->sendReliableTo(message, m_state.hostAccountId);
}

void PartyManager::setGameMode(GameModeType gameMode) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.gameMode = gameMode;
    broadcastSnapshot();
}

void PartyManager::setTargetProgress(int targetProgress) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.targetProgress = std::clamp(targetProgress, 1, 100);
    broadcastSnapshot();
}

void PartyManager::setDifficulty(DifficultyFilter difficulty) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.difficulty = difficulty;
    broadcastSnapshot();
}

void PartyManager::setOverrunCounts(bool enabled) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.overrunCounts = enabled;
    broadcastSnapshot();
}

void PartyManager::setPracticeModeEnabled(bool enabled) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.practiceModeEnabled = enabled;
    broadcastSnapshot();
}

void PartyManager::setMaxAttempts(int maxAttempts) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.maxAttempts = std::clamp(maxAttempts, 1, 1000);
    broadcastSnapshot();
}

void PartyManager::setRecentPoolSize(int recentPoolSize) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.recentPoolSize = std::clamp(recentPoolSize, 10, 500);
    broadcastSnapshot();
}

bool PartyManager::startMatch() {
    if (!isHost() || !m_state.active || m_state.phase != PartyPhase::Lobby || m_state.players.empty()) {
        return false;
    }

    auto allReady = std::all_of(m_state.players.begin(), m_state.players.end(), [](PartyPlayer const& player) {
        return player.ready;
    });

    if (!allReady) {
        Notification::create("Everyone must be ready first", NotificationIcon::Warning)->show();
        return false;
    }

    if (m_state.settings.gameMode != GameModeType::RouletteRace) {
        Notification::create("That mode is not playable yet", NotificationIcon::Warning)->show();
        return false;
    }

    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    m_state.settings.seed = static_cast<std::uint64_t>(now) ^ static_cast<std::uint64_t>(m_state.roomId);
    m_state.winnerAccountId = 0;
    m_lastReportedTarget = 0;
    m_pendingRoundAdvanceOpen = false;

    RouletteRace rouletteRace;
    rouletteRace.begin(m_state);
    broadcastSnapshot();
    queueAssignedLevelOpen();
    return true;
}

void PartyManager::reportLocalProgress(int progress, int levelId) {
    if (!canUseParty() || !m_state.active || m_state.phase != PartyPhase::Playing) {
        return;
    }

    auto player = localPlayer();
    if (!player || player->finished || player->currentLevelId != levelId || progress < player->rouletteTarget) {
        return;
    }

    if (m_lastReportedTarget == player->rouletteTarget) {
        return;
    }

    m_lastReportedTarget = player->rouletteTarget;

    auto message = baseMessage(PartyMessageType::ReachedTarget);
    message.valueA = player->rouletteTarget;
    message.valueB = levelId;
    message.valueC = static_cast<std::uint64_t>(std::clamp(progress, 0, 100));

    if (isHost()) {
        handleHostMessage(message);
    } else {
        m_transport->sendReliableTo(message, m_state.hostAccountId);
    }
}

void PartyManager::reportAttemptEnded(int progress, int levelId) {
    if (!canUseParty() || !m_state.active || m_state.phase != PartyPhase::Playing) {
        return;
    }

    auto player = localPlayer();
    if (!player || player->finished || player->currentLevelId != levelId) {
        return;
    }

    auto message = baseMessage(PartyMessageType::AttemptEnded);
    message.valueA = std::clamp(progress, 0, 100);
    message.valueB = levelId;
    message.valueC = static_cast<std::uint64_t>(player->rouletteTarget);

    if (isHost()) {
        handleHostMessage(message);
    } else {
        m_transport->sendReliableTo(message, m_state.hostAccountId);
    }
}

bool PartyManager::requestAdvanceRound(int achievedProgress) {
    if (!canUseParty() || !m_state.active || m_state.phase != PartyPhase::Playing) {
        return false;
    }

    auto player = localPlayer();
    if (!player || player->finished || player->currentLevelId <= 0) {
        return false;
    }

    achievedProgress = std::clamp(achievedProgress, 0, 100);
    if (achievedProgress < player->rouletteTarget && !player->roundSecured) {
        return false;
    }

    m_pendingRoundAdvanceOpen = true;

    auto message = baseMessage(PartyMessageType::AdvanceRound);
    message.valueA = player->rouletteTarget;
    message.valueB = player->currentLevelId;
    message.valueC = static_cast<std::uint64_t>(achievedProgress);

    if (isHost()) {
        handleHostMessage(message);
    } else {
        m_transport->sendReliableTo(message, m_state.hostAccountId);
    }

    return true;
}

PartyPlayer const* PartyManager::localPlayer() const {
    if (!m_transport) {
        return nullptr;
    }

    return findPlayer(m_transport->localAccountId());
}

PartyPlayer const* PartyManager::findPlayer(int accountId) const {
    auto player = std::find_if(m_state.players.begin(), m_state.players.end(), [accountId](PartyPlayer const& value) {
        return value.accountId == accountId;
    });

    return player == m_state.players.end() ? nullptr : &*player;
}

void PartyManager::resetForCurrentRoom() {
    m_state = {};
    m_lastReportedTarget = 0;
    m_pendingRoundAdvanceOpen = false;

    if (!m_transport) {
        return;
    }

    m_state.localAccountId = m_transport->localAccountId();
    m_state.roomId = m_transport->roomId();
    m_state.hostAccountId = m_transport->roomOwnerAccountId();
}

void PartyManager::handleMessage(PartyMessage const& message) {
    if (!canUseParty() || message.roomId == 0 || message.roomId != m_transport->roomId()) {
        return;
    }

    if (m_transport->isRoomOwner()) {
        handleHostMessage(message);
        return;
    }

    if (message.senderAccountId != m_transport->roomOwnerAccountId()) {
        return;
    }

    if (message.type == PartyMessageType::PartySnapshot) {
        applySnapshot(message);
    }
}

void PartyManager::handleHostMessage(PartyMessage const& message) {
    if (!m_transport || !m_transport->isRoomOwner()) {
        return;
    }

    switch (message.type) {
        case PartyMessageType::DiscoverParty:
            if (m_state.active) {
                sendSnapshotTo(message.senderAccountId);
            }
            break;

        case PartyMessageType::JoinRequest: {
            if (!m_state.active) {
                return;
            }

            auto appearance = unpackAppearance(message.valueA, message.valueB, message.valueC);
            addOrUpdatePlayer(
                message.senderAccountId,
                message.text,
                appearance.cubeIcon,
                appearance.color1,
                appearance.color2,
                appearance.glow,
                appearance.glowColor
            );
            broadcastSnapshot();
            break;
        }

        case PartyMessageType::PlayerReady:
            if (auto player = findMutablePlayer(message.senderAccountId)) {
                player->ready = message.valueA != 0;
                broadcastSnapshot();
            }
            break;

        case PartyMessageType::ReachedTarget: {
            if (m_state.phase != PartyPhase::Playing) {
                return;
            }

            auto player = findMutablePlayer(message.senderAccountId);
            if (!player || player->finished ||
                player->rouletteTarget != message.valueA ||
                player->currentLevelId != message.valueB) {
                return;
            }

            player->progress = std::max(
                player->progress,
                std::clamp(static_cast<int>(message.valueC), 0, 100)
            );
            player->roundSecured = player->progress >= player->rouletteTarget;
            broadcastSnapshot();
            break;
        }

        case PartyMessageType::AttemptEnded: {
            if (m_state.phase != PartyPhase::Playing) {
                return;
            }

            auto player = findMutablePlayer(message.senderAccountId);
            if (!player || player->finished || player->currentLevelId != message.valueB) {
                return;
            }

            player->progress = std::max(player->progress, std::clamp(message.valueA, 0, 100));
            player->roundSecured = player->progress >= player->rouletteTarget;
            ++player->attemptCount;
            broadcastSnapshot();
            break;
        }

        case PartyMessageType::AdvanceRound: {
            if (m_state.phase != PartyPhase::Playing) {
                return;
            }

            auto player = findMutablePlayer(message.senderAccountId);
            if (!player || player->finished ||
                player->rouletteTarget != message.valueA ||
                player->currentLevelId != message.valueB) {
                return;
            }

            auto achievedProgress = std::clamp(static_cast<int>(message.valueC), 0, 100);
            player->progress = std::max(player->progress, achievedProgress);
            player->roundSecured = player->progress >= player->rouletteTarget;
            if (!player->roundSecured) {
                return;
            }

            auto won = RouletteRace::advancePlayer(*player, m_state.settings);
            if (won) {
                player->finished = true;
                m_state.winnerAccountId = player->accountId;
                m_state.phase = PartyPhase::Results;
            }

            broadcastSnapshot();

            if (message.senderAccountId == m_transport->localAccountId()) {
                m_pendingRoundAdvanceOpen = false;
                if (!won) {
                    queueAssignedLevelOpen();
                }
            }
            break;
        }

        case PartyMessageType::LeaveParty: {
            auto removeAccountId = message.senderAccountId;
            std::erase_if(m_state.players, [removeAccountId](PartyPlayer const& player) {
                return player.accountId == removeAccountId;
            });
            broadcastSnapshot();
            break;
        }

        default:
            break;
    }
}

void PartyManager::broadcastSnapshot() {
    if (!m_transport || !m_transport->inRoom()) {
        return;
    }

    auto message = baseMessage(PartyMessageType::PartySnapshot);
    message.data = encodePartyState(m_state);
    m_transport->sendReliable(message);
}

void PartyManager::sendSnapshotTo(int accountId) {
    if (!m_transport || accountId <= 0) {
        return;
    }

    auto message = baseMessage(PartyMessageType::PartySnapshot);
    message.data = encodePartyState(m_state);
    m_transport->sendReliableTo(message, accountId);
}

void PartyManager::applySnapshot(PartyMessage const& message) {
    PartyState snapshot;
    if (!decodePartyState(message.data, snapshot) || snapshot.roomId != m_transport->roomId()) {
        return;
    }

    auto oldPhase = m_state.phase;
    auto oldLocalPlayer = localPlayer();
    auto oldLevelId = oldLocalPlayer ? oldLocalPlayer->currentLevelId : 0;
    auto oldTarget = oldLocalPlayer ? oldLocalPlayer->rouletteTarget : 0;

    snapshot.localAccountId = m_transport->localAccountId();
    m_state = std::move(snapshot);

    auto player = localPlayer();
    if (!player || player->rouletteTarget != m_lastReportedTarget) {
        m_lastReportedTarget = 0;
    }

    if (!player) {
        return;
    }

    auto startedMatch = oldPhase != PartyPhase::Playing && m_state.phase == PartyPhase::Playing;
    auto advancedRound = m_pendingRoundAdvanceOpen &&
        m_state.phase == PartyPhase::Playing &&
        player->currentLevelId > 0 &&
        (player->currentLevelId != oldLevelId || player->rouletteTarget != oldTarget);

    if (startedMatch || advancedRound) {
        m_pendingRoundAdvanceOpen = false;
        queueAssignedLevelOpen();
    }

    if (m_state.phase == PartyPhase::Results) {
        m_pendingRoundAdvanceOpen = false;
    }
}

void PartyManager::addOrUpdatePlayer(
    int accountId,
    std::string const& name,
    int cubeIcon,
    int color1,
    int color2,
    bool glow,
    int glowColor
) {
    if (accountId <= 0) {
        return;
    }

    if (auto existing = findMutablePlayer(accountId)) {
        if (!name.empty()) {
            existing->name = name;
        }
        existing->cubeIcon = std::max(1, cubeIcon);
        existing->color1 = color1;
        existing->color2 = color2;
        existing->glow = glow;
        existing->glowColor = glowColor;
        return;
    }

    PartyPlayer player;
    player.accountId = accountId;
    player.name = name.empty() ? "Player" : name;
    player.cubeIcon = std::max(1, cubeIcon);
    player.color1 = color1;
    player.color2 = color2;
    player.glow = glow;
    player.glowColor = glowColor;
    m_state.players.push_back(std::move(player));
}

PartyPlayer* PartyManager::findMutablePlayer(int accountId) {
    auto player = std::find_if(m_state.players.begin(), m_state.players.end(), [accountId](PartyPlayer const& value) {
        return value.accountId == accountId;
    });

    return player == m_state.players.end() ? nullptr : &*player;
}

PartyMessage PartyManager::baseMessage(PartyMessageType type) {
    PartyMessage message;
    message.type = type;
    message.roomId = m_transport ? m_transport->roomId() : 0;
    message.senderAccountId = m_transport ? m_transport->localAccountId() : 0;
    message.sequence = ++m_sequence;
    return message;
}

} // namespace gdparty
