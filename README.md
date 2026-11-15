# hostile-takeover-remake-ver.

Remake of [spiffcode/hostile-takeover](https://github.com/spiffcode/hostile-takeover) with new modern engine, full upstream history included.

This version of the game is **Windows 11 (64 bit) only** and **single player
only**. Compared to the original repository:

- The iOS, Android, macOS, Linux, Native Client, Palm OS and Windows Mobile
  ports and their build files are gone. The game runs on SDL3 (`game/sdl`)
  with a small Windows host layer, and builds with CMake and Visual Studio.
- Multiplayer, the multiplayer server (`server`, `stats`, `syncerror`), chat,
  accounts and login, the leaderboard, the forums link, add-on mission pack
  downloads, copy protection (DRM), the demo/purchase screens and the related
  code have been removed. The multiplayer maps (`M_*`) are still built into
  the game data, but the single-player menus don't list them.
- Story and challenge missions, saved games, options and help are unchanged.

### Modern engine

The engine has been updated from its 2007 handheld roots:

- **SDL3 with GPU presentation.** The game's 8 bit frame is uploaded as a
  palettized texture and scaled by the GPU with sharp pixel-art filtering,
  letterboxed to keep its shape. Vsync paces the frame rate.
- **Any resolution.** Choose the game resolution in Options > Display (from
  800x600 up to your desktop size; larger shows more of the map). The
  recommended one fits your screen, divided down on very high resolution
  screens so the art isn't tiny (4K runs at 1920x1080, scaled 2x).
  High-DPI aware.
- **Fullscreen or windowed**, resizable window; Alt+Enter or F11 switches.
- **Smooth motion.** The simulation still runs at its original fixed rate
  (so gameplay is unchanged), but units and projectiles are drawn
  interpolated between updates at the display's refresh rate. Keyboard
  scrolling is continuous instead of a tile at a time, and the map scrolls
  when the mouse is at the screen edge in fullscreen. Options > Display >
  Smooth Motion turns it off.
- **Higher limits.** Up to 4096 game objects (was 768), 500 units and 300
  structures for the player (was 88 and 55), 600 shots and explosions, 300
  scorch marks and 600 scenery objects on the map at once (was 50, 30 and
  100), build queues of 50 (was 10), and a larger graphics cache. The
  computer players keep their original limits so the missions keep their
  original difficulty.
- **Better sound.** Effects are mixed in 16 bit on 16 channels (was 8 bit,
  4 channels, averaged so effects got quieter as more played) with soft
  clipping, and resampled by SDL3 to the output device's rate (typically
  48 kHz stereo). The volume slider works.
- The full Options menu (Game, Sound, Performance, Color, Display) is back.

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
  SDL3 3.4 from GitHub and builds it as a static library. If you'd rather
  use your own SDL3 (for example from vcpkg), CMake uses it when
  `find_package(SDL3)` finds it, and copies `SDL3.dll` next to the game.

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
- Arrow keys, the mini map, or the mouse at the screen edge (fullscreen)
  scroll the map.
- **Menu** (bottom left) has save, load, options and help.
- Alt+Enter or F11 switches between fullscreen and a window; numpad + and -
  change the game speed.

Saved games, settings and mission progress are stored in
`%APPDATA%\Spiffcode\HostileTakeover`. If you quit during a mission, it is
saved and restored the next time the game starts.

## Game data

All art, sound, levels and UI layouts are in two data files in `game`:
`htdata832.pdb` (8 bit color, 32 pixel tiles) and `htsfx.pdb` (sounds). Both
are checked in, so you only need to rebuild `htdata832.pdb` if you change
levels, art, the forms in `data\forms.ini.pp`, `data\GobTemplates.ini.pp`,
the help text or `game\wistrings.h`.

### Updating menus, unit stats and help text

After editing `data\forms.ini.pp`, `data\game.ini.pp`,
`data\GobTemplates.ini.pp` or `data\help.txt`, update the data file with:

```bat
python data\update_text.py
```

This recompiles those files and replaces them in `game\htdata832.pdb`,
keeping everything else. It only needs Python and the CMake build (for
`inicrunch.exe`), not the .NET tools.

### Rebuilding htdata832.pdb

Requirements: [Python 3](https://www.python.org/downloads/) and the CMake
build above (it builds `inicrunch.exe`, which the data build uses). The
content tools in `bin` run on the .NET Framework 4.8 that is part of
Windows 11; .NET Framework 3.5 isn't needed.

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
(**AniMax**, `bin\AniMax.exe`). They are .NET Framework 4.8 programs; the
source of each is in the folder of the same name (and `inicrunch`, which is
C++, is built by CMake with the game). `legacy\docs` has the original design
documents and the mission authoring guide, `AuthorKit.chm`.

### Building the tools

All the C# tools are in one solution, `HostileTakeoverTools.sln`, with
SDK-style projects targeting .NET Framework 4.8. Building it puts the tools
straight into `bin`, where the data build and M expect them.

Requirements: Visual Studio 2022 with the **.NET desktop development**
workload (it includes the .NET Framework 4.8 targeting pack).

- **In Visual Studio:** open `HostileTakeoverTools.sln` and build (Release).
- **From a Developer Command Prompt:**

  ```bat
  msbuild HostileTakeoverTools.sln -restore -p:Configuration=Release
  ```

- **With the game:** configure CMake with `-DHT_BUILD_TOOLS=ON` and the
  `tools` target builds the solution as part of the game build.

Use Visual Studio's MSBuild rather than `dotnet build`: the forms of M,
AniMax and Schemer embed images and other non-text resources, which the
.NET SDK's own resource compiler can't embed for .NET Framework programs
(the other tools build either way). The shared settings are in
`Directory.Build.props`.

The AED animation script editor (`aed`) uses JScript.NET and still has its
original Visual Studio 2003 project; it isn't part of the solution.

### Making missions with M

M needs the .NET Framework 4.8, which is part of Windows 11. Start
`bin\m.exe`, and open a mission (`.ld`) from the `data` folder or make a new
one with File > New.

- **Misc > Run (F5)** test-plays the mission: M checks it, exports it to a
  temporary mission pack and starts the game straight into it with
  `HostileTakeover.exe -pdb <pack> -l <level>`. M looks for the game next to
  itself and in `build\bin\<config>`, and asks where it is otherwise (the
  answer is saved as `GamePath` in `bin\m.ini`).
- **File > Export Mission Pack** saves one or more open missions as a mission
  pack (`.pdb`). Copy it into a `Missions` folder next to
  `HostileTakeover.exe`, or into `%APPDATA%\Spiffcode\HostileTakeover\Missions`,
  and its missions appear in the game under **Play > Custom**. Packs can be
  shared with other players this way.
- From the command line, `bin\mcl.exe -pack out.pdb mission.ld [...]` makes a
  mission pack too.

To change the built-in missions instead, edit their `.ld` files and rebuild
the data file as above. The editor's own help (Help in M, `m\m.chm`) and
`legacy\docs\AuthorKit.chm` describe the old AuthorKit's `run.bat`; Run (F5)
replaces it.

M is built by `HostileTakeoverTools.sln` (projects `m`, `mgui`, which
makes `m.exe`, and `mcl`).
