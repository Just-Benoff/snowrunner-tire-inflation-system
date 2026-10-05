# SnowRunner Tire Inflation System

Tire pressure you change while driving, as in Expeditions: A MudRunner Game.

The truck you drive gets four pressure modes: Low, Reduced, Normal and Increased. Let air out and the tires flatten, grip harder on dirt, sand, rock and in mud, and lose grip on asphalt. The truck also uses more fuel and steers slower. Increased is the road mode: more grip on asphalt, less everywhere else, less fuel. Soft tires driven too fast take damage until they go flat. You pick the mode on Expeditions' panel, drawn in SnowRunner's font, with the pad or a key.

![The panel in Low, Reduced, Normal and Increased, and as the keyboard shows it](docs/panel.png)

## What you need

- SnowRunner on Steam, the game version of 22 July 2026. On any other version the mod writes that into its log and does nothing.
- For the panel and the settings tab: ReShade 6.8.0 or newer, the build "with full add-on support". Without ReShade the key still changes the pressure, with beeps in place of the panel.

ReShade has its own setup. On reshade.me take the download "with full add-on support", start it, pick `SnowRunner.exe` in `Sources\Bin` and then DirectX 10/11/12. No effects are needed. The mod has no ReShade file of its own to copy: `TirePressure.asi` finds ReShade when the game starts. In the game the Home key opens ReShade's overlay.

## Install

1. Close the game.
2. Copy `version.dll` and `TirePressure.asi` into the game's `Sources\Bin` folder, next to `SnowRunner.exe`.
3. Start the game.

`version.dll` is a small loader: it loads every file in that folder whose name ends in `.asi`. If you already use an ASI loader (other `.asi` mods work in that folder), copy only `TirePressure.asi` and leave your loader as it is. If another mod's `version.dll` is there and it is not an ASI loader, rename that one to `version_chain.dll` first, and this loader passes everything on to it.

To update, copy the two new files over the old ones. Keep only one `.asi` file of this mod in the folder: a second copy under another name stands down and says so in the log.

## Use

With a pad (any pad Windows sees as an Xbox pad):

- LB + d-pad down opens the panel.
- LB + d-pad left or right lowers or raises the pressure. With the panel closed, it opens the panel one step lower or higher.
- While the panel is open the d-pad works without LB, A confirms and B closes the panel unchanged.

With the keyboard, F3 opens the panel and steps the pressure down, from Low round to the highest mode.

A choice left alone confirms itself after 15 seconds. The tires then take 3 seconds to deflate or inflate, and the mod beeps: once for Normal, twice for Reduced, three times for Low, one high beep for Increased.

The mode belongs to the truck you drive: another truck takes the current mode when you get in. Every game start begins at Normal.

## The modes

Multipliers against the tire's own values:

| | Low | Reduced | Increased |
|---|---|---|---|
| Grip on dirt | x3.5 | x3.0 | x0.9 |
| Grip on gravel | x2.75 | x2.4 | x0.95 |
| Grip on sand | x4.0 | x3.4 | x0.85 |
| Grip on rock | x3.5 | x3.0 | x0.9 |
| Grip on asphalt | x0.85 | x0.95 | x1.3 |
| Grip in mud | x1.3 | x1.15 | x0.85 |
| Tire radius | up to 10 cm less | up to 6 cm less | unchanged |
| Fuel use while moving | x1.5 | x1.25 | x0.85 |
| Steering speed | x0.7 | x0.85 | x1.1 |

Each wheel takes the grip for the ground under it, so a truck with two wheels on a gravel road and two in sand grips differently left and right. Grip stops at 10, as in Expeditions.

The game draws the flattened tires itself, and only so flat. Where 10 cm would put the tread under the ground, both steps shrink by the same share: a scout on 94 cm tires gets 6.7 and 4 cm. A flattened tire rolls a little less far per turn, as a real one does: that scout loses about 5% of its speed at Low and 3% at Reduced.

## Tire damage

Soft tires wear when the truck is too fast for them:

| | Low | Reduced |
|---|---|---|
| Wear starts above | 20 km/h | 35 km/h |
| Most wear from | 30 km/h | 45 km/h |
| Damage per wheel | 4 to 8 | 2 to 6 |
| Every | 7 seconds | 10 seconds |

A SnowRunner wheel takes about 50 damage. While the truck is too fast a warning shows at the top left with the most worn tire's damage in percent, and a low beep marks each hit. A worn out tire goes flat. This is the game's own wheel damage: the game's wheel icon turns red for a flat tire, the damage stays in the save, and it is repaired like any other damage. The numbers start from those of Expeditions' off-road tires, with twice the damage at lower speeds, as SnowRunner's trucks are slower off the road.

