# Changelog

## [1.0.1] 2026-10-05

- Tire damage starts at higher speeds. Low: wear above 20 km/h, the most from 30 km/h (1.0.0: 15 and 25). Reduced: wear above 35 km/h, the most from 45 km/h (1.0.0: 25 and 35).
- A `TirePressure.ini` that 1.0.0 wrote keeps its own numbers. To get the new ones, set them in the settings tab or delete the file.
- The mod no longer needs one exact build of the game's exe. At its start it finds its places in the game's code and checks how the game's objects are laid out, and it stands down when anything is missing. That is meant for the Epic Games Store version, which has not been tested, and for game updates that leave that code alone.

## [1.0.0] 2026-10-04

- Four tire pressure modes for the truck being driven: Low, Reduced, Normal and Increased. A mode scales the grip by the ground under each wheel (dirt, gravel, sand, rock, asphalt, mud), the tire's radius, fuel use while moving and steering speed.
- A tire flattens as far as the game can draw it, and the truck then rolls as much slower as a real flattened tire would.
- Soft tires driven too fast take damage, with a warning on screen that shows how worn they are. A worn out tire goes flat. It is the game's own wheel damage and is repaired like any other.
- Expeditions' Tire Inflation System panel, drawn in the game's font through ReShade. LB + d-pad on a pad, F3 on the keyboard.
- A Tire Inflation System tab in ReShade's overlay edits every setting while the game runs and saves it to `TirePressure.ini`.
- Vanilla balance, base grip and an asphalt floor for tires that grip too much or too little as they come.
- `version.dll`, a loader for `.asi` files in the game folder.
