# SBS Wars QA Test Plan

Executed automatically by `Binaries/sbswars-tests` and the dedicated-server smoke.

## Simulation
- Maps load, spawns exist, A* reaches the hill on all six maps
- Weapon definitions and class roles (HP, radar, heal, repair)
- Hitscan fire between two pawns
- TDM with mixed humans/bots, snapshot pack/parse
- CTF / Domination / KOTH tick without crash
- Spectator flag
- Save/load settings, stats, LAN favorites
- Discover query/reply roundtrip
- Dedicated host bind, bot fill, stop
- LAN discover from a second UDP socket to 127.0.0.1

## Multiplayer (manual / LAN party)
1. Host listen: `sbswars --listen`
2. Dedicated: `sbswars-server --port 27015 --bots 8`
3. Join from LAN browser or `--join ip:27015`
4. Confirm scoreboard, respawn, team swap via class/faction, disconnect timeout
5. Packet loss: LAN is trusted; host remains authoritative if a client stalls (8s timeout)

## Performance
Low quality 320×180 internal, High 480×270. Cap FPS in settings. Dedicated server has no renderer.
