# GD Party development notes

## V0.1: prove Roulette Race end-to-end

The first milestone is intentionally narrow. We should not add more gamemodes until two real clients can finish one synchronized Roulette Race.

### Flow

1. Both players join the same private Globed room.
2. Room owner opens GD Party and creates a lobby.
3. Guest discovers/joins the lobby through a targeted Globed Server Event.
4. Players ready up.
5. Host chooses the final target and starts.
6. Host creates the seed and initializes every player's first target/level.
7. Host broadcasts the canonical party snapshot.
8. Each client opens its assigned level.
9. A PlayLayer hook detects reaching the required target percentage.
10. Client sends one `ReachedTarget` event to the host.
11. Host validates sender, target and level, advances only that player, then broadcasts a new snapshot.
12. First player to satisfy the configured final target becomes the winner.

## Authority and trust

- Globed room owner = GD Party host for V0.1.
- Incoming `senderAccountId` values are overwritten with Globed's authenticated `EventOptions.sender`; never trust an account ID from packet payload.
- Guests never mutate canonical party state locally.
- Host validates the current level ID and current roulette target before accepting a milestone.
- Packets are sent on meaningful state transitions only.

## Current packet types

- `DiscoverParty`
- `JoinRequest`
- `PartySnapshot`
- `PlayerReady`
- `StartMatch` (reserved for finer-grained flow)
- `ReachedTarget`
- `LeaveParty`

The current implementation uses a compact binary codec for messages and snapshots rather than JSON.

## Current temporary limitation

Roulette Race uses a small deterministic hard-coded level pool. This is deliberate for the first networking/playable test. Replace it with a proper level provider after the multiplayer state machine is verified.

## Next engineering targets

1. Compile cleanly on Geode 5.10.x / GD 2.2081.
2. Real two-client Globed room test.
3. Replace temporary level pool with online query/filter pipeline.
4. Improve round transition UX so players don't manually press Open Level every round.
5. Handle host leaving / room changes gracefully.
6. Add a compact in-level match HUD.
