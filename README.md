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
  letterboxed to keep its shape. The frame rate isn't limited (no vsync).
- **Any resolution.** Choose the game resolution in Options > Display (from
  800x600 up to your desktop size; larger shows more of the map). The
  recommended one is 600 lines with your screen's shape, scaled up to fill
  the window like the web version, so the menus fill the screen as designed
  (1.8x on a 1080p screen, 3.6x on 4K). High-DPI aware.
- **Sharp text at any size.** Text is drawn with anti-aliased TrueType fonts
  on top of the scaled frame, at the window's own pixel resolution, so it
  stays crisp however far the game is scaled up (true 4K text on a 4K
  screen). Text keeps its place in the stacking order: whatever the game
  draws over it later still covers it.
- **Fullscreen or windowed**, resizable window; Alt+Enter or F11 switches.
- **Modern RTS controls** in place of the original one-button Palm controls:
  right-click orders, box and Shift selection, control groups, attack-move,
  stop, middle-drag panning and wheel scrolling (see Playing below).
- **Smooth motion.** The simulation still runs at its original fixed rate
  (so gameplay is unchanged), but units and projectiles are drawn
  interpolated between updates in every frame, as many frames a second as
  the PC can draw. Keyboard scrolling is continuous instead of a tile at a
  time, and the map scrolls when the mouse is at the edge of the screen or
  window. (Sprite animations still have the frames the artists drew.)
  The edge of the fog of war fades out gradually instead of in tile-sized
  steps. The art is still 8-bit, but the frame is shown in true colour:
  the fog's fade and the mini map are blended in full colour on top, with
  no dithering. Options > Display > Show frames per second shows the frame rate.
- **Higher limits, scaled to the map.** On a 64x64 map: 300 structures and
  500 units for the player, the same for the computer players together
  (originally 55/88 and 72/117), 600 scenery objects, 300 scorch marks and
  600 shots and explosions at once. Bigger maps get proportionally more, up
  to about 8x on the largest maps. Build queues hold 50 (was 10), and the
  graphics cache is larger.
- **Better sound.** Effects are mixed in 16 bit on 32 channels (was 8 bit,
  4 channels, averaged so effects got quieter as more played) with a
  limiter, and resampled by SDL3 to the output device's rate (typically
  48 kHz stereo). In big battles the same sound starting many times at
  once plays once, and quieter (further away) sounds give way to louder
  ones; alerts and speech are never cut off by battle noise. The volume
  slider works.
- **Positional sound.** Fights, shots, explosions and buildings being hit
  are panned left or right by where they happen relative to the middle of
  the view, and are quieter off screen, so you can hear where a fight is.
  The panning is strong: at the edge of the view a sound is almost only
  in one ear.
- **Sound groups.** Options > Sound has a volume for each group, relative
  to the main volume: Effects (interface clicks), Battlefield (shots,
  explosions, hits, units dying), Unit Responses (units replying to you)
  and Menus & Alerts (build menus, buildings' select, build and ready
  sounds, alerts, credits, win / lose).