## Settings

With ReShade, the overlay has a Tire Inflation System tab. It shows the truck's tires, the ground under the first wheel and the speed, and it edits every setting while you play:

- Every number in the two tables above, and tire damage on or off.
- Whether flattened tires roll like real ones. On by default. Off, the truck loses about three times as much speed.
- Vanilla balance: tires that grip better than every vanilla tire on ground, asphalt and mud at once (some modded trucks have them) come down to the best vanilla tire of their kind. Vanilla tires stay as they are. Off by default.
- Base grip for ground, asphalt and mud: scales every mode, Normal too.
- Asphalt floor: every tire grips at least this much on paved ground. Off by default.
- The key, the pad buttons, the panel's size, the time until a choice confirms itself, how long a pressure change takes, and the beep.

Changes count at once and are saved to `TirePressure.ini` in the same folder. Without ReShade, edit that file: the game writes it with the defaults and a short guide on its first start.

## Remove

Delete `TirePressure.asi`, `TirePressure.ini`, `TirePressure.log`, `version.dll` and `AsiLoader.log` from `Sources\Bin`. If you renamed another mod's `version.dll` to `version_chain.dll`, rename it back.

## Notes

- The mod changes values in the running game's memory. It changes no game file.
- Trailers keep their tires as they are.
- The pad's panel buttons are taken from the game only while you drive a truck. In menus they are the game's.
- Co-op has not been tested.
- After a game update the mod does nothing until a version for the new game is out.
- If something does not work, `TirePressure.log` in `Sources\Bin` says what the mod found and did.

## Build

Windows with Visual Studio 2022 (C++ desktop tools).

```
build.bat
test.bat
```

`build.bat` makes `out\version.dll` and `out\TirePressure.asi`. `test.bat` runs the offline tests: the loader on a test `.asi`, the loader with a copy of itself as its chain file, the loader behind another ASI loader that had the `.asi` first, and the mod's own checks (every setting through the ini and back, the flattening and gear numbers). `test\build_preview.bat` builds a program that draws the panel, the warning and the settings tab into PNG files without the game and checks the pad binding window. It needs the Dear ImGui sources, see the file.

The mod runs only on the builds of the game's exe that it knows, as a few of its addresses differ from build to build. They are in the table `kBuilds` in `src\tire_pressure.cpp`. `node tools\find_build.js <exe>` finds them in an exe and prints the row, or says what it did not find.

## How it works

A thread in the game process reads and writes the game's own values 20 times a second, through `ReadProcessMemory` and `WriteProcessMemory` on its own process, so an object the game frees under it gives an error and no crash. No game code is patched. Three of the game's pointers are pointed at the mod: its two pointers to `XInputGetState` and one entry of its import table.

- Grip: the asphalt and mud grip of each wheel, and the truck's list of ground grip values, one per wheel. The game marks each wheel every frame with the kind of ground under it (gravel, sand, hard, paved), and the mod picks the factor from that.
- Flattening: the radius of the wheel's collision cylinder, held to what the game's wheel shader can draw flat.
- Speed: in the game the wheel is its collision cylinder, so a flattened tire would lose about three times the speed a real one does, whose belt keeps its length. The mod raises the truck's gear speeds by the difference.
- Fuel and steering: the truck's fuel consumption values and its steering speed.
- Damage: the game's damage data for each wheel. The game's truck update asks Windows once a frame which window is in front. That import table entry leads through the mod, which deals the damage at that moment, on the game's own thread, and then runs the game's damage update. The update makes a worn out tire flat, mends a repaired one and sums the truck's damage.
- Pad: the game keeps two pointers to `XInputGetState`. The mod points both at a filter that hides the panel's buttons from the game while the panel uses them.
- Panel: drawn through ReShade's add-on overlay. The text is baked from the game's own font files at run time.

## Licence and credits

GNU General Public License v3.0 (GPL-3.0-only), see `LICENSE`.

The tire inflation system, its panel and its numbers are from Expeditions: A MudRunner Game by Saber Interactive. The panel is drawn through ReShade by Patrick Mours (crosire) with Dear ImGui by Omar Cornut, and its text is baked with stb_truetype by Sean Barrett. Their notices are in `THIRD_PARTY_NOTICES.md`. The asphalt floor follows an idea by Naybour.

SnowRunner and Expeditions are games by Saber Interactive. This project is not affiliated with Saber Interactive or Focus Entertainment and contains no game files.
