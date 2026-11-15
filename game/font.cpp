#include "ht.h"

// stb_truetype renders the vector fonts; compiled privately into this file
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "vendor/stb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <math.h>
#include <algorithm>
#include <vector>

namespace wi {

#define kcchFont 256

Font *LoadFont(char *pszFont)
{
	Font *pfnt = new Font;
	Assert(pfnt != NULL, "out of memory!");
	if (pfnt == NULL)
		return NULL;
	if (!pfnt->Load(pszFont)) {
		delete pfnt;
		return NULL;
	}
	return pfnt;
}

Font::Font()
{
	m_pvf = NULL;
	m_pfnth = NULL;
	m_mpchpbCodeEven = NULL;
	m_mpchpbCodeOdd = NULL;
	m_nGlyphOverlap = 0;
	m_nLineOverlap = 0;
    m_cxEllipsis = 0;
}

static void DeleteVectorFont(VectorFont *pvf);

Font::~Font()
{
	DeleteVectorFont(m_pvf);
	if (m_pfnth != NULL)
		gpakr.UnmapFile(&m_fmap);

	for (int ch = 0; ch < kcchFont; ch++) {
		if (m_mpchpbCodeEven != NULL) {
			if (m_mpchpbCodeEven[ch] != NULL)
				gmmgr.FreePtr(m_mpchpbCodeEven[ch]);
		}
		if (m_mpchpbCodeOdd != NULL) {
			if (m_mpchpbCodeOdd[ch] != NULL)
				gmmgr.FreePtr(m_mpchpbCodeOdd[ch]);
		}
	}
	if (m_mpchpbCodeEven != NULL)
		gmmgr.FreePtr(m_mpchpbCodeEven);
	if (m_mpchpbCodeOdd != NULL)
		gmmgr.FreePtr(m_mpchpbCodeOdd);
}

bool Font::Load(char *pszFont)
{
	m_pfnth = (FontHeader *)gpakr.MapFile(pszFont, &m_fmap);
	if (m_pfnth == NULL)
		return false;

	byte *apbT[kcchFont];
	memset(apbT, 0, sizeof(apbT));

	m_mpchpbCodeEven = (byte **)gmmgr.AllocPtr(sizeof(byte *) * kcchFont);
	if (m_mpchpbCodeEven == NULL)
		return false;
	gmmgr.WritePtr(m_mpchpbCodeEven, 0, apbT, sizeof(apbT));

	m_mpchpbCodeOdd = (byte **)gmmgr.AllocPtr(sizeof(byte *) * kcchFont);
	if (m_mpchpbCodeOdd == NULL)
		return false;
	gmmgr.WritePtr(m_mpchpbCodeOdd, 0, apbT, sizeof(apbT));

    m_cxEllipsis = GetTextExtent("...");

	return true;
}

int Font::GetTextExtent(const char *psz)
{
	int cx = 0;
	while (*psz != 0) {
        byte ch = *psz++;
		cx += CharWidth(ch) - m_nGlyphOverlap;
    }

	// Shadow allows 1 pixel overlap but the last char doesn't overlap

	return cx + m_nGlyphOverlap;
}

int Font::GetTextExtent(const char *psz, int cch)
{
	int cx = 0;
	while (cch-- > 0) {
        byte ch = *psz++;
		cx += CharWidth(ch) - m_nGlyphOverlap;
    }

	// Shadow allows 1 pixel overlap but the last char doesn't overlap

	return cx + m_nGlyphOverlap;
}

int Font::CalcMultilineHeight(char *psz, int cxMultiline)
{
	int cy = 0;
	char *pszNext = psz;
	while (pszNext != NULL) {
		char *pszStart = pszNext;
		CalcBreak(cxMultiline, &pszNext);
		cy += BigWord(m_pfnth->cy) - m_nLineOverlap;
	}

	return cy;
}

void Font::DrawText(DibBitmap *pbm, char *psz, int x, int y, int cx, int cy,
        bool fEllipsis)
{
	int cyFont = BigWord(m_pfnth->cy) - m_nLineOverlap;
	int cyT = cyFont;
	char *pszNext = psz;
	while (pszNext != NULL) {
		if (cy != -1 && cyT > cy)
			return;
		char *pszStart = pszNext;
		int cch = CalcBreak(cx, &pszNext);
        if (fEllipsis && pszNext != NULL && cy != -1 && cyT + cyFont > cy) {
            DrawTextWithEllipsis(pbm, pszStart, cch, x, y, cx, true);
        } else if (fEllipsis && pszNext == NULL) {
            DrawTextWithEllipsis(pbm, pszStart, cch, x, y, cx, false);
        } else {
            DrawText(pbm, pszStart, x, y, cch);
        }
		y += cyFont;
		cyT += cyFont;
	}
}

void Font::DrawTextWithEllipsis(DibBitmap *pbm, char *psz, int cch, int x,
        int y, int cx, bool fForce)
{
    if (!fForce) {
        // If not being forced and text fits, draw it without ellipsis
        if (GetTextExtent(psz) < cx) {
            DrawText(pbm, psz, x, y, (int)strlen(psz));
            return;
        }
    }

    // Do a binary search to find out where the ellipsis is needed

    char szT[256];
    int imin = 0;

    // Convert to index and ensure room for ellipsis
    int imax = _min(cch - 1, (int)sizeof(szT) - 3 - 1);
    if (imax < 0) {
        imax = 0;
    }
    int icur = imax;
    int ifit = imax;
    while (true) {
        int icurT = imin + (imax - imin) / 2;
        if (icurT == icur) {
            break;
        }
        icur = icurT;
        int cxT = GetTextExtent(psz, icur + 1);
        if (cxT >= cx - m_cxEllipsis) {
            imax = icur - 1;
        } else {
            ifit = icur;
            imin = icur + 1;
        }
    }
    strncpyz(szT, psz, ifit + 2); // convert to count, add one for 0
    strcat(szT, "..."); // this fits without checks
    DrawText(pbm, szT, x, y, (int)strlen(szT));
}

#define IsBreakingChar(ch) ((ch) == ' ' || (ch) == '\t')

char *Font::FindNextNonBreakingChar(char *psz)
{
	while (*psz != 0) {
		if (!IsBreakingChar(*psz))
			return psz;
		psz++;
	}
	return NULL;
}

int Font::CalcBreak(int cx, char **ppsz, bool fChop)
{
	char *pchBreakAfter = NULL;
	int cchBreak;
	int cxT = 0;
	char *pchT = *ppsz;
	int cch = 0;
	bool fFoundBreak = false;

	while (*pchT != 0) {
		char ch = *pchT;

		// Handle any combo of \n, \r, \n\r, or \r\n

		switch (ch) {
		case '\n':
			pchT++;
			if (*pchT == '\r')
				pchT++;
			*ppsz = FindNextNonBreakingChar(pchT);
			return cch;

		case '\r':
			pchT++;
			if (*pchT == '\n')
				pchT++;
			*ppsz = FindNextNonBreakingChar(pchT);
			return cch;
		}

		// Otherwise if breaking char, remember break point

		if (IsBreakingChar(*pchT)) {
			fFoundBreak = true;
			pchBreakAfter = FindNextNonBreakingChar(pchT);
			cchBreak = cch;
		}

		// At right edge yet?

		int cxChar = CharWidth((byte)*pchT) - m_nGlyphOverlap;
		if (cxT + cxChar > cx) {
			// If last break exists, use it. Return pointer skips past break char

			if (fFoundBreak) {
				*ppsz = pchBreakAfter;
				return cchBreak;
			}

			// If no last break, sometimes callers want word chopping, sometimes not

			if (!fChop)
				return 0;

			// No last break; Scan forward for a breaking char so we know where to start
			// the next line

			while (*pchT != 0) {
				// Start the next past the break; only draw what fits in cx for this line

				if (IsBreakingChar(*pchT)) {
					*ppsz = FindNextNonBreakingChar(pchT);
					return cch;
				}

				// If carriage return, start at next char

				if (ch == '\n') {
					*ppsz = pchT + 1;
					return cch;
				}

				pchT++;
			}

			// At end of line with no break. Return length that fits in cx, and NULL to indicate done.

			if (*pchT == 0) {
				*ppsz = NULL;
				return cch;
			}
		}

		// Add char; next char

		cxT += cxChar;
		cch++;
		pchT++;
	}

	// Done without hitting edge. Return length and NULL to indicate done

	*ppsz = NULL;
	return cch;
}

int Font::DrawText(DibBitmap *pbm, char *psz, int x, int y, int cch, dword *mpscaiclr)
{
	if (m_pvf != NULL && InitVector()) {
		if (cch == -1)
			cch = (int)strlen(psz);
		return DrawVectorText(pbm, psz, x, y, cch);
	}
	return DrawBitmapText(pbm, psz, x, y, cch, mpscaiclr);
}

int Font::DrawBitmapText(DibBitmap *pbm, char *psz, int x, int y, int cch, dword *mpscaiclr)
{
#ifdef DEBUG
	for (int ichT = 0; ichT < cch; ichT++)
		Assert((word)psz[ichT] < (word)kcchFont);
#endif

	if (cch == -1)
		cch = (int)strlen(psz);

	// Clip entire line of text first.

	Size siz;
	pbm->GetSize(&siz);

	// Top clip

	if (y < 0)
		return 0;
	if (y + BigWord(m_pfnth->cy) > siz.cy)
		return 0;

	// Don't include x clipped chars
	// Left clip

	int ich = 0;
	int xT = x;
	char *pszT;
	for (pszT = psz; pszT - psz < cch; pszT++) {
		if (xT < 0) {
			ich++;
			xT += m_pfnth->acxChar[(byte)*pszT] - m_nGlyphOverlap;
			continue;
		}
		break;
	}
	int xDst = xT;

	// Right clip

	int cchT = 0;
	for (; pszT - psz < cch; pszT++) {
		int cx = m_pfnth->acxChar[(byte)*pszT] - m_nGlyphOverlap;
		if (xT + cx <= siz.cx) {
			cchT++;
			xT += cx;
			continue;
		}
		break;
	}
	if (cchT == 0)
		return 0;

	cch = cchT;

	// Draw

	xT = xDst;
	byte *pbDst = pbm->GetBits() + (long)y * siz.cx + xT;
	char *pszMax = &psz[ich + cch];

	for (pszT = &psz[ich]; pszT < pszMax; pszT++) {
		char ch = *pszT;
		int cxChar = m_pfnth->acxChar[(byte)ch];
		if (cxChar == 0)
			continue;

		byte *pbDraw;
		if (xT & 1) {
			pbDraw = m_mpchpbCodeOdd[(byte)ch];
		} else {
			pbDraw = m_mpchpbCodeEven[(byte)ch];
		}

		if (pbDraw == NULL) {
			ScanData *psd = (ScanData *)(((byte *)m_pfnth) +
                    BigWord(m_pfnth->mpchibsd[(byte)*pszT]));
			word cb = Compile8Thunk(gpbScratch, psd, xT & 1);
			byte *pbT = (byte *)gmmgr.AllocPtr(cb);
			if (pbT != NULL) {
				gmmgr.WritePtr(pbT, 0, gpbScratch, cb);
				if (xT & 1) {
					gmmgr.WritePtr(m_mpchpbCodeOdd, (byte)ch * sizeof(byte *),
                            &pbT, sizeof(byte *));
				} else {
					gmmgr.WritePtr(m_mpchpbCodeEven, (byte)ch * sizeof(byte *),
                            &pbT, sizeof(byte *));
				}
			}
			pbDraw = gpbScratch;
		}

		DrawDispatchThunk(pbDraw, NULL, pbDst, siz.cx - cxChar, mpscaiclr,
                gmpiclriclrShadow);
		pbDst += cxChar - m_nGlyphOverlap;
		xT += cxChar - m_nGlyphOverlap;
	}

	return xT - xDst;
}

//
// Vector fonts
//
// A vector font draws TrueType glyphs, anti-aliased, into the game's 8 bit
// frame buffer. It takes everything but the glyph shapes from the bitmap
// font it replaces: the line height and baseline (so every layout stays the
// same), the text color per row (fonts can have a vertical gradient), and the
// drop shadow. Coverage is blended into the palette with lookup tables built
// for the current palette.
//

#define kcLevels 16 // coverage levels; level kcLevels is fully opaque

struct VectorGlyph
{
	int xOff, yOff; // top left, relative to the pen position and baseline
	int cx, cy;
	std::vector<byte> ab; // coverage, 0..255
};

struct BlendTable
{
	dword rgb;
	int nAlpha; // 0..256
	byte aiclr[kcLevels + 1][256];
};

struct VectorFont
{
	VectorFont() : fInited(false), fFailed(false), pbData(NULL),
			fMapped(false), nUserScale(1.0f), nTracking(0), nScale(0),
			yBaseline(0), fCaps(false), fShadow(false), dxShadow(1),
			dyShadow(1), rgbShadow(0), nShadowAlpha(0), nPaletteSerial(-1) {
		memset(apglyph, 0, sizeof(apglyph));
		memset(acx, 0, sizeof(acx));
	}
	~VectorFont() {
		for (int i = 0; i < 256; i++)
			delete apglyph[i];
		for (size_t i = 0; i < apblt.size(); i++)
			delete apblt[i];
		if (fMapped)
			gpakr.UnmapFile(&fmap);
	}