- **Five sides.** Side 1 is ACME and side 2 is OMNI (now bright red,
  #FF3333). There is a new side 5, Crimson (#DC143C), for missions to use.
  Allied sides share what they can see.
- **Bigger maps.** Missions can be up to 256x256 tiles (was 64x64).
- **All missions are unlocked** from the start.
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

Modern RTS controls:

| | |
|---|---|
| Left-click | Select a unit or building (enemies too, to look at them). Clicking one of your buildings also opens its menu. Clicking the ground deselects. |
| Unit build menus | Point at a unit to see what it is; left-click (or 1-9) orders one, right-click (or Shift+1-9) cancels one from the queue |
| Placing a building | After Build, the building follows the mouse: left-click puts it down (white tiles: it fits; red: it doesn't), right-click or Esc cancels |
| Drag a box | Select your fighting units in it; miners and defence towers are left out unless the box holds nothing else (Shift+drag adds to the selection) |
| Double-click, Ctrl+click | Select all your units of that type on screen |
| Shift+click | Add a unit to the selection, or take it out |
| Right-click | Move to the ground, attack an enemy, or open the menu of one of your units or buildings; cancels building placement |
| Shift+right-click | Queue a move or attack: units carry it out after their current orders (up to 16) |
| Ctrl+right-click | Force attack: attack the unit or building clicked, even your own or an ally's (Ctrl+Shift queues it). Destroying a friend isn't counted as a kill |
| A, then left-click | Attack-move: units fight enemies they meet, including defence towers in range (buildings without weapons are left alone), then carry on |
| Ctrl+Alt+right-click | Red Alert style: on one of your (or an ally's) units, the selected units guard it: they follow it and fight enemies and defence towers near it, and keep guarding a miner while it's inside a Processor. On an enemy: attack. On the ground: attack-move. Shift queues it |
| S | Stop |
| Ctrl+1 to 9 | Make the selection control group 1 to 9 (Shift+number adds to it) |
| 1 to 9 | Select a control group; press twice quickly to go to it |
| Space | Go to the last "under attack" alert |
| Esc | Close the open menu or dialog; otherwise cancel attack-move or placement, otherwise deselect |
| Middle-drag | Pan the map |
| Mouse wheel | Scroll the map up and down (Shift+wheel: sideways) |

The cursor shows what a click would do: a crosshair over an enemy (or any
unit while Ctrl is held) when you have units selected, a hand over a unit
or building, and a "no entry" sign over cliffs and walls the selected units
can't reach. Double-click (within half a second) selects all units of that
type on screen.
| Arrow keys, mouse at the edge, mini map | Scroll the map |

The mini map (bottom right) is about a third of the screen's height
whatever the map's size, and shows units moving smoothly. Its terrain is
drawn from the full size tiles scaled down and averaged in true colour, and
its fog fades smoothly.

- The bar at the bottom shows what's selected: one unit's name and health,
  or how many of each type.
- **Menu** (bottom left, or M) has save, load, options and help.
- Alt+Enter or F11 switches between fullscreen and a window; numpad + and -
  change the game speed.

Saved games, settings and mission progress are stored in
`%APPDATA%\Spiffcode\HostileTakeover`. If you quit during a mission, it is
saved and restored the next time the game starts. Games saved by versions
before side 5 and the bigger maps can't be loaded.

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

### Fonts

Text is drawn with anti-aliased TrueType fonts from `data\fonts` (Saira and
Orbitron, both under the SIL Open Font License), rendered by
[stb_truetype](https://github.com/nothings/stb) (`game\vendor`). The
`[VectorFonts]` section of `data\game.ini.pp` picks the font, size and
letter spacing for each of the game's fonts; the original bitmap fonts still
set the line height, colors and shadow, so every layout stays the same. To
change a font, put the `.ttf` in `data\fonts`, edit `[VectorFonts]` and run
`update_text.py`; delete a line to go back to the bitmap font.

### Rebuilding htdata832.pdb

Requirements: [Python 3](https://www.python.org/downloads/) and the CMake
build above (it builds `inicrunch.exe`, which the data build uses). The
content tools in `bin` run on the .NET Framework 4.8 that is part of
Windows 11 (.NET Framework 3.5 isn't needed), except `mcl.exe`, which needs
the [.NET 8 Desktop Runtime](https://dotnet.microsoft.com/download/dotnet/8.0)
like M (see below).

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
(**AniMax**, `bin\AniMax.exe`). M and its command-line version `mcl` are
.NET 8 programs, the others .NET Framework 4.8 programs; the
source of each is in the folder of the same name (and `inicrunch`, which is
C++, is built by CMake with the game). `legacy\docs` has the original design
documents and the mission authoring guide, `AuthorKit.chm`.

### Building the tools

All the C# tools are in one solution, `HostileTakeoverTools.sln`, with
SDK-style projects. M (`m`, `mgui`, `mcl`) and `TemplateExtractor`, which
uses M's code, target .NET 8 (`m\MFramework.props`); the others target .NET
Framework 4.8. Building it puts the tools straight into `bin`, where the data
build and M expect them.

Requirements: Visual Studio 2022 with the **.NET desktop development**
workload (it includes the .NET 8 SDK and the .NET Framework 4.8 targeting
pack).

- **In Visual Studio:** open `HostileTakeoverTools.sln` and build (Release).
- **From a Developer Command Prompt:**

  ```bat
  msbuild HostileTakeoverTools.sln -restore -p:Configuration=Release
  ```

- **With the game:** configure CMake with `-DHT_BUILD_TOOLS=ON` and the
  `tools` target builds the solution as part of the game build.

Use Visual Studio's MSBuild rather than `dotnet build` for the whole
solution: the forms of AniMax and Schemer embed images and other non-text
resources, which the .NET SDK's own resource compiler can't embed for .NET
Framework programs (M and the other tools build either way). The shared
settings are in `Directory.Build.props`.

### Making missions with M

M needs the [.NET 8 Desktop Runtime](https://dotnet.microsoft.com/download/dotnet/8.0)
(x64); Windows offers to download it the first time M starts if it's
missing, or install it with `winget install Microsoft.DotNet.DesktopRuntime.8`.
Start
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

Maps, sides and limits:

- **Map size.** In the level's properties (Settings > Level),
  Map Width and Map Height under Map Size set the editing area, from 16 to
  258 tiles. Bounds, the part the game gets, can be up to 256x256 tiles.
  Maps wider or taller than 64 tiles get a mini map with 1 pixel per tile.
- **Sides.** Side 1 (ACME), side 2 (OMNI), sides 3 and 4, and side 5
  (Crimson). Settings > Side 5 (Crimson) sets up side 5; triggers and units
  can use it like the others.
- **Galaxite regrowth.** Settings > Level > Galaxite Regrowth (in the
  Galaxite group): Off, Slow (a size step every 200 seconds) or Normal
  (every 100 seconds).
  Mined Galaxite grows back only where the mission put it, spreading from
  Galaxite that's left, and only to 60% of its starting size.
- **Limits.** M no longer stops a mission for exceeding the old limits. It
  only reports what the game itself can't handle: more than 1024 triggers per
  side, 64 switches or 64 areas, or more scenery objects (600) and units
  and buildings placed on the map (1600) than the game has room for. The
  last two grow with the map like the game's limits.
- **Unit group time limits.** Wait, Attack, Guard, Guard Vicinity and the
  new actions below take a number of seconds or Unlimited (set Unlimited
  to True in the action's properties): with no limit the group keeps at
  it until it's destroyed.
- **New unit group actions.**
  - *Protect Group*: each member guards a unit of another unit group
    (follows it, attacks enemies and enemy defence towers near it, keeps
    guarding a miner inside a Processor); when that unit is destroyed it
    moves on to another. Done when the other group is gone (once it has
    been created) or the time runs out.
  - *Protect In Area*: the group moves into the area, then attacks enemy
    units that come into it.
  - *Attack In Area*: like Attack, but only units in the area are
    targeted; with none there the group waits in the area.
- **Counters removed.** The game never had Counters (the Counter condition
  and Modify Counter action did nothing), so M no longer has the Counters
  menu or the Modify Counter action. Older missions still open; any Modify
  Counter actions in them are dropped. Hunt Units (trigger action) and Hunt
  Enemies (unit group action) are real game features and stay.

Missions (`.ld`) and tile sets (`.tc`) are saved in M's own file format
(`m\DocFormat.cs`). Files from older versions of M, which used .NET's
retired BinaryFormatter, still open, and are converted the next time they
are saved; `bin\mcl.exe -convert desert.tc mission.ld [...]` converts them
in one go (tile sets first).

To change the built-in missions instead, edit their `.ld` files and rebuild
the data file as above. The editor's own help (Help in M, `m\m.chm`) and
`legacy\docs\AuthorKit.chm` describe the old AuthorKit's `run.bat`; Run (F5)
replaces it.

M is built by `HostileTakeoverTools.sln` (projects `m`, `mgui`, which
makes `m.exe`, and `mcl`).
