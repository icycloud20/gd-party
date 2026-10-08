#include "GlobedTransport.hpp"

#include <Geode/Geode.hpp>
#include <dankmeme.globed2/include/globed/core/Event.hpp>
#include <dankmeme.globed2/include/globed/soft-link/API.hpp>

using namespace geode::prelude;

namespace gdparty {
namespace {

struct GDPartyEvent : globed::ServerEvent<GDPartyEvent, globed::EventServer::Central> {
    static constexpr auto Id = "party-message"_spr;

    std::vector<std::uint8_t> payload;

    GDPartyEvent() = default;
    explicit GDPartyEvent(std::vector<std::uint8_t> data) : payload(std::move(data)) {}

    std::vector<std::uint8_t> encode() const {
        return payload;
    }

    static Result<GDPartyEvent> decode(std::span<const std::uint8_t> data) {
        return Ok(GDPartyEvent{{data.begin(), data.end()}});
    }
};

void sendEvent(PartyMessage const& message, std::optional<int> targetAccountId) {
    if (!globed::api::available() || !globed::api::net::isConnected() || !globed::api::room::isInRoom()) {
        return;
    }

    globed::EventOptions options{};
    options.server = globed::EventServer::Central;
    options.reliable = true;
    options.urgent = true;

    if (targetAccountId && *targetAccountId > 0) {
        options.targetPlayers.push_back(*targetAccountId);
    }

    GDPartyEvent{encodePartyMessage(message)}.send(options);
}

} // namespace

bool GlobedTransport::available() const {
    return globed::api::available() && globed::api::isAtLeast("v2.2.0");
}

bool GlobedTransport::connected() const {
    return available() && globed::api::net::isConnected();
}

bool GlobedTransport::inRoom() const {
    return connected() && globed::api::room::isInRoom();
}

bool GlobedTransport::isRoomOwner() const {
    return inRoom() && globed::api::room::isOwner();
}

std::uint32_t GlobedTransport::roomId() const {
    return inRoom() ? globed::api::room::getId() : 0;
}

int GlobedTransport::roomOwnerAccountId() const {
    return inRoom() ? globed::api::room::getOwner() : 0;
}

int GlobedTransport::localAccountId() const {
    auto accountManager = GJAccountManager::sharedState();
    return accountManager ? accountManager->m_accountID : 0;
}

std::string GlobedTransport::localUsername() const {
    auto gameManager = GameManager::sharedState();
    if (!gameManager) {
        return "Player";
    }

    auto playerName = std::string(gameManager->m_playerName);
    return playerName.empty() ? "Player" : playerName;
}

void GlobedTransport::sendReliable(PartyMessage const& message) {
    sendEvent(message, std::nullopt);
}

void GlobedTransport::sendReliableTo(PartyMessage const& message, int accountId) {
    sendEvent(message, accountId);
}

void GlobedTransport::setMessageHandler(PartyMessageHandler handler) {
    m_messageHandler = std::move(handler);
    ensureListener();
}

void GlobedTransport::ensureListener() {
    if (m_listenerRegistered) {
        return;
    }

    m_listenerRegistered = true;

    globed::api::waitForGlobed([this] {
        GDPartyEvent::listen([this](GDPartyEvent const& event, globed::EventOptions const& options) {
            if (!m_messageHandler) {
                return;
            }

            PartyMessage message;
            if (!decodePartyMessage(event.payload, message)) {
                log::warn("Ignoring malformed GD Party event");
                return;
            }

            message.senderAccountId = options.sender;
            m_messageHandler(message);
        }).leak();
    });
}

} // namespace gdparty
