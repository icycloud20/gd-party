# GD Party

GD Party is a Geode mod that turns Geometry Dash + Globed into lightweight competitive party modes for friends.

Instead of trying to be a global ranked ladder, GD Party is built around **private Globed rooms**: choose a mode, ready up, press Start, and compete under shared rules.

## Current V0.1 vertical slice

The first playable mode is **Roulette Race**.

- Open **Party** from the Geometry Dash main menu.
- The owner of the current private Globed room creates the GD Party lobby.
- Other room members discover and join it.
- Everyone readies up.
- The host chooses the finishing target (1-100%).
- Start generates one deterministic seed shared by the lobby.
- Every player receives the same level for each target percentage.
- When a player reaches the required percentage, their target advances by 1% and the next level is assigned.
- First player to reach the configured finishing target wins.

V0.1 currently uses a small hard-coded online-level pool while the dynamic level-provider/query system is developed.

## Multiplayer model

GD Party does **not** run a separate backend.

Globed provides the connection, private room and authenticated account identity. GD Party uses Globed 2.2+ Server Events for small, reliable state messages. The **Globed room owner is authoritative** for party state and validates ready/progress events before broadcasting snapshots.

Progress is detected locally from `PlayLayer::getCurrentPercentInt()`. We only transmit meaningful milestones such as reaching the current roulette target; GD Party does not stream position or progress every frame.

## Architecture

```text
src/
  core/       Party state, host authority, snapshot serialization
  gamemodes/  Mode-specific rules (Roulette Race first)
  network/    Transport abstraction, packet codec, Globed adapter
  ui/         Native-style Geometry Dash / Geode UI
```

Game rules do not talk directly to Globed. `PartyTransport` is the networking boundary so future transports/tests do not require rewriting the game logic.

## Building

Install the current stable Geode SDK/CLI, set `GEODE_SDK`, then run:

```bash
geode build
```

The repository also contains a multi-platform GitHub Actions build.

## Roadmap

After the Roulette Race vertical slice is solid:

- Dynamic random-level pools and filters
- Auto-opening / smoother round transitions
- Level Race
- Progress Rush
- Consistency
- Lives / Sudden Death
- Demon Sprint
- Higher or Lower
- Random Mode
