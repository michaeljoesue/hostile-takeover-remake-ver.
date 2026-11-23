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
  right-click orders, box and Shift selection, attack-move (Ctrl+Alt+right-click),
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
  to the main volume: Interface (button and menu clicks, selecting a
  building), Battle & Base (shots, explosions, hits, units dying, plus
  credits counting, building repairs, miners unloading and a Dominion
  deploying), Unit Voices
  (units replying to you) and Announcements (buildings' build and ready
  sounds, new build options, alerts, win / lose).
- **HD buildings.** Every building, defence tower and wall is drawn with
  new high-resolution art over the 8-bit game, at the window's own pixel
  resolution: each picture is scaled down once to the window's size
  (averaged, so it doesn't shimmer) and recoloured to its side's colour.
  The game logic doesn't change, and every picture the artist drew plays,
  in game time (it follows the game speed and stops when the game is
  paused). The HQ's hatch follows the classic animation, its pauses
  included; the generator's fan, the Surveillance Center's dish and the
  R&D Center's panel, which have far more pictures than the classic
  frames, play their pictures at a steady pace of their own: at the
  default game speed (2.0) the fan turns in 0.6 s (60 pictures a second, a
  new one every frame on a 60 Hz screen), the dish sweeps round in 1.9 s
  and the panel goes out and back in 2.6 s. The Processor's door opens and closes through
  all its pictures for each miner, the damaged R&D Center's panel turns
  like the healthy one (the classic damaged one is still), and buildings
  without power stand still as before. Damaged and destroyed
  buildings have their own art. Buildings keep their place in the drawing
  order (units in front, selection boxes, health bars, smoke and the fog
  of war still cover them, the fog's soft edge included), and their
  classic shadows. The HQ's build menu shows the buildings' HD pictures
  too, in the player's colour. F9 switches between the HD art and the
  original pictures. The pictures are in `data\hd` (see Game data).
- **Command Center.** A building for missions to place, for any side: it
  has no functions (no power, it isn't sold) and can't be built. Its menu
  only repairs it, and the player's and computer players' auto repair
  repair it too; a Corporate Raider can capture it. It has the R&D Center's armour (250) and
  classic art; its HD art is the R&D Center's dish version, turning at the
  R&D Center's pace. Like any building it can be attacked and destroyed,
  and it counts as a building in triggers.
- **Walls are objects.** Each wall tile belongs to a side or is neutral
  (no one's), and can be damaged and destroyed: it then opens the way.
  Its picture joins the walls next to it, in its side's colour. Units
  never pick walls as targets on their own, whatever their side (nor do
  defence towers, the computer players or hunting units): a wall is
  attacked only when ordered to
  (right-click, or Ctrl+right-click on your own). Walls can be selected
  but not repaired or sold. Walls in missions made before are neutral.
- **Six sides.** Side 1 is ACME and side 2 is OMNI (both in their original
  colours, as are sides 3 and 4), and there are two new sides for missions
  to use: side 5, Crimson (#DC143C), and side 6, Dodger Blue (#1E90FF).
  Allied sides share what they can see.
- **Smarter units.** Pathfinding is a real A* search: units go around a
  lake instead of following its shore into dead ends, and paths are about
  15% shorter for about the same work. Long trips first plan a rough route
  through 16x16-tile rooms, then the detailed path only along it; a big
  search that doesn't fit in one update carries on in the next instead of
  stalling the game; and a destination that can't be reached at all (an
  island, an enclosed spot) is recognised up front, so units head for the
  nearest place they can get to. A group sent somewhere (yours, or the
  computer's unit groups) follows one shared path at the speed of its
  slowest unit and spreads out into formation at the end. Units blocked in
  a crowd wait for blockers that are moving, wait longer after each failed
  try, and after a few give up (or stop if they're nearly there) instead
  of jiggling and re-planning every update. Units pick targets by how fast
  they can destroy them (rockets go for vehicles, machine guns for
  infantry, badly damaged enemies get finished off), how dangerous they
  are and how far away, instead of just the nearest. Defence towers pick
  targets the same way, and since they can't move they keep checking: a
  tower switches to a better target that comes in range (or to one that
  shoots it) instead of staying on the first enemy it saw. (As before, a
  tower only aims on its own while its side has a Surveillance Center and
  enough power.)
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
| Buttons | Every button (menus, dialogs, the unit and building menus) shows its highlighted (white) box while the mouse is over it and acts the moment you click it |
| Unit and building menus | Left-click a button to choose it. Right-click anywhere closes the menu. The Sell / Abort "Are you sure?" box works the same way (right-click answers No) and can't be dragged |
| Build menus | The selection box and the info follow the mouse. Buildings: left-click starts placing it. Units: left-click orders one, right-click on it cancels one from the queue. Right-click anywhere else closes the menu |
| Placing a building | After you click a building in the menu, it follows the mouse: left-click puts it down (white tiles: it fits; red: it doesn't), right-click or Esc cancels |
| Drag a box | Select your fighting units in it; miners and defence towers are left out unless the box holds nothing else (Shift+drag adds to the selection) |
| Double-click, Ctrl+click | Select all your units of that type on screen |
| Shift+click | Add a unit to the selection, or take it out |
| Hover | Rest the mouse on any unit or building you can see (yours, an ally's or an enemy's) to show its name |
| Right-click | Move to the ground (or to where one of your units or buildings stands), attack an enemy; cancels building placement. With miners selected, right-click one of your Processors: they unload there. With a Dominion selected, right-click it: the Headquarters' outline follows the mouse (white: it fits; red: it doesn't); left-click sends the Dominion there to deploy; right-click or Esc cancels |
| Shift+right-click | Queue a move or attack: units carry it out after their current orders (up to 16) |
| Ctrl+right-click | Force attack: attack the unit or building clicked, even your own or an ally's (Ctrl+Shift queues it). Destroying a friend isn't counted as a kill |
| Ctrl+Alt+right-click | Red Alert style: on the ground, attack-move: units fight enemies they meet, including defence towers in range (buildings without weapons are left alone), then carry on. On an enemy: attack. On one of your (or an ally's) units, the selected units guard it: they follow it and fight enemies and defence towers near it, and keep guarding a miner while it's inside a Processor. Walls are neither guarded nor attacked this way (clicking a wall attack-moves to it), and attack-moving and guarding units never pick walls as targets. Shift queues it |
| S | Stop |
| Space | Go to the last "under attack" alert |
| Esc | Close the open menu or dialog; otherwise cancel placement, otherwise deselect |
| Middle-drag | Pan the map |
| Mouse wheel | Scroll the map up and down (Shift+wheel: sideways) |
| Arrow keys, mouse at the edge, mini map | Scroll the map |

Double-click (within half a second) selects all units of that type on
screen.

The mini map (bottom right) is about a third of the screen's height
whatever the map's size, and shows units moving smoothly. Its terrain is
drawn from the full size tiles scaled down and averaged in true colour, and
its fog fades smoothly.

- The bar at the bottom shows what's selected: one unit's name and health,
  or how many of each type.
- **Menu** (bottom left, or M) has save, load, options and help.
- Alt+Enter or F11 switches between fullscreen and a window; numpad + and -
  change the game speed. F9 switches between the HD buildings and the
  original pictures.

If the game crashes, or stops answering Windows ("Not responding") for 10
seconds, it adds a report to `HostileTakeover-report.txt` next to
`HostileTakeover.exe`: what happened and the call stack as addresses in
that build of the game. Send that file along when reporting it; with the
same build, `addr2line -f -C -e HostileTakeover.exe <address>` (or a
symbol lookup with `nm -C`) turns the addresses into function names.

Saved games, settings and mission progress are stored in
`%APPDATA%\Spiffcode\HostileTakeover`. If you quit during a mission, it is
saved and restored the next time the game starts. Games saved by versions
before side 6 can't be loaded.

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

### HD buildings

The HD pictures of the buildings, towers and walls are in `data\hd`
(`manifest.txt` and one `.hdp` pack per building), copied next to the game
by the build. They are made from the artist's sheets in `assets\hd` by:

```bat
pip install numpy pillow qoi
python tools\hd\build_hd.py
```

Each sheet's pictures are fitted onto the classic frames (in
`tools\hd\classic`), so a building covers the same ground and lines up with
its shadow. The R&D Center uses the panel version of its sheet (the classic
one turns a panel); the dish version is the Command Center's (`ccenter`,
bound as `CommandCenter.anir` through `HdAnimation` in GobTemplates). The
build menu shows each building's first healthy picture (towers turned
toward the viewer). Without the `hd` folder the game draws the original
pictures.

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

- **Edit > Undo (Ctrl+Z) / Redo (Ctrl+Y or Ctrl+Shift+Z)** step back and
  forward through changes to the open mission (tiles, units and buildings,
  Galaxite, areas, triggers, unit groups, switches, sides and settings), up
  to 100 steps. A mouse drag counts as one step. In a text box they undo
  the typing instead. The old Settings > Comments (a note stored in the
  .ld that the game never used) is gone.
- **Hold Space and drag with the left mouse button** to pan the map.
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
- **Sides.** Side 1 (ACME), side 2 (OMNI), side 3, side 4,
  side 5 (Crimson) and side 6 (Dodger Blue). Settings > Side 5 (Crimson) and
  Settings > Side 6 (Dodger Blue) set them up; triggers, unit groups, units,
  buildings and walls can use them like the others.
- **Command Center.** In the gob panel next to the R&D Center; exported as
  `kgtCommandCenter`, and offered in unit type lists.
- **Walls.** Place walls from the gob panel. Each wall has a Side (pick it
  in the panel's side list before placing, or in the wall's properties;
  a coloured band shows it, none for neutral) and a Health (percent).
  They are exported as objects (`kgtWall`) and still marked in the
  terrain map; older missions' walls are neutral at full health.
- **Galaxite regrowth.** Settings > Level > Galaxite Regrowth (in the
  Galaxite group): Off, Slow (a size step every 200 seconds) or Normal
  (every 100 seconds).
  Mined Galaxite grows back only where the mission put it, spreading from
  Galaxite that's left, and only to 60% of its starting size.
- **Limits.** M no longer stops a mission for exceeding the old limits. It
  only reports what the game itself can't handle: more than 1024 triggers per
  side, 64 switches or 256 areas, or more scenery objects (600) and units
  and buildings placed on the map (1600) than the game has room for. The
  last two grow with the map like the game's limits.
- **The computer leaves the neutral side alone.** Computer units and towers
  don't go after neutral units or buildings on their own; they attack them
  only when ordered to (an Attack, Hunt or Attack In Area action, or a
  trigger).
- **Guard actions keep their ground.** Computer units with the Guard or
  Guard Vicinity action (unit or unit group) guard the spot where they
  start it: they chase enemies that back off, but only up to 3 tiles past
  what they watch from that spot (their firing range for Guard, five tiles
  for Guard Vicinity), and go back to it when the fight is over. Guard Area
  units go after any enemy in the area they can reach; ones they can't
  reach no longer freeze them.
- **Trigger conditions: All (AND) or Any (OR).** On a trigger's Conditions
  tab, "Fire when" chooses whether every condition must be true (as
  always) or any one of them is enough. The trigger list shows
  "CONDITIONS (any one):" for the second kind. In the level file it is an
  `M=1` line after the trigger's `T=` line (no line: All).
- **Reorder a unit group's units.** The up and down arrows beside the unit
  group's Units list move the selected unit type up or down.
- **Guard Area holds its ground, but fights back.** A Guard Area unit chases
  an enemy that leaves its area only while that enemy (or one near a
  friend) is shooting at it, instead of standing there taking fire from
  just outside the area. Protect Area hands its units the Guard Area order
  as soon as each one has stopped or is inside the area (before, it waited
  for every unit to stand idle, so while there was fighting at the area the
  group only shot what was in range).
- **Looping unit groups can't hang the game.** A "loop forever" group starts
  its actions over at the next update; before, a group whose actions all
  finish at once (Set Switch, Mine) started over again and again within one
  update and the game stopped responding.
- **Create Random Unit Group of Side.** A new trigger action: like Create
  Random Unit Group, but picks only among the random groups belonging to
  the chosen side(s). Create Random Unit Group still picks from every
  side's random groups, so with several computer sides use this one to keep
  each side's random attacks its own.
- **Comments removed.** M no longer offers Comment conditions and actions.
  Ones already in a level stay, and can be edited or deleted.
- **Copy unit groups.** Unit Groups > Copy makes a new group with
  everything the selected one has, named "<name> copy". The group list
  shows long names whole (it scrolls sideways; hover for the full name),
  and so do the pick lists of triggers and actions.
- **Picking on the map.** Clicking picks areas first, then units, buildings
  and other objects, then templates (roads and other terrain pieces),
  whatever was placed last.
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
