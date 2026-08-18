# SBS Wars — Network Architecture

LAN only. No internet relays, accounts, or matchmaking.

## Transports
- UDP game port default **27015**
- UDP discovery port **27016**
- Broadcast discover query/reply on both ports so mixed binds still see hosts

## Host types
| Type | Authority | Notes |
| --- | --- | --- |
| Offline vs bots | Local `World` | Listen server with no remote clients |
| Listen server | Host machine | Host plays; others join |
| Dedicated server | Headless `sbswars-server` | 2–64 slots, bot fill |

Peer-to-peer fallback: if dedicated bind fails, the client starts a listen server on an ephemeral port and advertises it on LAN.

## Packets
Magic `SBSW` + protocol version 1.

DiscoverQuery, DiscoverReply, JoinRequest, JoinAccept, JoinReject/Leave, ClientInput, Snapshot, Chat, Heartbeat.

Snapshots carry quantized player pose (1/32 cell), yaw byte, HP, score, projectiles, and objectives. LAN MTU is treated as 4 KB.

## Replication
Server is authoritative. Clients send input at tick rate. Hitscan and splash resolve on the server. Bots only exist on the server and replicate as normal players with the bot flag.

## Resilience
- 8s client timeout
- Join rejected at max players
- Disconnect removes the pawn; bots remain
- Rejoin is a fresh JoinRequest (new id)
- Intentionally no NAT punchthrough, steam, or EOS

## Anti-cheat (LAN)
Trust LAN operators. Server ignores input from unknown addresses, clamps move vectors, and rejects friendly fire in team modes. There is no kernel anti-cheat and no phone-home.
