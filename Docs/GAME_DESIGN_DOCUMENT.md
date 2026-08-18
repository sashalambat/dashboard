# SBS Wars — Game Design Document

## Fantasy
A LAN-party FPS set in the industrial war between the **SBS Alliance** (heat-stained foundry armor, analog radios, brass and ember) and **Cyber Dominion** (cold composite plates, vocoder callouts, cyan holography).

## Pillars
1. Instant local play — host or join in seconds, no accounts.
2. Readable gunfights — recoil, reload, and class roles beat ability spam.
3. Bots that fill a room — a match is always populated.
4. Objective variety on every map.

## Factions
| | SBS Alliance | Cyber Dominion |
| --- | --- | --- |
| Armor | Layered foundry plate, amber visor | Smooth carbon, cyan visor |
| Voice | Gravel radio, analog static | Vocoder staccato |
| Color | 255, 150, 40 | 40, 220, 255 |

## Classes
| Class | HP | Speed | Primary | Secondary | Role |
| --- | --- | --- | --- | --- | --- |
| Assault | 100 | 4.7 | SBS Assault Rifle | Grenade | Flex |
| Heavy | 150 | 3.5 | Machine Gun | Rocket Launcher | Anchor |
| Recon | 80 | 5.6 | Sniper Rifle | Railgun | Pick / radar |
| Engineer | 110 | 4.4 | Combat Shotgun | Pulse Cannon | Close / repair |
| Medic | 90 | 4.9 | Plasma Rifle | Grenade | Sustain |

Upgrades (F1 in match, 0–5): magazine, fire interval, and move speed scale slightly per level.

## Weapons
All weapons implement recoil, reload or heat, audio, and replicated fire events.

Hitscan: AR, Shotgun, MG, Sniper, Railgun.
Projectile + splash: Rocket, Plasma, Pulse, Grenade.

## Modes
- Deathmatch — 30 frags / 8:00
- Team Deathmatch — 50 team score / 10:00
- Capture The Flag — 3 captures / 12:00
- Domination — 200 tickets / 12:00, three points
- King Of The Hill — 100 hill score / 8:00
- Spectator — free roam after deploy skip or death cam

## Maps
Every map has DM/TDM spawns, bot navgrid, flags, three domination nodes, and a hill.

1. SBS Foundry — industrial lanes
2. Arctic Base Zeta — long ice sightlines
3. Orbital Platform Seven — cross halls
4. Crimson Desert — open dunes + hazard pit
5. Cyber Core — nested server rooms
6. Titan Factory — dual assembly lines

## Bots
Recruit / Veteran / Elite differ in reaction, accuracy, aim speed, aggression, and flank chance. Bots pathfind with A*, contest objectives, and reload when dry.

## UI
Main Menu, Options, Settings, LAN Browser, Class Select, Spawn Menu, HUD, Scoreboard, Spectator HUD, Match Results, Statistics.

## Save
`~/.sbswars/settings.ini` stores name, class, faction, graphics, keybind scalars, volume, LAN favorites, and career stats.
