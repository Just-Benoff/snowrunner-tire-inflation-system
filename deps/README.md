# Headers for the ReShade add-on build

`reshade\include` holds the add-on API headers of ReShade 6.8.0 (add-on API version 20), BSD-3-Clause, see `reshade\LICENSE.md`.

`imgui` holds `imgui.h` and `imconfig.h` of Dear ImGui 1.92.5, the version that ReShade release is built with, MIT, see `imgui\LICENSE.txt`. Headers only: ReShade hands add-ons the functions through a table, so no ImGui source is compiled into the add-on.

`stb` holds `imstb_truetype.h` (stb_truetype 1.26) and `imstb_rectpack.h` (stb_rect_pack 1.01) as Dear ImGui ships them (warning fixes only, marked [DEAR IMGUI]), public domain or MIT, see the end of each file. The panel bakes its glyph atlas with them from the game's own font files, which are read from the game folder at run time and never shipped.

When moving to a newer ReShade, replace both sets together. ReShade checks the API version when the add-on registers, and `reshade_overlay.hpp` checks the Dear ImGui version at compile time.
