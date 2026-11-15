# Game fonts

The game draws its text with these TrueType fonts (see `[VectorFonts]` in
`data/game.ini.pp`). Both are free under the SIL Open Font License 1.1; the
licenses are next to them. The files are the unmodified Google Fonts
releases, renamed to fit the game's 29 character file names.

- Saira Semi Condensed Medium (Omnibus-Type), for text:
  `saira-semicondensed-500.ttf`
- Saira SemiBold (Omnibus-Type), for buttons: `saira-600.ttf`
- Orbitron Bold (The League of Moveable Type), for titles: `orbitron-700.ttf`

To use a different font, put its `.ttf` here, name it in `[VectorFonts]`
and run `python data\update_text.py` (or the full `build.py`). Remove a line
from `[VectorFonts]` to go back to that font's original bitmap glyphs.
