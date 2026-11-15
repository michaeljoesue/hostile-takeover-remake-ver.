# hostile-takeover-remake-ver.

Remake of [spiffcode/hostile-takeover](https://github.com/spiffcode/hostile-takeover) with new modern engine, full upstream history included.

This version of the game is **Windows 11 (64 bit) only** and **single player
only**. Compared to the original repository:

- The iOS, Android, macOS, Linux, Native Client, Palm OS and Windows Mobile
  ports and their build files are gone. The game runs on the SDL2 layer in
  `game/sdl`, with a small Windows host layer, and builds with CMake and
  Visual Studio.
- Multiplayer, the multiplayer server (`server`, `stats`, `syncerror`), chat,
  accounts and login, the leaderboard, the forums link, add-on mission pack
  downloads, copy protection (DRM), the demo/purchase screens and the related
  code have been removed. The multiplayer maps (`M_*`) are still built into
  the game data, but the single-player menus don't list them.
- Story and challenge missions, saved games, options and help are unchanged.

## About

*Hostile Takeover* is the open source release of the wildly popular mobile Real Time Strategy game [*Warfare Incorporated*](http://www.warfareincorporated.com/). Warfare Incorporated's developers, grateful for all the contributions of the open source community, are delighted to give something back.

#### Legal Stuff
*Hostile Takeover* and *Warfare Incorporated* are registered trademarks of *Spiffcode, Inc* the developers of Warfare Incorporated. *Hostile Takeover* is copyrighted 2004-2014 by Spiffcode, Inc. The Hostile Takeover source code is made available under the [BSD open source license](LICENSE.txt).

## Building

### Prerequisites

- Windows 11 (or 10), 64 bit
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the
  **Desktop development with C++** workload (includes CMake)
- An internet connection the first time you configure: the build downloads
  SDL2 2.30 from GitHub and builds it as a static library. If you'd rather
  use your own SDL2 (for example from vcpkg), CMake uses it when
  `find_package(SDL2)` finds it, and copies `SDL2.dll` next to the game.

### Building the game

**In Visual Studio:** File > Open > Folder..., choose this folder, pick the
*Visual Studio 2022 (x64)* configure preset and *Release*, then Build > Build
All. Set `HostileTakeover.exe` as the startup item to run it.

**From a Developer Command Prompt:**

```bat
cmake --preset vs2022
cmake --build --preset release
```

The game is built to `build\bin\Release\HostileTakeover.exe`, with the game
data (`htdata832.pdb`, `htsfx.pdb`) copied next to it. To make a folder with
just what's needed to play:

```bat
cmake --install build --config Release --prefix dist
```

**Debug** builds define `DEV_BUILD`: they show "DEV BUILD" on the title
screen, log to Visual Studio's Output window, and have a test options menu
(click the power indicator during a mission).

## Playing

Run `HostileTakeover.exe`. Choose **Play** and start with Story Mission
"M0: Introduction", which teaches the controls.

- Click a unit to select it; click the ground to move, or an enemy to attack.
- Drag a box to select several units.
- Click and hold on one of your units or buildings for its menu.
- Arrow keys or the mini map scroll the map.
- **Menu** (bottom left) has save, load, options and help.

Saved games, settings and mission progress are stored in
`%APPDATA%\Spiffcode\HostileTakeover`. If you quit during a mission, it is
saved and restored the next time the game starts.

## Game data

All art, sound, levels and UI layouts are in two data files in `game`:
`htdata832.pdb` (8 bit color, 32 pixel tiles) and `htsfx.pdb` (sounds). Both
are checked in, so you only need to rebuild `htdata832.pdb` if you change
levels, art, the forms in `data\forms.ini.pp`, `data\GobTemplates.ini.pp`,
the help text or `game\wistrings.h`.

### Rebuilding htdata832.pdb

Requirements: [Python 3](https://www.python.org/downloads/) and the CMake
build above (it builds `inicrunch.exe`, which the data build uses). The
content tools in `bin` are .NET Framework 3.5 programs; if Windows asks to
turn on .NET Framework 3.5 the first time one runs, allow it.

```bat
python data\build.py --clean
```

This writes `game\htdata832.pdb`; rebuild the game (or copy the file next to
`HostileTakeover.exe`) to use it. Missions are listed in `data\build.py`
(`GRASSY_LEVELS` and `DESERT_LEVELS`); add new `.ld` files there. Story
missions are named `S_*` and challenge missions `C_*`.

## Tools

The `bin` directory contains the content tools used to build the data file,
and the GUI editors for maps and missions (**M**, `bin\m.exe`) and animations
(**AniMax**, `bin\AniMax.exe`). The source of each tool is in the folder of
the same name (C# projects for Visual Studio, plus `inicrunch`, which is
built by CMake). `legacy\docs` has the original design documents and the
mission authoring guide, `AuthorKit.chm`.

To edit a mission, open its `.ld` file from the `data` folder in M, then
rebuild the data file as above.