	bool fInited;
	bool fFailed;
	FileMap fmap;
	byte *pbData;
	bool fMapped;
	stbtt_fontinfo info;
	float nUserScale;
	int nTracking;
	float nScale;
	int yBaseline;
	bool fCaps;
	int acx[256];
	VectorGlyph *apglyph[256];

	// Colors sampled from the bitmap font: palette index per row of the line
	std::vector<int> aiclrRow;
	bool fShadow;
	int dxShadow, dyShadow;
	dword rgbShadow;
	int nShadowAlpha;

	// Blend tables for the current palette
	int nPaletteSerial;
	std::vector<BlendTable *> apblt;
};

static void DeleteVectorFont(VectorFont *pvf)
{
	delete pvf;
}

bool Font::LoadVector(const char *pszTtf, float nScale, int nTracking)
{
	VectorFont *pvf = new VectorFont;
	dword cb = 0;
	pvf->pbData = (byte *)gpakr.MapFile(pszTtf, &pvf->fmap, &cb);
	if (pvf->pbData == NULL) {
		delete pvf;
		return false;
	}
	pvf->fMapped = true;
	if (!stbtt_InitFont(&pvf->info, pvf->pbData,
			stbtt_GetFontOffsetForIndex(pvf->pbData, 0))) {
		delete pvf;
		return false;
	}
	pvf->nUserScale = nScale > 0 ? nScale : 1.0f;
	pvf->nTracking = nTracking;
	delete m_pvf;
	m_pvf = pvf;
	return true;
}

// Map a character of the game's text (Windows-1252 as far as it goes) to a
// Unicode code point

static int CodepointFromChar(byte ch)
{
	static const word s_mp80[32] = {
		0x20ac, 0x81, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021,
		0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0x8d, 0x17d, 0x8f,
		0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0x2dc, 0x2122, 0x161, 0x203a, 0x153, 0x9d, 0x17e, 0x178 };
	if (ch >= 0x80 && ch < 0xa0)
		return s_mp80[ch - 0x80];
	return ch;
}

static int LuminanceOf(dword rgb)
{
	return (int)(((rgb >> 16) & 0xff) * 3 + ((rgb >> 8) & 0xff) * 6 +
			(rgb & 0xff)) / 10;
}

// Sample how the bitmap font looks: draw text twice on two different
// backgrounds. Pixels that changed the same way on both are glyph pixels;
// pixels that became the shadowed background are shadow.

struct FontSample
{
	int cx, cy;
	std::vector<int> aiclr; // glyph color, -1 none, -2 shadow
};

bool Font::InitVector()
{
	VectorFont *pvf = m_pvf;
	if (pvf->fInited)
		return true;
	if (pvf->fFailed || gmpiclriclrShadow == NULL || gpdisp == NULL)
		return false;

	// Two background colors that the shadow map darkens: saturated ones,
	// which text colors are unlikely to be

	const dword *argb = gpdisp->GetPaletteColors();
	int iclrA = -1, iclrB = -1;
	int nBestA = -1, nBestB = -1;
	for (int i = 1; i < 255; i++) {
		if (gmpiclriclrShadow[i] == i)
			continue;
		int r = (argb[i] >> 16) & 0xff, g = (argb[i] >> 8) & 0xff,
				b = argb[i] & 0xff;
		int nA = r - (g > b ? g : b);
		int nB = g - (r > b ? r : b);
		if (nA > nBestA) {
			nBestA = nA;
			iclrA = i;
		}
		if (nB > nBestB) {
			nBestB = nB;
			iclrB = i;
		}
	}
	if (iclrA < 0 || iclrB < 0 || iclrA == iclrB) {
		pvf->fFailed = true;
		return false;
	}

	int cyCell = BigWord(m_pfnth->cy);
	auto sample = [&](const char *psz, FontSample *ps) -> bool {
		int cxT = 4;
		for (const char *pch = psz; *pch != 0; pch++)
			cxT += m_pfnth->acxChar[(byte)*pch];
		ps->cx = cxT;
		ps->cy = cyCell;
		ps->aiclr.assign(cxT * cyCell, -1);
		DibBitmap *pbmA = CreateDibBitmap(NULL, cxT, cyCell);
		DibBitmap *pbmB = CreateDibBitmap(NULL, cxT, cyCell);
		if (pbmA == NULL || pbmB == NULL) {
			delete pbmA;
			delete pbmB;
			return false;
		}
		memset(pbmA->GetBits(), iclrA, cxT * cyCell);
		memset(pbmB->GetBits(), iclrB, cxT * cyCell);
		dword *mpscaiclr = TBitmap::s_ampscaiclrSide[0];
		DrawBitmapText(pbmA, (char *)psz, 1, 0, (int)strlen(psz), mpscaiclr);
		DrawBitmapText(pbmB, (char *)psz, 1, 0, (int)strlen(psz), mpscaiclr);
		byte *pbA = pbmA->GetBits();
		byte *pbB = pbmB->GetBits();
		for (int i = 0; i < cxT * cyCell; i++) {
			if (pbA[i] == iclrA && pbB[i] == iclrB)
				continue;
			if (pbA[i] == gmpiclriclrShadow[iclrA] &&
					pbB[i] == gmpiclriclrShadow[iclrB]) {
				ps->aiclr[i] = -2;
			} else if (pbA[i] == pbB[i]) {
				ps->aiclr[i] = pbA[i];
			}
		}
		delete pbmA;
		delete pbmB;
		return true;
	};

	// Colors per row, and the shadow, from a full alphabet

	FontSample fs;
	if (!sample("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz",
			&fs)) {
		pvf->fFailed = true;
		return false;
	}
	// Some fonts draw their shadow with fixed dark pixels rather than by
	// darkening the background. Treat pixels much darker than the text as
	// shadow.

	int nLumText = 0;
	for (size_t i = 0; i < fs.aiclr.size(); i++) {
		if (fs.aiclr[i] >= 0 && LuminanceOf(argb[fs.aiclr[i]]) > nLumText)
			nLumText = LuminanceOf(argb[fs.aiclr[i]]);
	}
	std::vector<int> acShadowColor(256, 0);
	for (size_t i = 0; i < fs.aiclr.size(); i++) {
		int iclr = fs.aiclr[i];
		if (iclr >= 0 && LuminanceOf(argb[iclr]) * 100 < nLumText * 40) {
			acShadowColor[iclr]++;
			fs.aiclr[i] = -2;
		}
	}

	// Cap height and baseline from 'H'

	FontSample fsH;
	if (!sample("H", &fsH)) {
		pvf->fFailed = true;
		return false;
	}
	for (size_t i = 0; i < fsH.aiclr.size(); i++) {
		int iclr = fsH.aiclr[i];
		if (iclr >= 0 && LuminanceOf(argb[iclr]) * 100 < nLumText * 40)
			fsH.aiclr[i] = -2;
	}
	int yTop = -1, yBottom = -1;
	for (int y = 0; y < fsH.cy; y++) {
		for (int x = 0; x < fsH.cx; x++) {
			if (fsH.aiclr[y * fsH.cx + x] >= 0) {
				if (yTop < 0)
					yTop = y;
				yBottom = y;
				break;
			}
		}
	}
	if (yTop < 0) {
		pvf->fFailed = true;
		return false;
	}
	int cyCap = yBottom - yTop + 1;
	pvf->yBaseline = yBottom + 1;

	pvf->aiclrRow.assign(cyCell, -1);
	long cGlyph = 0, cShadow = 0;
	for (int y = 0; y < fs.cy; y++) {
		std::vector<int> acCount(256, 0);
		for (int x = 0; x < fs.cx; x++) {
			int iclr = fs.aiclr[y * fs.cx + x];
			if (iclr >= 0) {
				acCount[iclr]++;
				cGlyph++;
			} else if (iclr == -2) {
				cShadow++;
			}
		}

		// The row's text color: the most used of its brighter half, so
		// edge and outline pixels don't win

		int iclrBest = -1;
		int cBest = 0;
		int nLumMax = 0;
		for (int i = 0; i < 256; i++) {
			if (acCount[i] != 0 && LuminanceOf(argb[i]) > nLumMax)
				nLumMax = LuminanceOf(argb[i]);
		}
		for (int i = 0; i < 256; i++) {
			if (acCount[i] > cBest && LuminanceOf(argb[i]) * 2 >= nLumMax) {
				cBest = acCount[i];
				iclrBest = i;
			}
		}
		pvf->aiclrRow[y] = iclrBest;
	}
	if (cGlyph == 0) {
		pvf->fFailed = true;
		return false;
	}

	// Drop rows much darker than the font's typical text color (stripes in
	// the pixel art): on thin vector strokes they look like gaps

	std::vector<int> anLum;
	for (int y = 0; y < cyCell; y++) {
		if (pvf->aiclrRow[y] >= 0)
			anLum.push_back(LuminanceOf(argb[pvf->aiclrRow[y]]));
	}
	if (!anLum.empty()) {
		std::vector<int> anSorted = anLum;
		std::sort(anSorted.begin(), anSorted.end());
		int nLumMedian = anSorted[anSorted.size() / 2];
		for (int y = 0; y < cyCell; y++) {
			int iclr = pvf->aiclrRow[y];
			if (iclr >= 0 && LuminanceOf(argb[iclr]) * 100 < nLumMedian * 75)
				pvf->aiclrRow[y] = -1;
		}
	}

	// Fill rows without text from their neighbors

	int iclrLast = -1;
	for (int y = 0; y < cyCell; y++) {
		if (pvf->aiclrRow[y] >= 0)
			iclrLast = pvf->aiclrRow[y];
		else
			pvf->aiclrRow[y] = iclrLast;
	}
	iclrLast = -1;
	for (int y = cyCell - 1; y >= 0; y--) {
		if (pvf->aiclrRow[y] >= 0)
			iclrLast = pvf->aiclrRow[y];
		else
			pvf->aiclrRow[y] = iclrLast;
	}

	// Drop shadow: offset and strength

	if (cShadow > cGlyph / 8) {
		// The offset that best maps text pixels onto shadow pixels

		pvf->fShadow = true;
		int cBestMatch = -1;
		for (int dy = 0; dy <= 2; dy++) {
			for (int dx = 0; dx <= 2; dx++) {
				if (dx == 0 && dy == 0)
					continue;
				int cMatch = 0;
				for (int y = 0; y < fs.cy; y++) {
					int yS = y - dy;
					if (yS < 0 || yS >= fs.cy)
						continue;
					for (int x = 0; x < fs.cx; x++) {
						int xS = x - dx;
						if (xS < 0 || xS >= fs.cx)
							continue;
						if (fs.aiclr[y * fs.cx + x] == -2 &&
								fs.aiclr[yS * fs.cx + xS] >= 0)
							cMatch++;
					}
				}
				if (cMatch > cBestMatch) {
					cBestMatch = cMatch;
					pvf->dxShadow = dx;
					pvf->dyShadow = dy;
				}
			}
		}

		// A fixed shadow color, or a darkening of what's underneath

		int iclrShadow = -1;
		int cMost = 0;
		for (int i = 0; i < 256; i++) {
			if (acShadowColor[i] > cMost) {
				cMost = acShadowColor[i];
				iclrShadow = i;
			}
		}
		if (iclrShadow >= 0 && cMost * 2 > cShadow) {
			pvf->rgbShadow = argb[iclrShadow] & 0xffffff;
			pvf->nShadowAlpha = 256;
		} else {
			int nLumA = LuminanceOf(argb[iclrA]);
			int nLumShadow = LuminanceOf(argb[gmpiclriclrShadow[iclrA]]);
			pvf->rgbShadow = 0;
			pvf->nShadowAlpha = nLumA > 0 ?
					256 - (nLumShadow * 256) / nLumA : 160;
			if (pvf->nShadowAlpha < 64)
				pvf->nShadowAlpha = 64;
		}
	}

	// Fonts drawn in capitals only: lowercase looks like uppercase

	FontSample fsLower, fsUpper;
	if (sample("abcdefg", &fsLower) && sample("ABCDEFG", &fsUpper))
		pvf->fCaps = fsLower.aiclr == fsUpper.aiclr;

	// Scale the TrueType font to the same cap height

	int x0, y0, x1, y1;
	if (!stbtt_GetCodepointBox(&pvf->info, 'H', &x0, &y0, &x1, &y1) ||
			y1 <= 0) {
		pvf->fFailed = true;
		return false;
	}
	pvf->nScale = (float)cyCap * pvf->nUserScale / (float)y1;

	// Keep descenders inside the line: if they would reach into the next
	// line, raise the baseline into the space above the capitals

	int nDescent = 0;
	for (const char *pch = "gjpqy"; *pch != 0; pch++) {
		int gx0, gy0, gx1, gy1;
		if (stbtt_GetCodepointBox(&pvf->info, *pch, &gx0, &gy0, &gx1, &gy1) &&
				-gy0 > nDescent)
			nDescent = -gy0;
	}
	int cyLine = cyCell - m_nLineOverlap;
	int cyDescent = (int)ceil(nDescent * pvf->nScale);
	int cyCapNew = (int)ceil(y1 * pvf->nScale);
	int cyOver = pvf->yBaseline + cyDescent - cyLine;
	int cyRoom = pvf->yBaseline - cyCapNew;
	if (!pvf->fCaps && cyOver > 0 && cyRoom > 0)
		pvf->yBaseline -= cyOver < cyRoom ? cyOver : cyRoom;

	for (int ch = 0; ch < 256; ch++) {
		int chT = ch;
		if (pvf->fCaps && ch >= 'a' && ch <= 'z')
			chT = ch - 'a' + 'A';
		int nAdvance, nLsb;
		stbtt_GetCodepointHMetrics(&pvf->info, CodepointFromChar((byte)chT),
				&nAdvance, &nLsb);
		pvf->acx[ch] = (int)lround(nAdvance * pvf->nScale) + pvf->nTracking;
		if (pvf->acx[ch] < 0)
			pvf->acx[ch] = 0;
	}
	pvf->acx[0] = 0;

	pvf->fInited = true;
	m_nGlyphOverlap = 0;
	m_cxEllipsis = GetTextExtent("...");
	return true;
}

int Font::VectorCharWidth(byte ch)
{
	if (!InitVector())
		return m_pfnth->acxChar[ch];
	return m_pvf->acx[ch];
}

static VectorGlyph *GetGlyph(VectorFont *pvf, byte ch)
{
	if (pvf->apglyph[ch] != NULL)
		return pvf->apglyph[ch];
	VectorGlyph *pglyph = new VectorGlyph;
	pglyph->xOff = pglyph->yOff = pglyph->cx = pglyph->cy = 0;
	byte chT = ch;
	if (pvf->fCaps && ch >= 'a' && ch <= 'z')
		chT = ch - 'a' + 'A';
	int cp = CodepointFromChar(chT);
	if (cp > ' ' && stbtt_FindGlyphIndex(&pvf->info, cp) != 0) {
		int cx, cy, xOff, yOff;
		byte *pb = stbtt_GetCodepointBitmap(&pvf->info, pvf->nScale,
				pvf->nScale, cp, &cx, &cy, &xOff, &yOff);
		if (pb != NULL) {
			pglyph->xOff = xOff;
			pglyph->yOff = yOff;
			pglyph->cx = cx;
			pglyph->cy = cy;

			// Slightly heavier than linear coverage reads better at
			// small sizes on dark backgrounds

			pglyph->ab.resize(cx * cy);
			for (int i = 0; i < cx * cy; i++) {
				pglyph->ab[i] = (byte)(pow(pb[i] / 255.0, 0.8) * 255.0 + 0.5);
			}
			stbtt_FreeBitmap(pb, NULL);
		}
	}
	pvf->apglyph[ch] = pglyph;
	return pglyph;
}

// Blend table: for each coverage level and destination color, the palette
// color nearest to the destination blended toward rgb by level * nAlpha.

static BlendTable *GetBlendTable(VectorFont *pvf, dword rgb, int nAlpha)
{
	int nSerial = gpdisp->GetPaletteSerial();
	if (pvf->nPaletteSerial != nSerial) {
		for (size_t i = 0; i < pvf->apblt.size(); i++)
			delete pvf->apblt[i];
		pvf->apblt.clear();
		pvf->nPaletteSerial = nSerial;
	}
	for (size_t i = 0; i < pvf->apblt.size(); i++) {
		if (pvf->apblt[i]->rgb == rgb && pvf->apblt[i]->nAlpha == nAlpha)
			return pvf->apblt[i];
	}

	BlendTable *pblt = new BlendTable;
	pblt->rgb = rgb;
	pblt->nAlpha = nAlpha;
	const dword *argb = gpdisp->GetPaletteColors();
	int rT = (rgb >> 16) & 0xff, gT = (rgb >> 8) & 0xff, bT = rgb & 0xff;
	for (int iclr = 0; iclr < 256; iclr++)
		pblt->aiclr[0][iclr] = (byte)iclr;
	for (int nLevel = 1; nLevel <= kcLevels; nLevel++) {
		int nMix = nLevel * nAlpha / kcLevels; // 0..256
		for (int iclr = 0; iclr < 256; iclr++) {
			int r = (argb[iclr] >> 16) & 0xff, g = (argb[iclr] >> 8) & 0xff,
					b = argb[iclr] & 0xff;
			r += ((rT - r) * nMix) >> 8;
			g += ((gT - g) * nMix) >> 8;
			b += ((bT - b) * nMix) >> 8;
			int iclrBest = iclr;
			long nBest = 0x7fffffff;
			for (int i = 0; i < 256; i++) {
				int dr = ((argb[i] >> 16) & 0xff) - r;
				int dg = ((argb[i] >> 8) & 0xff) - g;
				int db = (argb[i] & 0xff) - b;
				long n = 3L * dr * dr + 4L * dg * dg + 2L * db * db;
				if (n < nBest) {
					nBest = n;
					iclrBest = i;
				}
			}
			pblt->aiclr[nLevel][iclr] = (byte)iclrBest;
		}
	}
	pvf->apblt.push_back(pblt);
	return pblt;
}

static void BlendGlyph(DibBitmap *pbm, VectorFont *pvf, VectorGlyph *pglyph,
		int xPen, int yTop, bool fShadow)
{
	if (pglyph->cx == 0)
		return;
	Size siz;
	pbm->GetSize(&siz);
	byte *pbBits = pbm->GetBits();
	const dword *argb = gpdisp->GetPaletteColors();

	int xLeft = xPen + pglyph->xOff;
	int yGlyphTop = yTop + pvf->yBaseline + pglyph->yOff;
	if (fShadow) {
		xLeft += pvf->dxShadow;
		yGlyphTop += pvf->dyShadow;
	}

	int cRows = (int)pvf->aiclrRow.size();
	BlendTable *pblt = NULL;
	int iclrRowLast = -2;
	if (fShadow)
		pblt = GetBlendTable(pvf, pvf->rgbShadow, pvf->nShadowAlpha);

	for (int gy = 0; gy < pglyph->cy; gy++) {
		int y = yGlyphTop + gy;
		if (y < 0 || y >= siz.cy)
			continue;
		if (!fShadow) {
			// Color of this row of the line, as in the bitmap font
			int nRow = y - yTop;
			if (nRow < 0)
				nRow = 0;
			if (nRow >= cRows)
				nRow = cRows - 1;
			int iclrRow = pvf->aiclrRow[nRow];
			if (iclrRow != iclrRowLast) {
				pblt = GetBlendTable(pvf, argb[iclrRow] & 0xffffff, 256);
				iclrRowLast = iclrRow;
			}
		}
		const byte *pbSrc = &pglyph->ab[gy * pglyph->cx];
		byte *pbDstRow = pbBits + (long)y * siz.cx;
		for (int gx = 0; gx < pglyph->cx; gx++) {
			int nCoverage = pbSrc[gx];
			if (nCoverage == 0)
				continue;
			int x = xLeft + gx;
			if (x < 0 || x >= siz.cx)
				continue;
			int nLevel = (nCoverage * kcLevels + 127) / 255;
			if (nLevel == 0)
				continue;
			pbDstRow[x] = pblt->aiclr[nLevel][pbDstRow[x]];
		}
	}
}

int Font::DrawVectorText(DibBitmap *pbm, const char *psz, int x, int y,
		int cch)
{
	VectorFont *pvf = m_pvf;

	// Shadow first, under all the glyphs

	if (pvf->fShadow) {
		int xT = x;
		for (int ich = 0; ich < cch; ich++) {
			byte ch = (byte)psz[ich];
			BlendGlyph(pbm, pvf, GetGlyph(pvf, ch), xT, y, true);
			xT += pvf->acx[ch];
		}
	}
	int xT = x;
	for (int ich = 0; ich < cch; ich++) {
		byte ch = (byte)psz[ich];
		BlendGlyph(pbm, pvf, GetGlyph(pvf, ch), xT, y, false);
		xT += pvf->acx[ch];
	}
	return xT - x;
}

} // namespace wi
