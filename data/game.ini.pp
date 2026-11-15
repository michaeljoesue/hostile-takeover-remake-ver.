#include <res.h>

[General]

[Fonts]
kifntDefault=standardfont.fnt
kifntShadow=shadowfont.fnt
kifntButton=buttonfont.fnt
kifntTitle=titlefont.fnt
kifntEcom=standardfont.fnt
kifntHud=hudfont.fnt

// TrueType fonts (in data/fonts) drawn in place of the bitmap fonts above:
// font.ttf [size scale [extra pixels between letters]]. The bitmap font still
// sets the line height, baseline, colors and shadow. Remove a line to use the
// bitmap font.
[VectorFonts]
kifntDefault=saira-semicondensed-500.ttf 0.92
kifntShadow=saira-semicondensed-500.ttf 0.92
kifntButton=saira-600.ttf
kifntTitle=orbitron-700.ttf
kifntEcom=saira-semicondensed-500.ttf 0.92
// kifntHud stays a bitmap font: its letters are the credits and power meter
// graphics
