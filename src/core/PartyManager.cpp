#include "PartyManager.hpp"

#include "PartyStateCodec.hpp"
#include "../gamemodes/RouletteRace.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <chrono>

using namespace geode::prelude;

namespace gdparty {

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
    addOrUpdatePlayer(m_transport->localAccountId(), m_transport->localUsername());
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

    auto message = baseMessage(PartyMessageType::JoinRequest);
    message.text = m_transport->localUsername();
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

void PartyManager::setTargetProgress(int targetProgress) {
    if (!isHost() || m_state.phase != PartyPhase::Lobby) {
        return;
    }

    m_state.settings.targetProgress = std::clamp(targetProgress, 1, 100);
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

    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    m_state.settings.seed = static_cast<std::uint64_t>(now) ^ static_cast<std::uint64_t>(m_state.roomId);
    m_state.winnerAccountId = 0;
    m_lastReportedTarget = 0;

    RouletteRace rouletteRace;
    rouletteRace.begin(m_state);
    broadcastSnapshot();
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

    if (isHost()) {
        PartyMessage message = baseMessage(PartyMessageType::ReachedTarget);
        message.valueA = player->rouletteTarget;
        message.valueB = levelId;
        handleHostMessage(message);
        return;
    }

    auto message = baseMessage(PartyMessageType::ReachedTarget);
    message.valueA = player->rouletteTarget;
    message.valueB = levelId;
    m_transport->sendReliableTo(message, m_state.hostAccountId);
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

        case PartyMessageType::JoinRequest:
            if (!m_state.active) {
                return;
            }
            addOrUpdatePlayer(message.senderAccountId, message.text);
            broadcastSnapshot();
            break;

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

            player->progress = std::max(player->progress, player->rouletteTarget);
            auto won = RouletteRace::advancePlayer(*player, m_state.settings);
            if (won) {
                player->finished = true;
                m_state.winnerAccountId = player->accountId;
                m_state.phase = PartyPhase::Results;
            }

            broadcastSnapshot();
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

    snapshot.localAccountId = m_transport->localAccountId();
    m_state = std::move(snapshot);

    auto player = localPlayer();
    if (!player || player->rouletteTarget != m_lastReportedTarget) {
        m_lastReportedTarget = 0;
    }
}

void PartyManager::addOrUpdatePlayer(int accountId, std::string const& name) {
    if (accountId <= 0) {
        return;
    }

    if (auto existing = findMutablePlayer(accountId)) {
        if (!name.empty()) {
            existing->name = name;
        }
        return;
    }

    PartyPlayer player;
    player.accountId = accountId;
    player.name = name.empty() ? "Player" : name;
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
