#ifndef __HT_H__
#define __HT_H__

namespace wi {

#define HOSTILE_TAKEOVER

#ifdef DEBUG
#define INCL_TRACE
#endif

// Build types
// DEV_BUILD is defined by the build for Debug configurations

// DEV_BUILD settings

#if defined(DEV_BUILD)
#ifdef DEBUG
//#define INCL_VALIDATEHEAP
//#define INCL_TESTS
//#define INCL_MEMTRACE

#endif

// Enable stress testing
#define STRESS

// Enable this to allow little arrows to be drawn showing the paths being followed
#define DRAW_PATHS

// Enable this to draw a box around each tile a unit believes it is occupying
//#define DRAW_OCCUPIED_TILE_INDICATOR

// Enable this to draw a white pixel at the upper left corner of each tile
//#define MARK_TILE_BOUNDARIES

// Enable this to see which tiles are occupied according to the terrain map
//#define MARK_OCCUPIED_TILES

// Enable this to allow stats display to be toggled via the Find button
// <broken>
//#define STATS_DISPLAY

// Maintain and check a ring buffer of old allocs
#endif // defined(DEV_BULD)

// Things that are permanently on

#define DRAW_LINES // Enable this to allow lines to be drawn to visualize unit targeting
#define STATUS_LINE // Enable this to get certain status displayed at the bottom of the screen
//#define RALLY_POINTS


} // namespace wi
    
#include "inc/basictypes.h"
//#include "base/log.h"
#include "base/misc.h"
#include "mpshared/mpht.h"
#include "mpshared/misc.h"
#include "mpshared/enum.h"
#include "mpshared/ini.h"
#include "mpshared/decompress.h"
#include "game/missionlist.h"
#include "base/thread.h"

namespace wi {


typedef void *FileHandle;	// hf
typedef word CacheHandle; // hc

// Game version

#define knVersion 1

#define MAKEDWORD(x) ((((dword)x) << 24) | (((dword)x) << 16) | (((dword)x) << 8) | (x))

// Performance options

const word kfPerfRocketShots = 0x0001;
const word kfPerfRocketTrails = 0x0002;
const word kfPerfRocketImpacts = 0x0004;
const word kfPerfShots = 0x0008;
const word kfPerfShotImpacts = 0x0010;
const word kfPerfSelectionBrackets = 0x0020;
const word kfPerfSmoke = 0x0040;
const word kfPerfEnemyDamageIndicator = 0x0080;
const word kfPerfScorchMarks = 0x0100;
const word kfPerfSymbolFlashing = 0x0200;
const word kfPerfAll = 0x03ff;
const word kfPerfMax = 0xffff;

// Handicap flags and values

const word kfHcapIncreasedFirePower = 0x0001;
const word kfHcapDecreasedTimeToBuild = 0x0002;
const word kfHcapIncreasedMinerLoadValue = 0x0004;
const word kfHcapShowEnemyBuildProgress = 0x0010;	// build progress & upgrading
const word kfHcapShowEnemyResourceStatus = 0x0020;	// power & credit symbol flash
const word kfHcapIncreasedArmor = 0x0040;

const word kfHcapEasy = kfHcapDecreasedTimeToBuild | kfHcapIncreasedArmor | kfHcapIncreasedFirePower | kfHcapIncreasedMinerLoadValue | kfHcapShowEnemyBuildProgress | kfHcapShowEnemyResourceStatus;
const word kfHcapNormal = kfHcapDecreasedTimeToBuild | kfHcapIncreasedArmor | kfHcapShowEnemyBuildProgress | kfHcapShowEnemyResourceStatus;
const word kfHcapHard = kfHcapShowEnemyBuildProgress;
const word kfHcapDefault = kfHcapNormal;

const int knDecreasedTimeToBuildPercent = -20;
const int knIncreasedFirePowerPercent = 20;
const int knIncreasedMinerLoadValuePercent = 20;
const int knDecreasedDamagePercent = 90;

// Color type

typedef word Color; // clr

// Dib bitmap

#define kfDibFreeMem 0x0001
#define kfDibBackWindow 0x0002
#define kfDibWantScrolls 0x0004

class Rect;
class UpdateMap;
class DibBitmap // bm
{
public:
	DibBitmap();
	virtual ~DibBitmap();
	bool Init(byte *pb, int cx, int cy);
	virtual byte *GetBits();
	virtual void Clear(Color clr);
	virtual void Fill(int x, int y, int cx, int cy, Color clr);
	virtual void Shadow(int x, int y, int cx, int cy);
	virtual void GetSize(Size *psiz);
	virtual int GetRowBytes();
	virtual void DrawLine(short x1, short y1, short x2, short y2, Color clr);
	virtual void Blt(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst);
	virtual void BltTiles(DibBitmap *pbmSrc, UpdateMap *pupd, int yTopDst);
    virtual void Scroll(Rect *prcSrc, int xDst, int yDst);
	virtual DibBitmap *Suballoc(int yTop, int cy);
	word GetFlags() {
		return m_wf;
	}

	// True colour layer (the screen buffers only). Per pixel: a colour with
	// alpha (0xAARRGGBB) blended over the 8-bit pixel when the frame is
	// shown, and the 8-bit pixel it was laid over: if something is drawn
	// over that pixel afterwards, the layer stands aside there. Used for
	// soft fog edges and the mini map, which 256 colours can't show
	// smoothly. Copies and scrolls carry it; copying from a bitmap without
	// one clears it.

	bool InitTrueColor(dword *pdw = NULL, byte *pbUnder = NULL);
	dword *GetTrueColor() {
		return m_pdwTC;
	}
	byte *GetTrueColorUnder() {
		return m_pbTCUnder;
	}
	void ClearTrueColor(Rect *prc);
	void SetTrueColor(int x, int y, dword argb) {
		long i = (long)y * m_cbRow + x;
		m_pdwTC[i] = argb;
		m_pbTCUnder[i] = m_pb[i];
	}

protected:
	void BltBits(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst);
	void BltTrueColor(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst);

	word m_wf;
	Size m_siz;
	byte *m_pb;
	dword m_cb;
	int m_cbRow;
	dword *m_pdwTC;
	byte *m_pbTCUnder;
	bool m_fFreeTC;
};
DibBitmap *CreateDibBitmap(byte *pb, int cx, int cy);

// Timer support

class Timer;
class TimerMgr // timm
{
public:
	TimerMgr();
	~TimerMgr();

	void AddTimer(Timer *ptmr, long ct);
	void RemoveTimer(Timer *ptmr);
	void SetTimerRate(Timer *ptmr, long ct);
	long ScanDispatch(long tCurrent);
	virtual long GetTickCount();
	void Enable(bool fEnable) {
		m_fEnabled = fEnable;
	}
    bool IsAdded(Timer *ptmr);

private:
	Timer *m_ptmrFirst;
	Timer *m_ptmrNotifying;
	Timer *m_ptmrTriggerNext;
	bool m_fForceScan;
	bool m_fEnabled;
};
extern TimerMgr gtimm;

// Timer

class Timer // tmr
{
public:
    Timer() { m_ptimm = NULL; }
    virtual ~Timer();
	virtual void OnTimer(long tCurrent) = 0;

private:
    TimerMgr *m_ptimm;
	Timer *m_ptmrNext;
	Timer *m_ptmrPrev;
	long m_ctRate;
	long m_tTrigger;

	friend class TimerMgr;
};

// Point

struct Point // pt
{
	int x, y;
};

// Rectangle class

class WRect;
class TRect;

class Rect // rc
{
public:
	int left;
	int top;
	int right;
	int bottom;

	bool Intersect(Rect *prcSrc1, Rect *prcSrc2);
	bool Subtract(Rect *prcSrc1, Rect *prcSrc2);
	void Add(Rect *prcSrc1, Rect *prcSrc2);
	void FromWorldRect(WRect *pwrc);
	void FromTileRect(TRect *ptrc);
	void Union(Rect *prc);
	bool Equal(Rect *prc);
	void GetCenter(Point *ppt);
    int GetDistance(int x, int y);

	void Offset(int dx, int dy)
	{
		left += dx;
		right += dx;
		top += dy;
		bottom += dy;
	}

	void Inflate(int cx, int cy)
	{
		left -= cx;
		right += cx;
		top -= cy;
		bottom += cy;
	}

	bool PtIn(int x, int y)
	{
		return (x >= left && x < right && y >= top && y < bottom);
	}

	bool RectIn(Rect *prc)
	{
		return (prc->left >= left && prc->top >= top && prc->right <= right && prc->bottom <= bottom);
	}

	void Set(int l, int t, int r, int b)
	{
		left = l;
		top = t;
		right = r;
		bottom = b;
	}

	void SetEmpty()
	{
		left = 0;
		right = 0;
		bottom = 0;
		top = 0;
	}

	bool IsEmpty()
	{
		return ((left >= right) || (top >= bottom));
	}
	int Width()

	{
		return right - left;
	}

	int Height()
	{
		return bottom - top;
	}
};

// Palette

struct Palette // pal
{
    word cEntries;
    byte argb[1][3];
};
    
} // namespace wi
    
#include <htplatform.h>
#include "soundeffects.h"
#include "mpshared/packfile.h"
#include <rip.h>
#include "game/res.h"

namespace wi {

// PalmOS sdk 3.5 has min/max but sdk 4.0 doesn't


// Ensures the same types are being compared
template<class T> inline T _min(const T a, const T b) { return (a > b) ? b : a; }
template<class T> inline T _max(const T a, const T b) { return (a < b) ? b : a; }

#define sign(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)

// For forward references

class Level;
class Gob;
class SimUIForm;
typedef int GobType; // gt
typedef word UpgradeMask;
struct StructConsts;
class UnitGob;
class Player;
class Trigger;
class UnitGroup;

const StateMachineId ksmidNull = (word)-1;



class UpdateInterval // unvl
{
public:
	UpdateInterval() {
		m_cSkip = 0;
		m_cDecrementsLast = 0;
		m_cDecrements = 0;
	}

	void MinSkip() {
		m_cSkip = 0;
	}

	void MinSkip(int cSkip) {
		m_cSkip = _min(m_cSkip, cSkip);
	}

	bool Decrement() {
		if (m_cSkip <= 0) {
			m_cSkip = 0x7fff;
			m_cDecrementsLast = m_cDecrements;
			m_cDecrements = 0;
			return true;
		}

		// Count Decs separately since m_cSkip can be arbitrarily Min'ed

		m_cSkip--;
		m_cDecrements++;
		return false;
	}

	int GetUpdateCount() {
		// A skip of 0 (meaning next update) is an update count of 1

		return m_cDecrementsLast + 1;
	}

private:
	int m_cSkip;
	int m_cDecrementsLast;

#ifdef DEV_BUILD
public:
#endif
	int m_cDecrements;
};

// World coordinates are high-byte = tile coord, low-byte = sub-tile coord
// Coordinate types and macros

const int ktcMax = 256;
const int kpcMax = ktcMax * 32;	// + 1 to allow for conversion of inclusive/exclusive rects
const WCoord kwcMax = ktcMax * 256;
const WCoord kwcTile = 0x0100;
const WCoord kwcTileHalf = kwcTile / 2;
const WCoord kwcTile16th = kwcTile / 16;
const WCoord kwxInvalidPrev = -32768;	// Gob::m_wxPrev when there is none

#define kwcScrollLeftExtra 0

#define kwcScrollBorderSize WcFromTile16ths(5)
#define kwcScrollStepSize	kwcTileHalf

extern short *gmpPcFromWc;		// [64 * 256];	// 16,384 * 2 = 32,768 bytes
extern WCoord *gmpWcFromPc;		// [64 * 32];	// 2,048 * 2 = 4,096 bytes

inline TCoord TcFromWc(WCoord wc) {
	return wc >> 8;
}

inline int PcFracFromUpc(int pc) {
	return gmpPcFromWc[gmpWcFromPc[pc] & 0xff];
}

inline WCoord WcFromPc(int pc) {
	// gmpWcFromPc has one extra entry to allow an exclusive bottom/right coord
	// to be mapped, hence the "<=" below

	Assert(abs(pc) <= kpcMax);
	if (pc >= 0)
		return gmpWcFromPc[pc];
	else
		return -gmpWcFromPc[-pc];
}

inline int PcFromWc(WCoord wc) {
	Assert(abs(wc) < kwcMax);
	if (wc >= 0)
		return gmpPcFromWc[wc];
	else
		return -gmpPcFromWc[-wc];
}

inline TCoord TcFromPc(int pc) {
	return TcFromWc(WcFromPc(pc));
}

inline WCoord WcFromUpc(int pc) {
	Assert(pc >= 0 && pc < kpcMax);
	return gmpWcFromPc[pc];
}

inline int PcFromUwc(WCoord wc) {
	Assert(wc >= 0 && wc < kwcMax);
	return gmpPcFromWc[wc];
}

inline TCoord TcFromUpc(int pc) {
	return TcFromWc(WcFromUpc(pc));
}

inline WCoord WcFromTc(TCoord tc) {
	return tc << 8;
}

inline WCoord WcFromTile16ths(int n) {
	return kwcTile16th * n;
}

inline WCoord WcTrunc(WCoord wc) {
	return wc & ~0xff;
}

inline int PcFromTc(TCoord tc) {
	return PcFromWc(WcFromTc(tc));
}

extern int gcxFullScreenFormDiv10;

inline int PcFromFc(int fc) {
	return (fc * gcxFullScreenFormDiv10) / 16;
}

struct TPoint // pt
{
	TCoord tx, ty;
};

class WRect : public Rect
{
public:
	void Set(WPoint wpt1, WPoint wpt2);
	void FromTileRect(TRect *ptrc);
};

class TRect : public Rect
{
public:
	void FromWorldRect(WRect *wrc);
};

// Specialized TRect. Its specialty is being 1/2 the size (shorts since
// maps grew past 127 tiles)

struct TRectSmall
{
	short txLeft;
	short tyTop;
	short txRight;
	short tyBottom;

	void Union(TRectSmall *ptrc) {
		if (ptrc->txLeft >= ptrc->txRight || ptrc->tyTop >= ptrc->tyBottom)
			return;
		if (txLeft > ptrc->txLeft)
			txLeft = ptrc->txLeft;
		if (tyTop > ptrc->tyTop)
			tyTop = ptrc->tyTop;
		if (txRight < ptrc->txRight)
			txRight = ptrc->txRight;
		if (tyBottom < ptrc->tyBottom)
			tyBottom = ptrc->tyBottom;
	}

};


// NOTE: predefined iclr constants (e.g., kiclrBlack) are in res.h
// except for these non-index-type virtual colors

#define kiclrShadow -1
#define kiclrShadow2x -2

// Fixed point number types and macros

#define INTBITS 11
#define FRACBITS 5
#define INTBITS32 (INTBITS + 16)

typedef long fix32; // INTBITS32 integer bits, FRACBITS fractional bits
typedef short fix;	// INTBITS integer bits, FRACBITS fractional bits

#define itofx(x) ((x) << 5)						// Integer to fixed point
#define itofx32(x) (((long)(x)) << 5)						// Integer to fixed point
#define ftofx(x) ((fix)((x) * 32))				// Float to fixed point
#define ftofx32(x) ((fix32)((x) * 32))				// Float to fixed point
#define dtofx(x) ((fix)((x) * 32))				// Double to fixed point
#define fxtoi(x) ((x) >> 5)						// Fixed point to integer
#define fx32toi(x) ((x) >> 5)						// Fixed point to integer
#define fxtof(x) ((float) (x) / 32)				// Fixed point to float
#define fxtod(x) ((double)(x) / 32)				// Fixed point to double
#define mulfx(x, y) ((((fix32)(x)) * (y)) >> 5)	// Multiply a fixed by a fixed
#define mulfx32(x, y) ((((fix32)(x)) * (y)) >> 5)	// Multiply a fixed by a fixed
#define divfx(x, y) ((((fix32)(x)) << 5) / (y))	// Divide a fixed by a fixed
#define divfx32(x, y) ((((fix32)(x)) << 5) / (y))	// Divide a fixed by a fixed
#define addfx(x, y) ((fix)(x) + (fix)(y))
#define addfx32(x, y) ((fix32)(x) + (fix32)(y))
#define subfx(x, y) ((x) - (y))
#define fracfx(x) ((x) & ((1 << FRACBITS) - 1)) // Fraction of a fixed point
fix32 sqrtfx(fix32 x);
unsigned int isqrt(unsigned long long val);
int isqrt(int val1, int val2);

#ifdef OLD_LINE_ITERATOR
// For iterating along a line at arbitrary (possibly fractional) increments

class LineIterator {
public:
	void Init(int x1, int y1, int x2, int y2, fix nStep);

	bool Step() {
		if (m_cStepsRemaining == 0)
			return false;

		m_cStepsRemaining--;
		m_fx = addfx(m_fx, m_fdx);
		m_fy = addfx(m_fy, m_fdy);
		return true;
	}

	int GetX() {
		return fxtoi(m_fx);
	}

	int GetY() {
		return fxtoi(m_fy);
	}

	fix GetFixedX() {
		return m_fx;
	}

	fix GetFixedY() {
		return m_fy;
	}

private:
	fix m_fx, m_fy;
	fix m_fdx, m_fdy;
	int m_cStepsRemaining;
};
#endif	// OLD_LINE_ITERATOR

// For iterating along a line at arbitrary increments

class WLineIterator {
public:
	void Init(WCoord wx1, WCoord wy1, WCoord wx2, WCoord wy2, int nIncr);

	bool Step() {
		if (m_cStepsRemaining == 0)
			return false;

		m_cStepsRemaining--;
		m_wx += m_wdx;
		m_wy += m_wdy;
		return true;
	}

	int GetWX() {
		return m_wx;
	}

	int GetWY() {
		return m_wy;
	}

	int GetStepsRemaining() {
		return m_cStepsRemaining;
	}
private:
	// OPT: can save space by using caller's m_wx, m_wy
	WCoord m_wx, m_wy;
	WCoord m_wdx, m_wdy;
	int m_cStepsRemaining;
};

// Handy functions for dealing with time and update conversions

const int kcmsUpdate = 80;		// 80ms per Update
const int kctUpdate = 8;
const int kctMapScrollRate = 8;	// TUNE:

inline int UpdFromSec(int csec) {
	// This is to catch any overflows. If we find ourselves concerned with
	// intervals > 327 seconds we can add a cast to LONG in here.
	Assert(csec < (32768 / 100));

	// We assume kcmsUpdate can be divided by 10 without loss of precision
	Assert(((kcmsUpdate / 10) * 10) == kcmsUpdate);

	return (csec * 100) / (kcmsUpdate / 10);
}

inline int UpdFromMsec(int cms) {
	return cms / kcmsUpdate;
}

// Transparent bitmap

struct ScanData // sd
{
	word ibaiclr;
	word cbaiclrUnpacked;
};

struct TBitmapEntry // tbe
{
	word cx;
	word cy;
	word yBaseline;
	word ibsd;
	word cbsd;
};
#define kcbTBitmapEntry (sizeof(word) * 5)

struct TBitmapHeader // tbh
{
	word ctbm;
	TBitmapEntry atbe[1];
};

// Abstract base bitmap class so TBitmaps and RawBitmaps can be used
// interchangeably in certain basic situations

class HtBitmap
{
public:
	virtual ~HtBitmap();
	virtual bool Init(char *pszFn) = 0;
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Rect *prcSrc = NULL) = 0;
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Side side, Rect *prcSrc = NULL) = 0;
	virtual void GetSize(Size *psiz) = 0;
};

#define kfTbCloseFile 1
#define kfTbShared 2

#define kcCopyBy4Procs 32
#define kcColoredSides 7
#define kiSideCodeWhite (kcColoredSides - 1)	// side code table used to flash units white
class TBitmap : public HtBitmap
{
public:
	TBitmap();

	bool Init(File *pfil, word ib);
	void BltTo(int itbm, class DibBitmap *pbmDst, int xDst, int yDst, Side side = ksideNeutral, Rect *prcSrc = NULL);
	void GetSize(int itbm, Size *psiz);
	void FillTo(int itbm, class DibBitmap *pbmDst, int xDst, int yDst,
            int cxDst, int cyDst, int xOrigin = 0, int yOrigin = 0);

	int GetBaseline() {
		return (int)m_atbe[0].yBaseline;
	}

	void SetShared() {
		m_wf |= kfTbShared;
	}

	void ClearShared() {
		m_wf &= ~kfTbShared;
	}

	static bool InitClass();
	static void ExitClass();

	// HtBitmap overrides

	virtual ~TBitmap();
	virtual bool Init(char *pszFn);
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Rect *prcSrc = NULL);
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Side side, Rect *prcSrc = NULL);
	virtual void GetSize(Size *psiz);

private:
	byte *GetCompiledBits(int itbm, bool fOdd);
	static void MakeSideCodeMapping(Color *aclr, dword *mpscaiclrSide);
    void BltToScan(byte *pbDraw, int cx, int cy, DibBitmap *pbmDst,
            int xDst, int yDst, Side side, Rect *prcSrc);

	File *m_pfil;
	word m_ibtbh;
	word m_wf;
	int m_ctbm;
	TBitmapEntry *m_atbe;
	CacheHandle *m_ahc;

public:
	static dword *s_ampscaiclrSide[kcColoredSides];
};
TBitmap *LoadTBitmap(char *pszFn);
void FreeSharedTBitmaps();
TBitmap *GetSharedTBitmap(char *pszFn);
void FindSharedTBitmapFilename(TBitmap *ptbm, char *psz, int cb);

// For bltting raw bitmaps in a non-clipped, streamed manner

class RawBitmap : public HtBitmap // rbm
{
public:
	RawBitmap();

	// HtBitmap overrides

	virtual ~RawBitmap();
	virtual bool Init(char *pszFn);
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Rect *prcSrc = NULL);
	virtual void BltTo(class DibBitmap *pbmDst, int xDst, int yDst, Side side, Rect *prcSrc = NULL);
	virtual void GetSize(Size *psiz);

public:
	File *m_pfil;
	int m_cx;
	int m_cy;
};
RawBitmap *LoadRawBitmap(char *pszFn);

// Font / Text output

struct FontHeader // fnth
{
	word cy;
	byte acxChar[256];
	word mpchibsd[256];
	ScanData asd[1];
};

class Font // fnt
{
public:
	Font();
	~Font();

	bool Load(char *pszFont);

	// Draw with a TrueType font (a file in the pack) instead of the bitmap
	// glyphs. The bitmap font still provides the line height, baseline and
	// colors, so layouts are unchanged. nScale multiplies the glyph size,
	// nTracking adds pixels between letters.
	bool LoadVector(const char *pszTtf, float nScale = 1.0f,
			int nTracking = 0);

	// Vector font support for the text overlay (see TextOverlay)
	bool IsVector() {
		return m_pvf != NULL && InitVector();
	}
	void GetVectorBounds(const char *psz, int cch, int x, int y,
			Rect *prcInk);
	void RenderOverlay(const char *psz, int cch, int x, int y, float nScale,
			int xOut, int yOut, int cxOut, int cyOut, dword *pargb);

	int GetHeight() {
		return BigWord(m_pfnth->cy);
	}

	void SetGlyphOverlap(int nGlyphOverlap) {
		m_nGlyphOverlap = nGlyphOverlap;
	}

	void SetLineOverlap(int nLineOverlap) {
		m_nLineOverlap = nLineOverlap;
	}

	int GetLineOverlap() {
		return m_nLineOverlap;
	}

	int GetTextExtent(const char *psz);
	int GetTextExtent(const char *psz, int cch);
	int DrawText(DibBitmap *pbm, char *psz, int x, int y, int cch = -1,
            dword *mpscaiclr = NULL);
	void DrawText(DibBitmap *pbm, char *psz, int x, int y, int cx,
            int cy, bool fEllipsis = false);
    void DrawTextWithEllipsis(DibBitmap *pbm, char *psz, int cch,
            int x, int y, int cx, bool fForce = false);
	int CalcMultilineHeight(char *psz, int cxMultiline);
	int CalcBreak(int cx, char **psz, bool fChop = true);

private:
	char *FindNextNonBreakingChar(char *psz);
	int CharWidth(byte ch) {
		return m_pvf != NULL ? VectorCharWidth(ch) : m_pfnth->acxChar[ch];
	}
	int VectorCharWidth(byte ch);
	bool InitVector();
	int DrawVectorText(DibBitmap *pbm, const char *psz, int x, int y, int cch);
	int DrawBitmapText(DibBitmap *pbm, char *psz, int x, int y, int cch,
			dword *mpscaiclr);

	struct VectorFont *m_pvf;
	FileMap m_fmap;
	FontHeader *m_pfnth;
	byte **m_mpchpbCodeEven;
	byte **m_mpchpbCodeOdd;
	int m_nGlyphOverlap;
	int m_nLineOverlap;
    int m_cxEllipsis;
};

// Text overlay. Text in the menus and the game's forms is not drawn into the
// game frame, which may be shown scaled up; it is drawn on top of it at the
// window's own resolution, so it stays sharp at any size. Each piece of text
// remembers the frame pixels under it, and only shows where they are
// unchanged, so anything drawn over it later still covers it.
// See game/sdl/textoverlay.cpp.

class TextOverlay
{
public:
	// Should text drawn into pbm go to the overlay?
	static bool IsTarget(DibBitmap *pbm);

	// Record text drawn into pbm at x, y
	static void AddText(Font *pfnt, DibBitmap *pbm, const char *psz, int cch,
			int x, int y);

	// Frame bracketing, from MultiFormMgr::DrawFrame. The form managers'
	// back buffers, where they are on the screen, and what they repaint.
	static void BeginFrame(int cdib, DibBitmap **apbm, int *ayTop);
	static void EndFrame(int cdib, DibBitmap **apbm, UpdateMap **apupd);

	// Draw on the window, after the game frame (from Display)
	static void Render(float nScale, float xOut, float yOut);

	// Drop everything (display mode change, exit)
	static void Reset();
};
Font *LoadFont(char *pszFont);

struct TileSetHeader // tseth
{
	word cTiles;
	word cxTile;
	word cyTile;
};
#define kcbTileSetHeader 6

struct MiniTileSetHeader // mtseth
{
	word offNext;
	word cTiles;
	word cxTile;
	word cyTile;
};

#define kcchFnTset 28
struct TileMapHeader // tmaph
{
	char szFnTset[kcchFnTset];
	word ctx;
	word cty;
};

#define kfTmapMapped 1
#define kfMiniTsetMapped 2

#define kcTileSetsMax 64
class TileMap // tmap
{
public:
	TileMap();
	~TileMap();

	bool Load(char *psz, Size *psizPlayfield);
	void GetMapSize(Size *psiz);
	void GetTCoordMapSize(Size *psiz) {
		psiz->cx = BigWord(m_ptmaph->ctx);
		psiz->cy = BigWord(m_ptmaph->cty);
	}
	void GetTileSize(Size *psiz);
	void Draw(DibBitmap *pbm, int x, int y, int cx, int cy, int xMap, int yMap, byte *pbFogMap, UpdateMap *pupd);
	MiniTileSetHeader *GetMiniTileSetHeader(int nScale);

private:
	FileMap m_fmapTmap;
	TileMapHeader *m_ptmaph;
	FileMap m_afmapTset[kcTileSetsMax];
	TileSetHeader *m_aptseth[kcTileSetsMax];
	MiniTileSetHeader *m_pmtseth;
	FileMap m_fmapMiniTset;
	int m_cTileSets;
	int m_cTiles;
	int m_cxTile;
	int m_cyTile;
	word m_wf;

public:
	byte **m_apbTileData;
	word *m_pwMapData;
	int m_ctx;
	int m_cty;
	byte **m_apbDrawMap;
};
TileMap *LoadTileMap(char *pszFn, Size *psizPlayfield);

// Fog map

struct RevealPattern // rvlp
{
	byte ctx;
	byte cty;
	byte ab[1];
};

#define kbOpaque 0
#define kbEmpty 15

const word kbfFogMask = 0x000f;
const int kcFogShift = 0;
#define IsFogOpaque(bFog) (((bFog) & kbfFogMask) == kbOpaque)

const word kbfGalaxiteMask = 0x00f0;
const int kcGalaxiteShift = 4;
#define HasGalaxite(bFog) (((bFog) & kbfGalaxiteMask) != 0)

//#define HasWall(wFog)     (((wFog) & kwfWallMask) != 0)
//const word kwfWallMask = 0x0f00;
//const int kcWallShift = 8;

class Stream;
class AnimationData;
class FogMap // fogm
{
public:
	FogMap();
	~FogMap();

	bool Init(Size *psizTile, Size *psizMap);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	void RevealAll(UpdateMap *pupd);
	void Reveal(TCoord txMap, TCoord tyMap, RevealPattern *prvlp, UpdateMap *pupd, WCoord wxView, WCoord wyView);
	void Reveal(TRect *ptrc, UpdateMap *pupd, WCoord wxView, WCoord wyView);
	void Draw(DibBitmap *pbm, int xMap, int yMap, UpdateMap *pupd);
	void DrawGalaxite(DibBitmap *pbm, int xMap, int yMap, UpdateMap *pupd, byte *pbTrMap);

	byte *GetMapPtr() {
		return m_pbMap;
	}

	// Galaxite methods

	int GetGalaxite(TCoord tx, TCoord ty);
	void SetGalaxite(int nGx, TCoord tx, TCoord ty);
	bool DecGalaxite(TCoord tx, TCoord ty);
	bool FindNearestGalaxite(TCoord tx, TCoord ty, TPoint *ptpt, bool fIgnoreFog);

	// Galaxite regrowth (a per-level setting: knGalaxiteRegrowth*)

	void SetGalaxiteRegrowth(int nRegrowth) {
		m_nRegrowth = nRegrowth;
	}
	void CaptureGalaxiteOriginal(long tCurrent);
	void UpdateGalaxiteRegrowth(long tCurrent);


private:
	int m_ctxMap;
	int m_ctyMap;
	int m_cxTile;
	int m_cyTile;
	byte m_mpSrcDstResult[256];
	byte *m_pbMap;
	TBitmap *m_aptbm[16];
	int *m_apnCoverage[16];	// coverage of each edge tile, 0..256 per pixel
	bool m_fSoftInit;
	void *m_pvSoftCache;	// soft tiles by 3x3 neighbourhood (see fogmap.cpp)
	void InitSoftEdges();
	byte *GetSoftTile(byte *abNeighbours, int ffEdges);
	void DrawSoftEdge(DibBitmap *pbm, byte *pbSoft, int x, int y);
	void DrawEdgeFade(DibBitmap *pbm, int tx, int ty, int x, int y);
	int EdgeFadeAlpha(int d, int cpBand);
	byte GetFogAt(int tx, int ty);
	TBitmap *m_aptbmGalax[9];
	byte *m_pbGxOriginal;	// the Galaxite each tile started with
	int m_nRegrowth;
	long m_tRegrowthLast;
};

#define knGalaxiteRegrowthOff 0
#define knGalaxiteRegrowthSlow 1		// a size step every 200 seconds
#define knGalaxiteRegrowthNormal 2		// a size step every 100 seconds

// Directions

typedef char Direction; // dir

const Direction kdirInvalid = -1;
const Direction kdirN = 0;
const Direction kdirNE = 1;
const Direction kdirE = 2;
const Direction kdirSE = 3;
const Direction kdirS = 4;
const Direction kdirSW = 5;
const Direction kdirW = 6;
const Direction kdirNW = 7;

typedef int Direction16; // dir16
const Direction16 kdir16Invalid = -1;

// TerrainMap

struct PathSearch;	// terrainmap.cpp

#define kbfMobileUnit 0x80
#define kbfStructure 0x40
#define kbfReserved 0x20
#define kbfLinked 0x10
#define kbfHead 0x08
#define kbfDirMask 0x07

class Path;
class TrackPoint
{
public:
	bool Init(Path *ppath, TCoord txFrom, TCoord tyFrom, int itptStart, int ctptFurtherStop);
	void InitFrom(TrackPoint *ptrkp);
	bool IsProgress(TrackPoint *ptrkpA);
	bool IsBetterSort(TrackPoint *ptrkpA);
	void GetClosestPoint(TPoint *ptpt) {
		*ptpt = m_tptClosest;
	}
	int GetClosestPointIndex() {
		return m_itptClosest;
	}
	void GetAfterPoint(TPoint *ptpt) {
		*ptpt = m_tptAfter;
	}
	void GetInitialPoint(TPoint *ptpt) {
		*ptpt = m_tptInitial;
	}

protected:
	TPoint m_tptInitial;
	TPoint m_tptClosest;
	TPoint m_tptBefore;
	TPoint m_tptAfter;
	int m_itptClosest;
	int m_n2Before;
	int m_n2After;
};

class TerrainMap;
class Path
{
public:
	Path();
	~Path();

	bool Init(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, Direction *adir, int cdir);
	int GetCount();
	bool GetPoint(int itpt, TPoint *ptpt, byte bf);
	bool GetPointRaw(int itpt, TPoint *ptpt);
	void SetCacheIndex(int itpt);
	int FindClosestPoint(TCoord txFrom, TCoord tyFrom, int itptStart, int ctptFurtherStop, TPoint *ptptClosest);
	bool LoadState(TerrainMap *ptrmap, Stream *pstm);
	bool SaveState(Stream *pstm);
#ifdef DRAW_PATHS
	void Draw(DibBitmap *pbm, int xView, int yView, Side side);
#endif

protected:
	void CalcTo(int idir, TCoord *ptx, TCoord *pty);

	TerrainMap *m_ptrmap;
	int m_cdirs;
	Direction *m_adir;
	TCoord m_txStart, m_tyStart;
	int m_idirCache;
	TCoord m_txCache, m_tyCache;
};
Path *CreatePath(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, TPoint *ptpt, int ctpt);
Path *CreatePath(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, Direction *adir, int cdir);

// 0 = area
// 1 = open
// 2 = wall
// 3 = blocked

#define kttArea 0
#define kttOpen 1
#define kttWall 2
#define kttBlocked 3

struct TerrainMapHeader // trmaph
{
	word ctx;
	word cty;
};

class TerrainMap // trmap
{
public:
	TerrainMap();
	~TerrainMap();

	bool Init(char *pszFn);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	void SetFlags(int tx, int ty, int ctx, int cty, byte bf);
	bool TestFlags(int tx, int ty, int ctx, int cty, byte bf);
	void ClearFlags(int tx, int ty, int ctx, int cty, byte bf);
	bool IsOccupied(int tx, int ty, int ctx, int cty, byte bf);
	Path *FindPath(int txFrom, int tyFrom, int txTo, int tyTo, byte bfTerrainAvoid);
	Path *FindLinePath(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, byte bfTerrainAvoid);
	bool IsLineOccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, byte bfTerrainAvoid);
	bool FindFirstUnoccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, TCoord *ptxFree, TCoord *ptyFree);
	bool FindLastUnoccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, TCoord *ptxFree, TCoord *ptyFree);
	bool IsBlocked(TCoord tx, TCoord ty, byte bf);
	bool GetFlags(int tx, int ty, byte *pbf);
	int GetTerrainType(TCoord tx, TCoord ty);

	byte *GetMapPtr() {
		return m_pbTerrain;
	}

	// The tiles the level put walls on (see Init)

	int GetWallTileCount() {
		return m_cWalls;
	}
	TPoint *GetWallTiles() {
		return m_atptWall;
	}

	// True if the last FindPath returned no path only because the search
	// didn't fit in this update's budget: ask again next update

	bool IsSearchPending() {
		return m_fSearchPending;
	}

	// Terrain-connected area of a tile (0: blocked terrain). Tiles in
	// different areas can't reach each other.

	int GetArea(TCoord tx, TCoord ty);

	TCoord GetWidth() {
		return m_ctx;
	}
	TCoord GetHeight() {
		return m_cty;
	}


private:
	void InitAreas();
	void InitRooms();
	bool IsTerrainOpen(int off) {
		return ((byte *)(m_ptrmaph + 1))[off] <= kttOpen;
	}
	int AreaAround(int tx, int ty);
	bool FindNearestInArea(int txTo, int tyTo, int nArea, int *ptx, int *pty);
	int RoomNodeOf(int tx, int ty);
	bool MarkCorridor(int txFrom, int tyFrom, int txTo, int tyTo);
	void StartSearch(PathSearch *psrch, int txFrom, int tyFrom, int txTo, int tyTo, byte bfAvoid, bool fCorridor, bool fThroughBlocked);
	int RunSearch(PathSearch *psrch, int cNodesMax);
	Path *MakePath(PathSearch *psrch, int off);
	void RefreshBudget();

	FileMap m_fmap;
	TerrainMapHeader *m_ptrmaph;
	byte *m_pbTerrain;
	TPoint *m_atptWall;
	int m_cWalls;
	byte *m_abBuffer;
	int m_cbBuffer;
	int m_ctx;
	int m_cty;
	int m_mpDirDelta[8];
	static word s_anScoreVector[8];

	// Areas: terrain-connected regions

	word *m_awArea;

	// Rooms: the map in kctRoom x kctRoom blocks. A room node is one
	// connected part of a room; a rough route between room nodes limits
	// the detailed search to a corridor of rooms.

	int m_ctxRoom, m_ctyRoom;
	byte *m_abRoomPart;			// per tile: part of its room (1..), 0 blocked
	int *m_aiRoomNodeFirst;		// per room: index of its first node
	int m_cRoomNodes;
	struct RoomNode *m_arnod;
	int *m_aiRoomEdge;
	dword *m_adwRoomMark;		// per room: == m_dwRoomMark if in the corridor
	dword m_dwRoomMark;

	// Searches: one run to completion now, one big one that may continue
	// over several updates

	PathSearch *m_psrchNow;
	PathSearch *m_psrchBig;
	int m_cupdBudget;
	int m_cNodesBudget;
	bool m_fSearchPending;

	friend class Path;
};

// Animation & AnimationData

struct FrameData // frmd
{
	byte ibm;			// bitmap index
	byte ibm2;			// second bitmap index
	byte cHold;			// frame delay
	char xOrigin;		// x offset for drawing
	char yOrigin;		// y offset for drawing
	char xOrigin2;		// x offset for drawing second bitmap
	char yOrigin2;		// y offset for drawing second bitmap
	byte bCustomData1;	// first custom data value
	byte bCustomData2;	// second custom data value
};
#define kcbFrameData 9

class StripData // stpd
{
private:
	char szName[26];	// name of the Strip
	byte cDelay;		// inter-frame delay (in ??? units)
	byte bfFlags;		// e.g., fLoop
	word cfrmd;			// count of FrameData structures
	FrameData afrmd[1];	// array of FrameData structures

public:
	const char *GetName() {
		return szName;
	}

	int GetDefaultDelay() {
		return cDelay;
	}

	word GetFrameCount() {
		return BigWord(cfrmd);
	}

	FrameData *GetFrameData(int ifrmd) {
        return (FrameData *)(((byte *)afrmd) + ifrmd * kcbFrameData);
		//return &afrmd[ifrmd];
	}
};

// NOTE: All values are stored big-endian

struct AnimationFileHeader // anih
{
	word cstpd;			// count of StripData structures
	word aoffStpd[1];	// array of offsets to StripData structures
};

class AnimationData // anid
{
public:
	AnimationData();
	~AnimationData();

	bool Init(const char *pszAniName);
	int GetStripCount();
	int GetFrameCount(int nStrip);
	int GetStripIndex(const char *pszStripName);
	const char *GetStripName(int nStrip);
	void GetSpecialPoint(int nStrip, int nFrame, Point *pptSpecial);
	void GetBounds(int nStrip, int nFrame, Rect *prc);
	void DrawFrame(int nStrip, int nFrame, DibBitmap *pbm, int x, int y, Side side, Rect *prcSrc = NULL,
			class Animation *pani = NULL);
	int GetStripDelay(int nStrip);
	int GetFrameDelay(int nStrip, int nFrame);

private:
	struct HdBinding *m_phdb;	// HD pictures for this animation (hdsprites.cpp)
	TBitmap *m_ptbm;
	FileMap m_fmap;
	AnimationFileHeader *m_panih;
};
AnimationData *LoadAnimationData(const char *pszAniName);

// HD structures (game/sdl/hdsprites.cpp)

struct HdBinding;
HdBinding *HdBindAnimation(const char *pszAniName, AnimationData *panid);
bool HdIsActive();
bool HdWantsStrip(HdBinding *phdb, int nStrip, DibBitmap *pbm);
bool HdWantsRedraw();
void HdBeginFrame();
void HdPaintStart();
void HdHoldAnimation(class Animation *pani, bool fHold);
void HdNoteCovered(DibBitmap *pbm, int x, int y, int cx, int cy);
void HdRecord(HdBinding *phdb, AnimationData *panid, int nStrip, int nFrame, DibBitmap *pbm,
		int x, int y, Side side, class Animation *pani, const Rect *prcShadow);

const word kfAniLoop = 0x0001;
const word kfAniDone = 0x0002;
const word kfAniIgnoreFirstAdvance = 0x0004;
const word kfAniResetWhenDone = 0x0008;
const word kfAniFreeAnimationData = 0x8000;

class Animation // ani
{
public:
	Animation() {
		m_panid = NULL;
		m_nStrip = 0;
		m_nFrame = 0;
		m_cDelay = 0;
		m_wf = kfAniDone;
	}

	~Animation() {
		if (m_wf & kfAniFreeAnimationData)
			delete m_panid;
	}

	void Init(AnimationData *panid);
	bool Start(int nStrip = 0, int nFrame = 0, word wf = 0);
	bool Start(const char *pszStripName, word wf = 0);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);

	void SetStrip(int nStrip) {
		Assert(nStrip >= 0 && nStrip < m_panid->GetStripCount());
		m_nStrip = nStrip;
	}


	int GetStrip() {
		return m_nStrip;
	}

	void SetFrame(int nFrame) {
		Assert(nFrame >= 0 && nFrame < m_panid->GetFrameCount(m_nStrip));
		m_nFrame = nFrame;
	}

	int GetFrame() {
		return m_nFrame;
	}

	void GetSpecialPoint(Point *pptSpecial, int nFrame = -1) {
		if (nFrame < 0)
			nFrame = m_nFrame;
		m_panid->GetSpecialPoint(m_nStrip, nFrame, pptSpecial);
	}

	void GetBounds(Rect *prc) {
		m_panid->GetBounds(m_nStrip, m_nFrame, prc);
	}

	void Draw(DibBitmap *pbm, int x, int y, Side side = ksideNeutral, Rect *prcSrc = NULL) {
		m_panid->DrawFrame(m_nStrip, m_nFrame, pbm, x, y, side, prcSrc, this);
	}

	word GetFlags() {
		return m_wf;
	}

	int GetRemainingFrameTime() {
		return m_cCountdown;
	}

	AnimationData *GetAnimationData() {
		return m_panid;
	}

	bool Advance(int cAdvance);
	long GetRemainingStripTime();

private:
	AnimationData *m_panid;
	int m_nStrip;
	int m_nFrame;
	word m_wf;
	byte m_cCountdown;
	byte m_cDelay;
};

// Event Manager

struct Event { // evt
	int idf;
	int eType;
	union {
		int chr;
	};
	int x;
	int y;
	dword dw;
    dword ff;
    long ms;
    dword ffMods; // kfMod* keys held when the event happened
};
#define kfEvtCoalesce 0x00000002

// Custom events

#define gamePaintEvent 0x6000
#define penHoldEvent 0x6001
#define gameOverEvent 0x6002
#define penHoverEvent 0x6003
#define updateTriggersEvent 0x6004
#define resizeEvent 0x6007
#define hidePinEvent 0x6008
#define cancelModeEvent 0x6009
#define nullEvent 0x600a

// Mouse buttons other than the left one, and the wheel. These go only to the
// battlefield (SimUIForm), never to buttons or menus. x, y are as for pen
// events; wheelEvent's dw is the number of notches (+ is away from the user).
// middleMoveEvent is mouse motion while the middle button is held.

#define rightDownEvent 0x600b
#define middleDownEvent 0x600c
#define middleUpEvent 0x600d
#define middleMoveEvent 0x600e
#define wheelEvent 0x600f

// HostGetModifiers() flags

#define kfModShift 1
#define kfModCtrl 2
#define kfModAlt 4

// gameOverEvent constants (placed in Event::dw)

enum {
	knGoSuccess,
	knGoFailure,
	knGoInitFailure,
	knGoAbortLevel,
	knGoRetryLevel,
	knGoAppStop,
	knGoLoadSavedGame,
};
#define knAppExit 0xBEEFFEED // modifer to AppStopEvent, used only in windows for WM_CLOSE.

#define kcevtPostMax 20

#define kfRedrawDirty 1
#define kfRedrawMax 2
#define kfRedrawBeforeTimer 4
#define kfRedrawBeforeInput 8

struct FlickVector {
    int GetMagnitude() {
        return dx * dx + dy * dy;
    }
    int dx;
    int dy;
    dword cms;
};
#define kcevtPenHistory 32 // keep power of 2
#define kcmsFlickQuantum 150
    
class EventMgr // evm
{
public:
	EventMgr();
	~EventMgr();

	bool PeekEvent(Event *pevt, long cWait = 0);
	bool GetEvent(Event *pevt, long cWait = -1, bool fCheckPaints = true);
	void PostEvent(Event *pevt, bool fCoalesce = true);
	bool DispatchEvent(Event *pevt);
	void Init();
	void SetPenEventInterval(word ctInterval);
    bool GetFlickVector(int nPen, FlickVector *pfliv);

	void SetAppStopping() {
		m_fAppStopping = true;
	}

	bool IsAppStopping() {
		return m_fAppStopping;
	}

	void SetRedrawFlags(word wfRedraw)
	{
		m_wfRedraw |= wfRedraw;
	}

	void ClearRedrawFlags(word wfRedraw)
	{
		m_wfRedraw &= ~wfRedraw;
	}

	word GetRedrawFlags()
	{
		return m_wfRedraw;
	}

private:
    void UpdatePenHistory(Event *pevt);    
    bool QueryPenHistory(int nPen, long t, Point *ppt);

    int m_ievtPen1Next;
    Event m_aevtPen1History[kcevtPenHistory];
    int m_ievtPen2Next;
    Event m_aevtPen2History[kcevtPenHistory];    
    bool m_fPenHistoryInitialized;
	Event m_evtPeek;
	bool m_fPeekKeep;
	Event m_aevtQ[kcevtPostMax];
	int m_cevt;
	bool m_fAppStopping;
	word m_wfRedraw;
	bool m_fTimingPenHold;
	long m_tPenDown;
	int m_xHold, m_yHold;
	word m_ctMoveEventInterval;
	word m_nctMoveEventFraction;
	long m_tLastMoveEvent;
};
extern EventMgr gevm;

//---------------------------------------------------------------------------
// StateMachine
// Portions Copyright (C) Steve Rabin, 2000

class StateMachine;
class StateMachineMgr;

// NOTE: be sure to update the corresponding string table in StateMachine.cpp

enum State { // st
	kstReservedNull,
	kstReservedZombie,
	kstReservedGlobal,
	kstGuard,
	kstMove,
	kstAttack,
	kstChase,
	kstIdle,
	kstDying,
	kstBuildOtherCompleting,
	kstBeingBuilt,
	kstHuntEnemies,
	kstProcessorGetMiner,
	kstProcessorPutMiner,
	kstProcessorTakeGalaxite,
	kstMinerMoveToProcessor,
	kstMinerRotateForEntry,
	kstMinerMine,
	kstMinerFindGalaxite,
	kstMinerFaceGalaxite,
	kstMinerApproachGalaxite,
	kstMinerSuck,
	kstMinerStepAside,
	kstChangeStatePendingFireComplete,
	kstContinueActionPendingFireComplete,
};

STARTLABEL(StateNames)
	LABEL(kstReservedNull)
	LABEL(kstReservedZombie)
	LABEL(kstReservedGlobal)
	LABEL(kstGuard)
	LABEL(kstMove)
	LABEL(kstAttack)
	LABEL(kstChase)
	LABEL(kstIdle)
	LABEL(kstDying)
	LABEL(kstBuildOtherCompleting)
	LABEL(kstBeingBuilt)
	LABEL(kstHuntEnemies)
	LABEL(kstProcessorGetMiner)
	LABEL(kstProcessorPutMiner)
	LABEL(kstProcessorTakeGalaxite)
	LABEL(kstMinerMoveToProcessor)
	LABEL(kstMinerRotateForEntry)
	LABEL(kstMinerMine)
	LABEL(kstMinerFindGalaxite)
	LABEL(kstMinerFaceGalaxite)
	LABEL(kstMinerApproachGalaxite)
	LABEL(kstMinerSuck)
	LABEL(kstMinerStepAside)
	LABEL(kstChangeStatePendingFireComplete)
	LABEL(kstContinueActionPendingFireComplete)
ENDLABEL(StateNames)

STARTLABEL(GobTypes)
    LABEL(kgtNone)
    LABEL(kgtShortRangeInfantry)
    LABEL(kgtLongRangeInfantry)
    LABEL(kgtHumanResourceCenter)
    LABEL(kgtSurfaceDecal)
    LABEL(kgtScenery)
    LABEL(kgtAnimation)
    LABEL(kgtReactor)
    LABEL(kgtProcessor)
    LABEL(kgtStructure)
    LABEL(kgtUnit)
    LABEL(kgtGalaxMiner)
    LABEL(kgtHeadquarters)
    LABEL(kgtResearchCenter)
    LABEL(kgtVehicleTransportStation)
    LABEL(kgtRadar)
    LABEL(kgtLightTank)
    LABEL(kgtMediumTank)
    LABEL(kgtMachineGunVehicle)
    LABEL(kgtRocketVehicle)
    LABEL(kgtTakeoverSpecialist)
    LABEL(kgtWarehouse)
    LABEL(kgtMobileHeadquarters)
    LABEL(kgtOvermind)
    LABEL(kgtTankShot)
    LABEL(kgtRocket)
    LABEL(kgtMachineGunTower)
    LABEL(kgtRocketTower)
    LABEL(kgtScorch)
    LABEL(kgtSmoke)
    LABEL(kgtPuff)
    LABEL(kgtBullet)
    LABEL(kgtArtillery)
    LABEL(kgtArtilleryShot)
    LABEL(kgtAndy)
    LABEL(kgtReplicator)
    LABEL(kgtActivator)
    LABEL(kgtFox)
    LABEL(kgtAndyShot)
    LABEL(kgtWall)
ENDLABEL(GobTypes)

// Helper macros for sending messages
// (would be better as inline functions but I don't trust gcc to do the right thing)


#define SendMoveAction(_gidReceiver, _wx, _wy, _tcRadius, _wcMoveDistPerUpdate) { \
	Message msgT; \
	msgT.mid = kmidMoveAction; \
	msgT.smidSender = kgidNull; \
	msgT.smidReceiver = _gidReceiver; \
	msgT.MoveCommand.wptTarget.wx = _wx; \
	msgT.MoveCommand.wptTarget.wy = _wy; \
	msgT.MoveCommand.gidTarget = kgidNull; \
	msgT.MoveCommand.wptTargetCenter.wx = _wx; \
	msgT.MoveCommand.wptTargetCenter.wy = _wy; \
	msgT.MoveCommand.tcTargetRadius = _tcRadius; \
	msgT.MoveCommand.wcMoveDistPerUpdate = _wcMoveDistPerUpdate; \
	gsmm.SendMsg(&msgT); \
}

#define SendGuardAreaAction(_gidReceiver, _nArea) { \
	Message msgT; \
	msgT.mid = kmidGuardAreaAction; \
	msgT.smidSender = kgidNull; \
	msgT.smidReceiver = _gidReceiver; \
	msgT.GuardAreaCommand.nArea = _nArea; \
	gsmm.SendMsg(&msgT); \
}

#define SendGuardVicinityAction(_gidReceiver) { \
	Message msgT; \
	msgT.mid = kmidGuardVicinityAction; \
	msgT.smidSender = kgidNull; \
	msgT.smidReceiver = _gidReceiver; \
	gsmm.SendMsg(&msgT); \
}

#define SendMineCommand(_gidReceiver, _wx, _wy) { \
	Message msgT; \
	msgT.mid = kmidMineCommand; \
	msgT.smidSender = kgidNull; \
	msgT.smidReceiver = _gidReceiver; \
	msgT.MoveCommand.wptTarget.wx = _wx; \
	msgT.MoveCommand.wptTarget.wy = _wy; \
	msgT.MoveCommand.gidTarget = kgidNull; \
	msgT.MoveCommand.wptTargetCenter.wx = _wx; \
	msgT.MoveCommand.wptTargetCenter.wy = _wy; \
	msgT.MoveCommand.tcTargetRadius = 0; \
	msgT.MoveCommand.wcMoveDistPerUpdate = 0; \
	gsmm.SendMsg(&msgT); \
}

#define SendHuntEnemiesAction(_gidReceiver, _um) { \
	Message msgT; \
	msgT.mid = kmidHuntEnemiesAction; \
	msgT.smidSender = kgidNull; \
	msgT.smidReceiver = _gidReceiver; \
	msgT.HuntEnemiesCommand.um = _um; \
	gsmm.SendMsg(&msgT); \
}

// NOTE: The upgrade bits must not overlap the ksum bits used for structures
// because they are used as prerequisites. The InProgress version don't matter.

const word kfUpgradeHrc = 0x0001;
const word kfUpgradeVts = 0x0002;
const word kfUpgradeHrcInProgress = 0x0004;
const word kfUpgradeVtsInProgress = 0x0008;

struct DelayedMessage // dmsg
{
	Message msg;
	DelayedMessage *pdmsgNext;
};

const int knHandled = 1;
const int knNotHandled = 0;
const int knDeleted = -1;

// StateMachine block macros. One version with embedded tracing, one without.

#define BeginStateMachine   if (st == kstReservedGlobal) { if(0) {
#define State(stTest)       return knHandled; } } else if (st == stTest) { if(0) {

#define OnEnter             return knHandled; } else if (pmsg->mid == kmidReservedEnter) {
#define OnExit              return knHandled; } else if (pmsg->mid == kmidReservedExit) {
#define OnUpdate            return knHandled; } else if (pmsg->mid == kmidReservedUpdate) {
#define OnMsg(midTest)		return knHandled; } else if (pmsg->mid == midTest) {
#define SetState(stNext)    { m_stNext = stNext; m_fForceStateChange = true; m_unvl.MinSkip(); }
#define DiscardMsgs			return knHandled; } else {

#define EndStateMachineInherit(base)     return knHandled; } } else { \
							return (int)##base::ProcessStateMachineMessage(st, pmsg); } return (int)##base::ProcessStateMachineMessage(st, pmsg);
#define EndStateMachine		return knHandled; } } else { return knNotHandled; } return knNotHandled;

class StateMachine // sm
{
	friend class StateMachineMgr;

public:
	StateMachine();
	virtual int ProcessStateMachineMessage(State st, Message *pmsg);


	UpdateInterval *GetUpdateInterval() {
		return &m_unvl;
	}

protected:
    int ProcessMessage(State st, Message *pmsg) {
        return ProcessStateMachineMessage(st, pmsg);
    }

	UpdateInterval m_unvl;
	State m_st;
	State m_stNext;
	bool m_fForceStateChange;
};

class StateMachineMgr // smm
{
public:
	StateMachineMgr();
	~StateMachineMgr();
	bool Init(TimerMgr *ptimm);
	bool SaveState(Stream *pstm);
	bool LoadState(Stream *pstm);
	void ClearDelayedMessages();
	void SendMsg(MessageId mid, StateMachineId smidReceiver);
	void SendMsg(MessageId mid, StateMachineId smidSender, StateMachineId smidReceiver);
	void SendMsg(Message *pmsg);
	void PostMsg(MessageId mid, StateMachineId smidSender, StateMachineId smidReceiver);
	void SendDelayedMsg(Message *pmsg, long tDelay);
	void SendDelayedMsg(MessageId mid, long tDelay, StateMachineId smidSender, StateMachineId smidReceiver);
	void DispatchDelayedMessages();
	void RouteMessage(Message *pmsg);
	virtual StateMachineId GetId(StateMachine *psm) = 0;
	virtual StateMachine *GetStateMachine(StateMachineId smid) = 0;

private:
	void ProcessStateChange(StateMachine *psm);
	bool StoreDelayedMessage(Message *pmsg);

private:
	DelayedMessage *m_pdmsgHead;
	TimerMgr *m_ptimm;


};

const int kcmsgCommandQueueMax = 2000;	// was 300

class CommandQueue // cmdq
{
public:
	CommandQueue();
	~CommandQueue();

	bool Init(int cmsgMax = kcmsgCommandQueueMax);
	void Exit();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	void Enqueue(MessageId mid, StateMachineId smidReceiver);
	void Enqueue(Message *pmsg);

	void Clear() {
        m_cmsg = 0;
    }

    Message *GetFirst() {
        return m_amsg;
    }

	int GetCount() {
		return m_cmsg;
	}

private:
	void SetEntry(Message *pqmsg);

private:
	int m_cmsg;
	int m_cmsgMax;
	Message *m_amsg;
};


#define kcbPvarNameMax 21
#define kcbPvarValueMax kcbFilename

struct DictionaryEntry // de
{
	DictionaryEntry *pdeNext;
	char szName[kcbPvarNameMax];
	char szValue[kcbPvarValueMax];
};

class Dictionary // dict
{
public:
	Dictionary();
	~Dictionary();
	bool Init(Dictionary *pdict);
	void Clear();
	const char *Get(const char *pszName);
	bool Set(const char *pszName, const char *pszValue);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);

private:
	DictionaryEntry **Find(const char *pszName);

	int m_cde;
	DictionaryEntry *m_pdeHead;
};

// Game

struct ModeMatch
{
	int imode;
	int nDepthData;
	int nSizeData;
};

#define kfGameInitFonts 0x10
#define kfGameInitBitmap 0x20
#define kfGameInitDone 0x40

class MultiFormMgr;
class InputUIForm;
class Form;
class Game // game
{
public:
	Game();
	~Game();

	bool Init(int imm);
	void Exit();
	void Shell();
	int RunSimulation(Stream *pstm, char *pszLevel);
	bool FilterEvent(Event *pevt);
	void GetPlayfieldSize(Size *psiz);
	void RequestModeChange(int imm);
	SimUIForm *GetSimUIForm();
	InputUIForm *GetInputUIForm();
	void SaveGame(Stream *pstm = NULL);
	char *GetNextLevel();
	void SetNextLevel(char *pszLevel);
	int PlayLevel(MissionIdentifier *pmiid, Stream *pstmSavedGame = NULL,
            int nRank = 0);
	int PlaySavedGame(Stream *pstm);
	void SetGameSpeed(int t);
	void SavePreferences();
	bool SaveReinitializeGame();
	bool GetFormattedVersionString(char *pszVersion, char *pszOut);
	void ClearDisplay();
	bool GetVar(const char *pszName, char *pszBuff, int cbBuff);
	bool SetVar(const char *pszName, const char *pszValue);
	void CalcUnitCountDeltas(Level *plvl);
	bool IsVersionCompatibleWithExe(int nVersionCompareShip, int nVersionCompareMajor, char chVersionCompareMinor, bool fUpwardCompatOK);
	void ParseVersion(char *pszVersion, int *pnShipVersion, int *pnMajorVersion, char *pchMinorVersion);
	bool CheckDatabaseVersion(const char *pszDir, char *pszPdb,
            bool fUpwardCompatOK);

	// ModeMatch helpers

	int GetModeMatchBest() {
		return m_immBest;
	}

	int GetModeMatchCurrent() {
		return m_immCurrent;
	}

	int GetModeMatchCount() {
		return m_cmm;
	}

	void GetModeMatch(int imm, ModeMatch *pmm) {
		*pmm = m_amm[imm];
	}

    const MissionIdentifier& GetLastMissionIdentifier() {
        return m_miid;
    }

    void SkipSaveReinitializeGame() {
        m_fSkipSaveReinitialize = true;
    }

    void ScheduleUpdateTriggers();
    void UpdateTriggers();

private:
	bool CheckMemoryAvailable();
	void LoadPreferences();
	bool InitDisplay(int imm);
	int FindBestModeMatch2(int nDepthData, int nSizeData, int nDepthMode, int cxWidthModeMin, int cxWidthModeMax, byte bfMatch);
	int FindBestModeMatch(int nSizeDataAbove);
	void AddModeMatches(int nDepthData, int nSizeData, int nDepthOrGreater, int cxWidthOrGreater);
	bool LoadGameData();
	bool InitCoordMappingTables();
	bool InitSimulation(Stream *pstm, char *pszLevel);
	void ExitSimulation();
	ddword IsDataPresent(int cBpp);
	bool InitMemMgr();
	bool InitMultiFormMgr();

	int m_cmm;
	int m_cmmAlloc;
	ModeMatch *m_amm;
	int m_immCurrent;
	int m_immBest;
	SimUIForm *m_pfrmSimUI;
	InputUIForm *m_pfrmInputUI;
	Form *m_pfrmMiniMap;
	Size m_sizPlayfield;
	word m_wf;
	bool m_fSimUninitialized;
	char m_szNextLevel[kcbFilename];
    MissionIdentifier m_miid;
	Dictionary m_dictPvars;
    bool m_fSkipSaveReinitialize;
    bool m_fUpdateTriggers;
};
extern Game ggame;

void GameMain(char *psz);

enum PlayMode {
	kpmSavedGame,
	kpmNormal
};

class Shell
{
public:
	Shell();
	bool Init();
	void Exit();
	void Launch(bool fLoadReinitializeSave = false,
            MissionIdentifier *pmiid = NULL);
	int PlayGame(PlayMode pm, MissionIdentifier *pmiid, Stream *pstm,
            int nRank);
	void SetPalette();

private:
    bool DoPlay();
	bool PlaySinglePlayer();

	FileMap m_fmapPalette;
	FileMap m_fmapShadowMap;
	Palette *m_ppal;
	byte *m_mpiclriclrShadow;
};
extern Shell gshl;

#define knStageNone 0
#define knStageCollect 1
#define knStageCheckDraw 2

// UpdateMap

struct MapInfo
{
	int cxLeftTile;
	int cyTopTile;
	int cxRightTile;
	int cyBottomTile;
	int ctxInside;
	int ctyInside;
};

class UpdateMap
{
public:
	UpdateMap();
	~UpdateMap();

	bool Init(Size *psiz);
	void Reset();
	void InvalidateRect(Rect *prc = NULL);
	void InvalidateMapTileRect(TRectSmall *ptrc);
	bool EnumUpdateRects(bool fFirst, Rect *prcBounds, Rect *prc);
	bool Scroll(int dx, int dy);
	bool *GetInvalidMap();
	void GetMapSize(Size *psiz);
	MapInfo *GetMapInfo();
	bool IsInvalid();
	void SetViewOrigin(int xOrigin, int yOrigin);
	void Validate();
	void InvalidateTile(TCoord tx, TCoord ty);
	void StartMergeDamagedInvalid();
	void EndMergeDamagedInvalid();

	bool IsRectInvalid(Rect *prc)
	{
		TRectSmall trc;
		CalcTileRect(prc, &trc);
		return IsTileRectInvalid(&trc);
	}

	bool IsRectInvalidAndTrackDamage(Rect *prc)
	{
		TRectSmall trc;
		CalcTileRect(prc, &trc);
		bool fNewInvalid;
		return IsTileRectInvalidAndTrackDamage(&trc, &fNewInvalid);
	}
	bool IsMapTileRectInvalidAndTrackDamage(TRectSmall *ptrc, bool *pfNewInvalid);

private:
	bool EnumRowRects(bool fFirst, Rect *prcBounds, Rect *prc);
	void InvalidateTileRect(TRectSmall *ptrc);
	bool IsTileRectInvalidAndTrackDamage(TRectSmall *ptrc, bool *pfNewInvalid);
	bool IsTileRectInvalid(TRectSmall *ptrc);
	void CalcTileRect(Rect *prc, TRectSmall *ptrc);
	void SetMapOffset(int xMapOffset, int yMapOffset, bool fInvalidate = true);

	Rect m_rcDib;
	int m_ctx;
	int m_cty;
	int m_txOrigin;
	int m_tyOrigin;
	int m_xOriginView;
	int m_yOriginView;
	bool *m_afInvalid;
	bool *m_afInvalidDamage;
	int m_xMapOffset;
	int m_yMapOffset;
	bool m_fInvalid;
	bool m_fInvalidDamage;
	bool m_fMergeDamage;
	MapInfo m_mnfo;
};
    
class FlickScroller // flics
{
public:
    FlickScroller();
    bool Init(int nPen, float flMultiplier = 1.0f,
              float flDecayPercent = 0.01f,
              float cmsDecaySpan = kcmsFlickQuantum / 10, bool fChoose = true);
    void Clear();
    bool GetPosition(Point *ppt);
    bool HasMagnitude();
    
private:
    bool CheckMagnitude(float dx, float dy);
    
    FlickVector m_fliv;
    long m_msStart;
    float m_flMultiplier;
    float m_flDecayPercent;
    dword m_cmsDecaySpan;
    bool m_fHasMagnitude;
};  

// Form Manager

#define kcFormsMax 16

#define kfFrmmNoScroll 1

class Form;
class ButtonControl;
class FormMgr // frmm
{
public:
	FormMgr();
	virtual ~FormMgr();

	virtual bool Init(DibBitmap *pbm, bool fFreeDib);
	virtual void AddForm(Form *pfrm);
	virtual void RemoveForm(Form *pfrm);
	virtual bool EcomSuppressed();
	virtual bool CookEvent(Event *pevt);
	virtual Form *GetFormPtr(word idf);
	virtual Form *LoadForm(IniReader *pini, word idf, Form *pfrm);
	virtual void Paint(bool fScrolled, Rect *prcOpaqueStart);
	virtual void Scroll(int dx, int dy);
	virtual bool HasCapture();
    virtual Form *GetFormCapture();
	virtual Form *GetModalForm();
    Form *HitTestForms(Event *pevt);
	virtual void InvalidateRect(Rect *prc);
	virtual DibBitmap *GetDib();
	virtual void CalcOpaqueRect(Form *pfrmStop, Rect *prcOpaqueStart, Rect *prcResult);
	virtual Form *GetFocus();
    virtual void FrameStart();
    virtual void FrameComplete();
    virtual void BreakCapture();
    
	void SetFlags(word wf) {
		m_wf = wf;
	}
	word GetFlags() {
		return m_wf;
	}
	int GetFormCount() {
		return m_cfrm;
	}
	Form *GetFormAt(int n) {
		return m_apfrm[n];
	}
protected:
    Form *FindPen2Form();
	bool CookPenEvent(Event *pevt);
	bool CookKeyEvent(Event *pevt);
	virtual bool BltTo(DibBitmap *pbmDst, int yTop, bool fScrolled);
	virtual bool ScrollBits();

	word m_wf;
	word m_idfCapture;
    int m_cCaptureDowns;
	int m_cfrm;
	Form *m_apfrm[kcFormsMax];
	DibBitmap *m_pbm;
	UpdateMap *m_pupd;
	bool m_fFreeDib;
	int m_dxScrollAccumulate;
	int m_dyScrollAccumulate;

public:
	UpdateMap *GetUpdateMap() {
		return m_pupd;
	}

	friend class MultiFormMgr;
};

#define kcFormMgrMax 5
class MultiFormMgr : public FormMgr
{
public:
	MultiFormMgr();
	virtual ~MultiFormMgr();

	void AddFormMgr(FormMgr *pfrmm, Rect *prc);
	void RemoveFormMgr(FormMgr *pfrmm);
	void DrawFrame(bool fForceBackBufferValid, bool fPaint = true);
	void CheckSetRedrawDirty();
	bool GetFormMgrRect(FormMgr *pfrmm, Rect *prc);

#ifdef STATS_DISPLAY
	int GetUpdateRectCount();
#endif

	virtual void AddForm(Form *pfrm);
	virtual void RemoveForm(Form *pfrm);
	virtual bool EcomSuppressed();
	virtual bool CookEvent(Event *pevt);
	virtual Form *GetFormPtr(word idf);
	virtual bool Paint(bool fForceBackBufferValid);
	virtual void InvalidateRect(Rect *prc);
	virtual void CalcOpaqueRect(Form *pfrmStop, Rect *prcOpaqueStart, Rect *prcResult);
	virtual Form *GetFocus();

private:
    bool CookBattlefieldEvent(Event *pevt);
	virtual bool BltTo(DibBitmap *pbmDst, int yTop, bool fScrolled);
    bool StealCapturePen2(Event *pevt, FormMgr *pfrmmChildCapture);

	int m_cfrmm;
	FormMgr *m_apfrmm[kcFormMgrMax];
	Rect m_arcFormMgr[kcFormMgrMax];
    Event m_evtPen1Down;
};

// Form

#define kcControlsMax 32
#define kfFrmDeleted 0x01
#define kfFrmDoModal 0x02
#define kfFrmVisible 0x04
#define kfFrmPenInside 0x08
#define kfFrmHasPalette 0x10
#define kfFrmHasShadowMap 0x20
#define kfFrmScaleCoords 0x40
#define kfFrmAutoTakedown 0x80
#define kfFrmTranslucent 0x100
#define kfFrmNoFocus 0x200
#define kfFrmShowSound 0x400
#define kfFrmNoEcom 0x800
#define kfFrmTopMost 0x1000
#define kfFrmDemandPen2 0x2000

#define knNotifySelectionChange 0
#define knNotifySelectionTap 1

class IControlEventHandler {
public:
	virtual void OnControlSelected(word nSelect) = 0;
	virtual bool OnControlHeld(word idc) = 0;
	virtual void OnControlNotify(word idc, int nNotify) = 0;
};

class Control;
class Form : public IControlEventHandler
{
public:
	Form();
	virtual ~Form();

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
    virtual bool OnFilterEvent(Event *pevt) { return false; }
    virtual bool OnRightDown(Event *pevt) { return false; }	// right button, while modal
	virtual bool EventProc(Event *pevt);
	virtual bool DoModal(int *pnResult = NULL, Sfx sfxShow = ksfxGuiFormShow, Sfx sfxHide = ksfxGuiFormHide);
	virtual bool OnHitTest(Event *pevt);
	virtual bool OnKeyTest(Event *pevt);
	virtual bool OnPenEvent(Event *pevt);
	virtual void OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnPaintControls(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnScroll(int dx, int dy);
	virtual void ScrollInvalidate(UpdateMap *pupd);
	virtual void InvalidateRect(Rect *prc);
    virtual void FrameStart();
    virtual void FrameComplete();
    virtual Control *HitTestControls(Event *pevt);
    virtual void BreakCapture();

	// IControlEventHandler methods

	virtual void OnControlSelected(word idc);
	virtual bool OnControlHeld(word idc);
	virtual void OnControlNotify(word idc, int nNotify);

	FormMgr *GetFormMgr();

	word GetId();
	word GetFlags();
	void SetFlags(word wf);
	void GetRect(Rect *prc);
	void SetRect(Rect *prc);
	void Show(bool fShow);
	void EndForm(int nResult = 0);
	Control *GetControlPtr(word idc);
	bool IsControlInside(Control *pctl);
	static void UpdateHover(Form *pfrm, Event *pevt);
	static void SetHoverControl(Control *pctl);
	static ButtonControl *GetActiveButton(Control *pctl);
	void ClearHover(bool fInvalidate = true);
	void SetUserDataPtr(void *pUserData);
	void *GetUserDataPtr();
    Form *FindPen2Form();

    Control *GetControlCapture() {
        return m_pctlCapture;
    }

protected:
	bool InitFromProperties(FormMgr *pfrmm, word idf, IniReader *pini, char *pszForm);

	FormMgr *m_pfrmm;
	TBitmap *m_ptbm;
	Control *m_pctlCapture;
	static Control *s_pctlHover;
	Rect m_rc;
	int m_nResult;
	word m_wf;
	word m_idf;
	word m_idcDefault;
	word m_idcLast;
	int m_cctl;
	Sfx m_sfxShow;
	Sfx m_sfxHide;
	Control *m_apctl[kcControlsMax];
	FileMap m_fmapPalette;
	FileMap m_fmapShadowMap;
	int m_iclrBack;
	void* m_pUserData;

	friend class FormMgr;
};

inline FormMgr *Form::GetFormMgr()
{
	return m_pfrmm;
}

// DialogForm

#define kcyTitle 10

class DialogForm : public Form
{
public:
	DialogForm();
	~DialogForm();

	void SetBackgroundColorIndex(int iclr);
	void SetTitleColor(Color clr);
	void SetBorderColorIndex(int iclr);
	void SetClearDibFlag();

	// Form overrides

	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnPaint(DibBitmap *pbm);
	virtual bool OnPenEvent(Event *pevt);
	virtual bool DoModal(int *pnResult = NULL, Sfx sfxShow = ksfxGuiFormShow, Sfx sfxHide = ksfxGuiFormHide);

private:
	Color m_clrTitle;
	int m_iclrBorder;
	int m_iclrBackground;
	bool m_fClearDib;
};

inline void DialogForm::SetTitleColor(Color clr) {
	m_clrTitle = clr;
}

inline void DialogForm::SetBorderColorIndex(int iclr) {
	m_iclrBorder = iclr;
}

inline void DialogForm::SetClearDibFlag() {
	m_fClearDib = true;
}

// ShellForm

class ShellForm : public Form, public Timer
{
public:
	ShellForm();
	virtual ~ShellForm();

	// Doesn't match Form's DoModal signature so it's not really an override

	virtual bool DoModal(int *pnResult = NULL, bool fAnimate = true, bool fShowSound = true);

	// Form overrides

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);

	// Timer interface

	virtual void OnTimer(long tCurrent);
    virtual void OnZipDone() {}

protected:
	bool m_fAnimate;

private:
	bool m_fCached;
	int m_axDst[kcControlsMax];
	bool m_fTimerEnabled;
	int m_cctlToZip;
	long m_tLast;
};

// InGameOptions

class InGameOptionsForm : public ShellForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	void InitResettableControls();
	void UpdateLabels();

	long m_tGameSpeed;
	word m_wfHandicap;
    double m_nScrollSpeed;
};

// Control

#define knSelDownInside 0
#define knSelUpInside 1
#define knSelUpOutside 2
#define knSelMoveInside 3
#define knSelMoveOutside 4
#define knSelHoldInside 5

#define kfCtlVisible 1
#define kfCtlSet 2
#define kfCtlRedraw 4
#define kfCtlDisabled 8
#define kfCtlUseSide1Colors 16

class Control
{
public:
	Control();
	virtual ~Control();

	virtual bool Init(Form *pfrm, word idc, int x, int y, int cx, int cy);
	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnPenEvent(Event *pevt);
	virtual void OnSelect(int nSelect);
    virtual int OnHitTest(Event *pevt);
	virtual void Invalidate();
    virtual void OnBreakCapture();

	word GetId();
	void GetRect(Rect *prc);
	void SetRect(Rect *prc, bool fCompareRect = true);
	void SetPosition(int x, int y);
	void Show(bool fShow);
	word GetFlags();
	void SetFlags(word wf);
	void Enable(bool fEnable);

protected:
	Rect m_rc;
	word m_wf;
	word m_idc;
	Form *m_pfrm;
	IControlEventHandler *m_pceh;

	friend class Form;
};

// Button Control

class ButtonControl : public Control // btn
{
public:
	static bool InitClass();
	static void ExitClass();

	ButtonControl();
	virtual ~ButtonControl();

	virtual bool Init(Form *pfrm, word idc, int x, int y, int cx, int cy,
			char *pszLabel, int nfnt, char *szFnUp = NULL, char *szFnDown = NULL, char *szFnDisabled = NULL);
	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnSelect(int nSelect);

	void SetText(char *psz);

private:
	virtual bool Init(char *pszLabel, int nfnt, char *szFnUp, char *szFnDown, char *szFnDisabled, bool fCenter);

protected:
	TBitmap *m_ptbmUp;
	TBitmap *m_ptbmDown;
	TBitmap *m_ptbmDisabled;
	int m_nfnt;
	char *m_szLabel;

private:
	static TBitmap *s_ptbmLeftUp;
	static TBitmap *s_ptbmMidUp;
	static TBitmap *s_ptbmRightUp;
	static TBitmap *s_ptbmLeftDown;
	static TBitmap *s_ptbmMidDown;
	static TBitmap *s_ptbmRightDown;
};

class RadioButtonBarControl : public Control
{
public:
	RadioButtonBarControl();
	virtual ~RadioButtonBarControl();

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
    virtual bool Init(const char *pszLabel, int ifnt, int isel);
	virtual void OnPaint(DibBitmap *pbm);
    virtual void OnPenEvent(Event *pevt);

    int GetSelectionIndex();
    void SetSelectionIndex(int isel);

private:
    void AddLabel(const char *psz, int cch);
    void GetCellRects(int icell, Rect *prcInner, Rect *prcOuter);
    void GetOuterCellSize(int icell, Size *psiz);

    int m_ifnt;
    int m_isel;
    int m_cLabels;
    const char *m_apszLabels[20];
};

// Checkbox Control

class CheckBoxControl : public Control // cbox
{
public:
	CheckBoxControl();
	virtual ~CheckBoxControl();

	virtual bool Init(Form *pfrm, word idc, int x, int y, char *pszLabel, int nfnt, bool fChecked);
	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnSelect(int nSelect);

	void SetText(char *psz);
	bool IsChecked();
	void SetChecked(bool fChecked);
	static bool InitClass();
	static void ExitClass();

private:
	bool Init(char *pszLabel, int nfnt, bool fChecked);

	static TBitmap *s_ptbmOnUp;
	static TBitmap *s_ptbmOnDown;
	static TBitmap *s_ptbmOffUp;
	static TBitmap *s_ptbmOffDown;
	int m_ifnt;
	char m_szLabel[64];
	bool m_fChecked;
};

// Label Control

#define kfLblCenterText 0x8000
#define kfLblMultiLine 0x4000
#define kfLblRightText 0x2000
#define kfLblClipVertical 0x1000
#define kfLblEllipsis 0x0800
#define kfLblHitTest 0x0400

const int kcbLabelTextMax = 1000;

class LabelControl : public Control // lbl
{
public:
	LabelControl();
	virtual ~LabelControl();
	bool Init(int nfnt, char *pszLabel, char *pszFlags1, char *pszFlags2);

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual int OnHitTest(Event *pevt);

	virtual void SetText(const char *psz);

protected:
	virtual void CalcRect();

	char *m_szLabel;
	int m_nfnt;
};

class EcomTextControl : public LabelControl // ect
{
public:
	EcomTextControl();
	virtual ~EcomTextControl();

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual int OnHitTest(Event *pevt);
	virtual void SetText(char *psz);

	// new methods

	bool ShowMoreText();
	void ShowAll();
	void DrawText(DibBitmap *pbm, Font *pfnt, char *psz, int x, int y, int cx, int cchMax);

private:
	void CalcRect();

	int m_cchCur;
	dword m_aiclrEcom[4];
	long m_ctPrevTime;
};

const int kctEcomOutputInterval = kctUpdate; //	how often we call the timer

// Help Control

// define types

#define knChunkRawText 0
#define knChunkBitmap 1
#define knChunkLinkText 2
#define knChunkLargeText 3
#define knChunkGenericTag 4
#define knChunkAniData 5
#define knChunkHRTag 6

typedef struct Chunk {
	int nType;
	char *psz;
	int cch;
	DibBitmap *pbm;
	void *pv;
	char szText[30];
	bool fLargeFont;
} Chunk;

typedef struct HitTest {
	int x;
	int y;
    int nchBuffer;
	int cch;
    int dBest;
    bool fHit;
	char szText[30];
} HitTest;

// Positioning Constants
#define knFindPosRunToIndex 0
#define knFindPosAtLeastY 1
#define knFindPosAtMostY 2
#define knFindPosFingerScroll 3

typedef struct FindPositionHelper {
	int nDistY;
	int nIndex;
	int cyControl;
	int cySpan;
	bool fLargeFontSpanMet;
	int nchSpanMet;
	bool fLargeFontLastHR;
	int nchLastHR;
	bool fLargeFontFirstHR;
	int nchFirstHR;
	int nCondition;
	bool fLargeFont;
} FindPositionHelper;

#define knHelpControlBRHeight (gapfnt[kifntDefault]->GetHeight() * 3)

typedef bool (*ChunkProc)(int x,int y, int cx, int cy, int nchBuffer, int yCurrent, int nyBottom, Chunk *pchk, void *pv);

#define kfHelpScrollPosition 0x8000

class HelpControl : public Control, Timer
{
public:
	HelpControl();
	~HelpControl();

	bool FollowLink(const char *pszLink, int cch = -1);
	void DoNextPage();
	void DoPrevPage();
	void DoIndex();
	void DoBack();
	bool SetFile(const char *pszFile);

	// Control overrides

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnPenEvent(Event *pevt);

private:
    virtual void OnTimer(long tCurrent);

	bool Layout(dword nchStart, bool fLargeFont, DibBitmap *pbm, ChunkProc pfn, void *pv);
	int LineHeight(bool);
	int FindPrevPosition(int nchFrom, int cyAmount, bool *pfLargeFont);
	int FindNextPosition(int nchFrom, int cySpan, bool *pfLargeFont,
            bool fCondition, bool fSmooth);
    void GetSubRects(Rect *prcInterior, Rect *prcScrollPos = NULL);
    void DragScroll(int y);

	File *m_pfil;
    int m_cb;
	bool m_fLargeFont;
	char m_szText[129];
	int m_nchCurrent;
	int m_nchBack[10];
	int m_cyPageAmount;
    bool m_fDrag;
    int m_yDrag;
    FlickScroller m_flics;
    int m_yDragUp;
    bool m_fTimerAdded;
    HitTest m_hittest;
};

void Help(const char *pszAnchor = NULL, bool fPauseSimulation = false, const char *pszFile = NULL);
void CutScene(const char *pszScene, bool fPauseSimulation);

// List Control

#define kfLstcBorder 0x8000
#define kfLstcScrollPosition 0x4000
#define kfLstcKeepInteriorPositioning 0x2000

#define kfLstTabEllipsis 1
#define kfLstTabCenter 2
#define kfLstTabCenterOn 4
#define kfLstTabRight 8

struct ListItem;

class ListControl : public Control, IControlEventHandler, Timer // lstc
{
public:
	static bool InitClass();
	static void ExitClass();

	ListControl();
	virtual ~ListControl();

	void Clear();
	bool Add(ListItem *pli);
	bool Add(const char *psz, void *pvData = NULL);
//	ListItem *EnumItemPtr(Enum *penum);
	int GetSelectedItemIndex();
	int HitTestItem(int x, int y);	// item under the point (form manager coords) or -1
	void HoverSelect(int ili);	// select an item the mouse is over: no scrolling, no sound
	ListItem *GetItem(int iItem);
	bool IsUserClick() { return m_fUserClick; }	// notifying because of a click
	ListItem *GetSelectedItem();
	void *GetSelectedItemData();
	void Select(int iItem, bool fOnly = false, bool fMakeCenter = false);
	int GetCount();
    void SetTabStops(int x0, int x1 = -1, int x2 = -1, int x3 = -1);
    void SetTabFlags(word wf0, word wf1 = 0, word wf2 = 0, word wf3 = 0);
	virtual void GetSubRects(Rect *prcInterior, Rect *prcUpArrow = NULL,
            Rect *prcDownArrow = NULL, Rect *prcScrollPosition = NULL);
    void SetScrollPosColorIndex(int iclr) { m_iclrScrollPos = iclr; }

	// IControlEventHandler methods

	virtual void OnControlSelected(word idc);
	virtual bool OnControlHeld(word idc);
	virtual void OnControlNotify(word idc, int nNotify);

	// Control overrides

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnSelect(int nSelect);
	virtual void OnPenEvent(Event *pevt);

    // Timer methods

	virtual void OnTimer(long tCurrent);

	static TBitmap *s_ptbmScrollUpUp;
	static TBitmap *s_ptbmScrollUpDown;
	static TBitmap *s_ptbmScrollDownUp;
	static TBitmap *s_ptbmScrollDownDown;

protected:
    virtual void OnPenEvent2(Event *pevt);
	virtual void DrawItem(DibBitmap *pbm, ListItem *pli, int x, int y,
            int cx, int cy);
    virtual void DrawText(DibBitmap *pbm, char *psz, int x, int y,
            int cx, int cy, word wf);

private:
	bool NeedsScrollUpArrow();
	bool NeedsScrollDownArrow();
	int GetVisibleItemCount();
    void DragScroll(int y);

private:
	ListItem *m_pliFirst, *m_pliLast;
	int m_nfnt;
	int m_cli;
	int m_iliTop;
    int m_cxEllipsis;
    int m_yDrag;
    int m_yDragUp;
    bool m_fTimerAdded;
    bool m_fDrag;
    int m_iliTopDrag;
    FlickScroller m_flics;
    int m_iclrScrollPos;
    bool m_fPenDown;
    bool m_fUserClick;

protected:
	int m_cyItem;
    int m_axTab[4];
    word m_awfTab[4];
};

const int kidcScrollDownButton = 0x7fff;
const int kidcScrollUpButton = 0x7ffe;

struct ListItem {
	ListItem *pliNext;
	union {
		char szText[80];
		struct {
			AnimationData *panid;
			int nStrip;
			int nFrame;
		} Anim;
	};
	void *pvData;
	bool fSelected;
	bool fDisabled;
};

class BuilderGob;
class BuildQueue;

// Animation List Control

class BuildListControl : public ListControl // alc
{
public:
	// ListControl overrides

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void DrawItem(DibBitmap *pbm, ListItem *pli, int x, int y, int cx, int cy);
	virtual void GetSubRects(Rect *prcInterior, Rect *prcUpArrow = NULL,
            Rect *prcDownArrow = NULL, Rect *prcScrollPos = NULL);

	bool Add(AnimationData *panid, int nStrip, int nFrame, void *pvData, bool fDisabled = false);
	void SetQueueInfo(BuilderGob *pbldr, BuildQueue *pbq) { m_pbldr = pbldr; m_pbq = pbq;}

private:
	BuilderGob *m_pbldr;
	BuildQueue *m_pbq;
};

// Bitmap Control

class BitmapControl : public Control // bmc
{
public:
	BitmapControl();
	virtual ~BitmapControl();

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);

	void SetBitmap(HtBitmap *phtbm);

private:
	HtBitmap *m_phtbm;
};

// Slider Control

class SliderControl : public Control // sldr
{
public:
	SliderControl();
	~SliderControl();

	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnSelect(int nSelect);
	virtual void OnPenEvent(Event *pevt);

	void SetRange(int nMin, int nMax);
	void SetValue(int n);
	int GetValue();

private:
	int m_nMin, m_nMax;
	int m_nValue;
};

inline SliderControl::~SliderControl() {
};

// MiniMap Control

#define kfMmVertBorderInside 1
#define kfMmRedraw 2
#define kfMmHasPoweredRadar 4

class MiniMapControl : public Control // mmc
{
public:
	MiniMapControl();
	virtual ~MiniMapControl();

	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnPenEvent(Event *pevt);
	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);

	void Redraw();
	void RedrawTRect(TRect *ptrc);
	void RedrawTile(TCoord tx, TCoord ty);
	void Update();
	bool CalcPoweredRadar();
	static int CalcWidth();

private:
	static void CalcLayout(int *pnBase, int *pcxDisp, int *pcyDisp, int *pnPpt);

	long m_tInvalidateLast;
	int m_nScale;		// terrain dib pixels per tile (1 or 2)
	int m_nPpt;			// display pixels per tile, 16.16
	int m_cxDisp;
	int m_cyDisp;
	int *m_axSrc;
	int *m_aySrc;
	int m_cyInputUI;
	int m_cxyBorder;
	DibBitmap *m_pbm;
	word m_wfMm;
	int m_xOff;
	int m_yOff;
	TCoord m_ctx;
	TCoord m_cty;
	byte *m_pbTileData;
	word *m_pwTileMap;
	int m_cbRowBytes;
	byte *m_pbFogMap;
	byte m_clrWhite;
	byte m_clrBlack;
	byte m_clrGalaxite;
	byte m_aclrSide[kcSides];
	byte *m_pbFogLevel;		// per display pixel: fog coverage 0 clear .. 255 black
	dword *m_pdwColor;		// per display pixel: terrain colour, 0xRRGGBB
	byte *m_pbNearest;		// 15 bit RGB -> nearest palette index (0 = not yet; index + 1)
	Palette *m_ppal;
	byte NearestColor(int r, int g, int b);
	int FogCoverage(int tx, int ty);
};

// Pip Meter Control

class PipMeterControl : public Control // btn
{
public:
	static bool InitClass();
	static void ExitClass();

	PipMeterControl();
	virtual ~PipMeterControl();

	virtual bool Init(Form *pfrm, word idc, int x, int y, int cx, int cy,
			char *szPip = NULL);
	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);

	void SetValue(int nValue);

private:
	virtual bool Init(char *szPip);

protected:
	TBitmap *m_ptbmPip;
	int m_nValue;

private:
	static TBitmap *s_ptbmPip;
};

// Damge Meter Control

struct UnitConsts;

class DamageMeterControl : public Control // btn
{
public:
	static bool InitClass();
	static void ExitClass();

	DamageMeterControl();
	virtual ~DamageMeterControl();

	virtual void OnPaint(DibBitmap *pbm);

	void SetUnitConsts(UnitConsts *puntc);

private:
	UnitConsts *m_puntc;
	static TBitmap *s_ptbmInfantry;
	static TBitmap *s_ptbmVehicle;
	static TBitmap *s_ptbmStructure;
};

// Credits Control

class CreditsControl : public Control
{
public:
	CreditsControl() {m_cCreditNeeders = 0;m_fDrawCreditSymbol = true;};
	void Update(long nCredits, word cCreditNeeders);

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);

private:
	long m_nCredits;
	bool m_fDrawCreditSymbol;
	word m_cCreditNeeders;
};

// Power Control

class PowerControl : public Control
{
public:
	PowerControl() {m_fShowPowerSymbol = true; m_fPowerLow = false;};
	void Update(int nDemand, int nSupply);

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);
	virtual void OnPaint(DibBitmap *pbm);

private:

	int m_nDemand, m_nSupply;
	int m_nDemandMax;
	bool m_fShowPowerSymbol;
	bool m_fPowerLow;
};

// InputUIForm

class InputUIForm : public Form, public Timer
{
public:
	InputUIForm();
	~InputUIForm();
	void Update();

	// form

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual bool EventProc(Event *pevt);
	virtual void OnControlSelected(word idc);
	virtual bool OnControlHeld(word idc);

	// timer

	virtual void OnTimer(long tCurrent);

private:
	bool m_fTimerAdded;
	long m_msLastTimer;
	double m_wcScrollRemainder;
	long m_cmsCheckScroll;
	int m_xSelectionInfo;
	int m_cxSelectionInfo;
	char m_szSelectionInfo[200];

	void UpdateSelectionInfo();
	void InGameMenu();
	void TestOptions();
};

const int kfMasSelect = 1;
const int kfMasMove = 2;
const int kfMasAttack = 4;
const int kfMasShowMenu = 8;

// SimUIForm

class MobileUnitGob;
class UnitMenu;

class SimUIForm : public Form, public Timer
{
public:
	SimUIForm();
	virtual ~SimUIForm();
	void Update();
	void CalcLevelSpecificConstants();
	void InvalidateDragSelection();
    Gob *HitTestGob(int x, int y, WCoord *pwx, WCoord *pwy);
	void MoveOrAttackOrSelect(int x, int y, dword ff);
    void MoveOrAttackOrSelect(Gob *pgobHit, WCoord wxTarget, WCoord wyTarget,
            dword ff);
    void ShowUnitMenu(Gob *pgob);
    void SelectSameUnitTypes(Gob *pgob, bool fSfx);
    bool IsSelectionCommand(Gob *pgobHit);
    void ClickSelect(Gob *pgob, dword ffMods);
    bool DeliverSelectedMiners(Gob *pgobProcessor, bool fQueue);
    void PlaySelectSfx(Gob *pgob);

    // The name label shown while the mouse rests on a unit or building
    void SetHover(bool fHover, int x = 0, int y = 0);

    // Keyboard commands (InputUIForm passes keys on)
    bool OnBattlefieldKey(Event *pevt);
    void CenterViewOn(WCoord wx, WCoord wy);

    class PenHandler {
    public:
        virtual ~PenHandler() {}
        virtual bool OnPenEvent(Event *pevt, bool fScrollOnly) = 0;
        virtual void OnPaint(DibBitmap *pbm) = 0;
        virtual void CheckScroll() = 0;
    };

    PenHandler *GetPenHandler() {
        return m_ppenh;
    }

	// Form methods

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual bool EventProc(Event *pevt);
	virtual bool OnPenEvent(Event *pevt);
	virtual void OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque);
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnPaintControls(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual void ScrollInvalidate(UpdateMap *pupd);
    virtual void FrameComplete();
    
	// Timer methods

	virtual void OnTimer(long tCurrent);

private:
    void CalcHoverLabel(Rect *prc, const char **ppsz);
    void UpdateHoverLabel(UpdateMap *pupd);
    void DrawHoverLabel(DibBitmap *pbm);

    bool m_fHover;
    int m_xHover, m_yHover;
    Rect m_rcHover;
    const char *m_pszHover;

    PenHandler *m_ppenh;

    class StylusHandler : public PenHandler {
    public:
        StylusHandler(SimUIForm *psui);
        virtual ~StylusHandler();
        virtual bool OnPenEvent(Event *pevt, bool fScrollOnly);
        virtual void OnPaint(DibBitmap *pbm) {}
        virtual void CheckScroll();
    private:
        SimUIForm *m_psui;
        bool m_fDragging;

        // The click that started this drag
        dword m_ffMods;
        Gid m_gidDown;

        // Shift+drag adds to the units that were selected before it
        Gid *m_agidKeep;
        int m_cgidKeep;

        void OnPenDown(Event *pevt);
        void OnPenUp(Event *pevt);
        void OnPenDrag(Event *pevt);
        void CancelModes();
        void UpdateDragSelection();
    };

	MobileUnitGob *SetSelectionTargets(Gid gid, WCoord wxTarget,
            WCoord wyTarget);
    void OnRightClick(int x, int y, bool fQueue = false, bool fForce = false);
    void OnGuardClick(int x, int y, bool fQueue);
    void ScrollViewBy(WCoord dwx, WCoord dwy);

    int GetOwnSelection(Gid *agid, int cgidMax, bool fMobileOnly);


    // Middle-button panning
    bool m_fPanning;
    int m_xPanLast;
    int m_yPanLast;

	static bool s_fReadyForPaint;
    
	bool m_fTimerAdded;
#ifdef STATS_DISPLAY
	bool m_fShowStats;
#endif
	Animation m_aniMoveTarget;
	WCoord m_wxMoveTarget;
	WCoord m_wyMoveTarget;
	int m_nStateMoveTarget;
};

class MissionList;

class SelectMissionForm : public ShellForm
{
public:
	SelectMissionForm(MissionList *pml, const MissionIdentifier *pmiidFind);
    bool GetSelectedMission(MissionIdentifier *pmiid);

	// Form overrides

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);
	virtual void OnControlNotify(word idc, int nNotify);
    virtual bool OnPenEvent(Event *pevt);

private:
    MissionType InitLists(int iMissionSelect);
    int IndexFromMissionType(MissionType mt);
    MissionType MissionTypeFromIndex(int i);
    int GetSelectedMissionIndex(ListControl *plstc);
    bool IsSelectedMissionLocked(ListControl *plstc);
    void SwitchToMissionType(MissionType mt);
    void UpdateDescription();

    int m_fMagicUnlock;
    int m_cMagic;
    MissionList *m_pml;
    const MissionIdentifier *m_pmiidFind;
    MissionType m_mt;
    ListControl *m_aplstc[3];
};

// LoadGameForm

class LoadGameForm : public ShellForm
{
public:
	LoadGameForm();
	void SelectLast(bool fLast);

	// Form overrides

	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	int m_nGameLast;
};

class TestOptionsForm : public DialogForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);
};

class MemoryUseForm : public DialogForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	void UpdateLabels();
};

//
// Triggers, Conditions, Actions and Condition/Action parameter types
//

// Condition/Action parameter types

class QualifiedNumber
{
public:
	bool Parse(char **ppsz);
	bool Compare(long nNumber);

	int m_nQualifier;
	long m_nNumber;
};

// class to run a countdown timer managed by triggers and
// shown in a SimUIForm control.

#define kfCtVisibleAtStart 1
#define kfCtRunning 2

class CountdownTimer
{
public:
	CountdownTimer();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);

	void SetTimer(int csecs, char *pszFormatString);
	bool GetTimer(int *psecs);
	void StartTimer(bool fStart);
	void ShowTimer(bool fShow);
	void Update();

	word GetFlags() {
		return m_wf;
	}

private:
	void UpdateString();

	int m_secs;
	char m_szFormat[80];
	word m_wf;
	long m_tLast;
};

// Conditions

class Condition // cdn
{
public:
	Condition();
	virtual ~Condition() {}
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side) = 0;
	Condition *m_pcdnNext;
};

class MissionLoadedCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	MissionLoadedCondition();
	virtual bool IsTrue(Side side);
};

class CreditsCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	CreditsCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
	QualifiedNumber m_qnum;
};

class OwnsUnitsCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	OwnsUnitsCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
	QualifiedNumber m_qnum;
	UnitMask m_um;
};

class AreaContainsUnitsCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	AreaContainsUnitsCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
	QualifiedNumber m_qnum;
	UnitMask m_um;
	int m_nArea;
	int m_nVersionLevel;
};

class PlaceStructureModeCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	PlaceStructureModeCondition();
	virtual bool IsTrue(Side side);
};

class MinerCantFindGalaxiteCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	MinerCantFindGalaxiteCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
};

class GalaxiteCapacityReachedCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	GalaxiteCapacityReachedCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
};

class ElapsedTimeCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	ElapsedTimeCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	QualifiedNumber m_qnum;
};

class SwitchCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	SwitchCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	int m_iSwitch;
	bool m_fOn;
};

class PeriodicTimerCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	PeriodicTimerCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	int m_iTimer;
};

class CountdownTimerCondition : public Condition
{
public:
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	QualifiedNumber m_qnum;
};

class DiscoversSideCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	DiscoversSideCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMaskA;
	word m_wfCaSideMaskB;
};

class TestPvarCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	TestPvarCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	char m_szName[kcbPvarNameMax];
	QualifiedNumber m_qnum;
};

class HasUpgradesCondition : public Condition
{
public:
	// REMOVE_SOMEDAY: keep m68k-gcc from generating default constructor in .text section
	HasUpgradesCondition();
	virtual bool Init(char *psz);
	virtual bool IsTrue(Side side);

private:
	word m_wfCaSideMask;
	UpgradeMask m_upgm;
};

// Actions

class TriggerAction // actn
{
public:
	TriggerAction();
	virtual ~TriggerAction();
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side) = 0;
	TriggerAction *m_pactnNext;

};

class PreserveTriggerAction : public TriggerAction
{
public:
	virtual bool Perform(Trigger *ptgr, Side side);
};

class WaitAction : public TriggerAction
{
public:
	WaitAction();
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	dword m_ctWait;
	dword m_atStartSide[kcSides];
	bool m_afWaitingSide[kcSides];
};

class CenterViewAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nArea;
};

class SetNextMissionAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char m_szLevel[kcbFilename];
};

class EndMissionAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);


private:
	int m_nWinLose;
};

class EcomAction : public TriggerAction
{
public:
	virtual ~EcomAction();
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nBackground;
	bool m_fMore;
	int m_nCharFrom;
	int m_nCharTo;
	char *m_pszMessage;
};

class SetAllowedUnitsAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	UnitMask m_um;
};

class SetAllowedUpgradesAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	UpgradeMask m_upgm;
};

class SetUpgradesAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	UpgradeMask m_upgm;
};

class AlliesAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMaskA;
	word m_wfCaSideMaskB;
};

class SetObjectiveAction : public TriggerAction
{
public:
	SetObjectiveAction();
	virtual ~SetObjectiveAction();

	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	char *m_szObjective;
};

class SetSwitchAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_iSwitch;
	bool m_fOn;
};

class DefogAreaAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nArea;
};

// This is the "CreateUnitGroup-Action", not "Create-UnitGroupAction"

class CreateUnitGroupAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nUnitGroup;
};

class HuntAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask1;
	UnitMask m_um1;
	word m_wfCaSideMask2;
	UnitMask m_um2;
};

// This is the "CreateRandomUnitGroup-Action", not "Create-RandomUnitGroupAction"

class CreateRandomUnitGroupAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);
};

// Start Countdown Action

class StartCountdownAction : public TriggerAction
{
public:
	StartCountdownAction();
	virtual ~StartCountdownAction();

	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nSecs;
	char *m_szCountdown;
};

// Modify countdown action

class ModifyCountdownAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_nAction;
};

class RepairAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	bool m_fOn;
};

class EnableReplicatorAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	bool m_fOn;
};

// Modify Credits action

class ModifyCreditsAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	int m_nAmount;
	int m_nAction;
};

// Move Units In Area action

class MoveUnitsInAreaAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	word m_wfCaSideMask;
	UnitMask m_um;
	int m_nAreaSrc;
	int m_nAreaDst;
};

// Set Formal Objective Text action

const int kcchObjectiveMax = 80;
const int kcchObjectiveStatusMax = 15;
const int kcchObjectiveInfoMax = 350;

class SetFormalObjectiveTextAction : public TriggerAction
{
public:
	SetFormalObjectiveTextAction();
	virtual ~SetFormalObjectiveTextAction();

	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_iObjective;
	char *m_szObjective;;
};

// Set Formal Objective Status action

class SetFormalObjectiveStatusAction : public TriggerAction
{
public:
	SetFormalObjectiveStatusAction();
	~SetFormalObjectiveStatusAction();

	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	int m_iObjective;
	char *m_szStatus;
};

// Set Formal Objective Info action

class SetFormalObjectiveInfoAction : public TriggerAction
{
public:
	SetFormalObjectiveInfoAction();
	virtual ~SetFormalObjectiveInfoAction();

	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char *m_szInfo;
};

// Show Objectives action

class ShowObjectivesAction : public TriggerAction
{
public:
	virtual bool Perform(Trigger *ptgr, Side side);
};

// Show Cut Scene action

class CutSceneAction : public TriggerAction
{
public:
	CutSceneAction();
	virtual ~CutSceneAction();
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char *m_pszMessage;
};

// Jump To Mission action

class JumpToMissionAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char m_szLevel[kcbFilename];
};

// For the moment, only actions can "block".
// Per-side context is stored in the actions that need it for blocking.

class Trigger
{
public:
	Trigger();
	~Trigger();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	bool Init(IniReader *pini, FindProp *pfind);
	void Execute(Side side, bool fForce = false);
	void Arm(Side side);

private:
	bool LoadCondition(IniReader *pini, FindProp *pfind);
	bool LoadAction(IniReader *pini, FindProp *pfind);

	Condition *m_pcdn;
	TriggerAction *m_pactn;
	TriggerAction *m_apactnLast[kcSides];
	bool m_afArmed[kcSides];

};

// Modify Persistent Variable action

class ModifyPvarAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char m_szName[kcbPvarNameMax];
	int m_nAmount;
	int m_nAction;
};

// Set Persistent Variable Text action

class SetPvarTextAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char m_szName[kcbPvarNameMax];
	char m_szValue[kcbPvarValueMax];
};

// ShowAlert action

class ShowAlertAction : public TriggerAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(Trigger *ptgr, Side side);

private:
	char m_szAlert[80];
};

// Raised in 2.0 (were 128 and 16)
#define kcTriggersPerSide 1024
#define kcSwitchMax 64
#define kcTriggerTimersMax 16
#define kctTimerNotStarted -100000

class TriggerMgr // tgrm
{
public:
	TriggerMgr();
	~TriggerMgr();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	bool Init(IniReader *pini);
	void Exit();
	void Update();
	void SetConditionTrue(int nCondition, SideMask sidm);
	bool IsConditionTrue(int nCondition, SideMask sidm);

	CountdownTimer *GetCountdownTimer() {
		return &m_cdt;
	}

	void SetSwitch(int iSwitch, bool fOn) {
		m_abSwitch[iSwitch] = (byte)fOn;
	}

	bool GetSwitch(int iSwitch) {
		return m_abSwitch[iSwitch] != 0;
	}

	int AddPeriodicTimer(int ctPeriod) {
		Assert(m_cTimers < kcTriggerTimersMax);
		m_actPeriod[m_cTimers] = ctPeriod;
		m_actCountdown[m_cTimers] = kctTimerNotStarted;
		return m_cTimers++;
	}

	// Periodic timers only start counting the first time they are tested.
	// This way they can sit 'below' other conditions and have deterministic
	// firing times.

	void StartPeriodicTimer(int iTimer) {
		if (m_actCountdown[iTimer] == kctTimerNotStarted)
			m_actCountdown[iTimer] = m_actPeriod[iTimer];
	}

	bool IsPeriodicTimerTriggered(int iTimer) {
		return m_actCountdown[iTimer] <= 0;
	}


	void Enable(bool fEnable) {
		m_fEnabled = fEnable;
	}

private:
	bool AssignTriggerSides(int ntgr, char *psz);

	int m_ctgr;
	Trigger *m_atgr;
	// Each side's triggers in order, ended by 0xffff
	word m_mpSide2nTrigger[kcSides][kcTriggersPerSide + 1];
	SideMask m_asidmCondition[knConditionMax];
	byte m_abSwitch[kcSwitchMax];
	int m_cTimers;
	int m_actPeriod[kcTriggerTimersMax];
	int m_actCountdown[kcTriggerTimersMax];
	long m_tLastUpdate;
	bool m_fEnabled;
	CountdownTimer m_cdt;

};

// UnitGroup
// - directs the process of building all the Units in the group
// - needs to know when Units under its direction complete commands given to them (or deactivate)
// - loads Unit list and UnitGroupActions (called by UnitGroupMgr at Level load time)
// - loads/saves itself

const byte kfUleBuilt = 0x01;
const byte kfUleReplicant = 0x02;

struct UnitListEntry {
	UnitType ut;
	Gid gid;
	byte bf;
};

class UnitGroupAction;	// forward declaration

class UnitGroup
{
public:
	UnitGroup();
	~UnitGroup();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	bool Init(IniReader *pini, int iug);
	void Activate();
	void Update();
	void ReorderMissingUnits();
	void OnBuilt(UnitGob *punt);
	void AddUnit(UnitGob *punt, bool fReplicant);
	void RemoveReplicants();

	word GetFlags() {
		return m_wf;
	}

	void SetFlags(word wf) {
		m_wf = wf;
	}

	Player *GetOwner() {
		return m_pplr;
	}

	UnitListEntry *GetUnitList() {
		return m_aule;
	}

	int GetUnitCount() {
		return m_cule;
	}
#ifdef DEBUG
	char *GetName() {
		return m_szName;
	}
#endif

private:
	bool LoadAction(IniReader *pini, FindProp *pfind);

	word m_wf;
	UnitGroupAction *m_pactn;
	UnitGroupAction *m_pactnLast;
	Player *m_pplr;
	int m_cule;
	UnitListEntry *m_aule;
	word m_wfMunt;		// flags for built MobileUnits
	int m_nSpawnArea;
	int m_nHealth;		// percent
#ifdef DEBUG
	char m_szName[64];
#endif

};

const word kfUgNeedsUnit = 0x0001;
const word kfUgActive = 0x0002;
const word kfUgLoopForever = 0x0004;
const word kfUgCreateAtLevelLoad = 0x0008;
const word kfUgRandomGroup = 0x0010;
const word kfUgReplaceGroup = 0x0020;
const word kfUgSpawn = 0x0040;
const word kfUgWaitingToReplace = 0x0080;
const word kfUgActivatedBefore = 0x0100;
const word kfUgNotRecentlyActivated = 0x0200;

// UnitGroupMgr
// - Updates the active UnitGroups
// - deactivates groups when the contained units are all deactivated
// - loads group data from Level file
// - loads/saves itself and UnitGroups

class UnitGroupMgr // tgrm
{
public:
	UnitGroupMgr();
	~UnitGroupMgr();

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	bool Init(IniReader *pini);
	void Exit();
	void Update();
	void ActivateUnitGroup(int iug);
	void ActivateRandomUnitGroup();
	bool CreateAtLevelLoadGroups();
	UnitGroup *GetUnitGroup(Gid gid);
	UnitGroup *GetUnitGroupByIndex(int iug) {
		return iug >= 0 && iug < m_cug ? &m_aug[iug] : NULL;
	}

private:
	int m_cug;
	UnitGroup *m_aug;

};

// UnitGroup Actions

class UnitGroupAction
{
public:
	UnitGroupAction();
	virtual ~UnitGroupAction() {}
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz) = 0;
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug) = 0;

	UnitGroupAction *m_pactnNext;

};

class WaitUnitGroupAction : public UnitGroupAction
{
public:
	WaitUnitGroupAction();
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	dword m_ctWait;
	long m_tStart;
	bool m_fWaiting;
};

class SetSwitchUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(UnitGroup *pug);

private:
	int m_iSwitch;
	bool m_fOn;
};

class MoveUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	bool m_fWaiting;
	int m_nArea;
};

class AttackUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	// These are initialized from the loaded level

	UnitMask m_um;
	word m_wfCaSideMask;
	dword m_ctWait;

	// These are persisted in the save game

	bool m_fWaiting;
	long m_tStart;
	Gid m_gidTarget;
};

class GuardUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	// These are initialized from the loaded level

	dword m_ctWait;

	// These are persisted in the save game

	bool m_fWaiting;
	long m_tStart;
};

class MineUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool Init(char *psz);
	virtual bool Perform(UnitGroup *pug);
};

class GuardVicinityUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	// These are initialized from the loaded level

	dword m_ctWait;

	// These are persisted in the save game

	bool m_fWaiting;
	long m_tStart;
};

// Protect Group <unit group> for <seconds>: each member guards (follows and
// protects) a unit of the other group

class ProtectGroupUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	int m_iugProtect;
	dword m_ctWait;
	bool m_fWaiting;
	long m_tStart;
};

// Protect In Area <area> for <seconds>: the group moves into the area, then
// guards it

class ProtectAreaUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	int m_nArea;
	dword m_ctWait;
	int m_nPhase;		// 0: start, 1: moving there, 2: guarding
	long m_tStart;
};

// Attack In Area <unit types> owned by <sides> in <area> for <seconds>: like
// Attack, but only targets in the area; with none there the group waits in
// the area

class AttackAreaUnitGroupAction : public UnitGroupAction
{
public:
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool Init(char *psz);
	virtual void Reset();
	virtual bool Perform(UnitGroup *pug);

private:
	UnitMask m_um;
	word m_wfCaSideMask;
	int m_nArea;
	dword m_ctWait;
	bool m_fWaiting;
	bool m_fMovedToArea;
	long m_tStart;
	Gid m_gidTarget;
};

// BuildMgr
// - directs BuilderGobs to build desired Units
// - keeps track of which BuilderGobs are building which Units for which UnitGroups
// - is notified when a BuilderGob completes a build (and what Unit was
//   completed and for which UnitGroup)

struct BuildEntry {
	BuildEntry *pbldeNext;
	UnitType ut;
	int nArea;
	UnitGroup *pug;
	Gid gidBuilder;
};

class BuildMgr
{
public:
	BuildMgr();
	~BuildMgr();
	bool BuildUnit(UnitType ut, UnitGroup *pug, int nArea);
	void OnBuilt(UnitGob *punt, BuilderGob *pbldr);
	void Update();

private:
	BuildEntry *m_pbldeFirst;
};

// Simulation

// Use min gob count for 68K build because of stack variables

#define kcpgobMax 32000	// pool ceiling (gids are 15 bit); was 768 on the original devices.
								// The pool for a level is sized by SetGobLimitsForMap.
#define kcpgobVisibleMax kcpgobMax

class Simulation : public TimerMgr // sim
{
public:
	Simulation();
	~Simulation();

	bool OneTimeInit();
	void OneTimeExit();
	bool PerLevelInit();
	void PerLevelExit();

	bool LoadLevel(const char *pszLevelName);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	void ClearGobSelection();
	void SetGobSelected(Gob *pgob);
	void SelectSameUnitTypes(UnitGob *punt, TRect *ptrc);

	Level *GetLevel() {
		return m_plvl;
	}

	int GetUpdateCount() {
		return m_cUpdates;
	}

	void SetMiniMapScale(int nScale) {
		m_nMiniMapScale = nScale;
	}

	int GetMiniMapScale() {
		return m_nMiniMapScale;
	}

	BuildMgr *GetBuildMgr() {
		return m_pbldm;
	}

	void Update(CommandQueue *pcmdq);
	void Draw(UpdateMap *pupd, DibBitmap *pbm);
	void DrawBackground(UpdateMap *pupd, DibBitmap *pbm);
	void DrawFog(UpdateMap *pupd, DibBitmap *pbm);
	bool SetViewPos(WCoord wx, WCoord wy, bool fInit = false);
	void GetViewPos(WCoord *pwx, WCoord *pwy);
	bool HitTest(Enum *penm, WCoord wx, WCoord wy, word wf, Gob **ppgob);
	void Pause(bool fPause);
	bool IsPaused();
	void FindVisibleGobs(Gob ***pppgobVisible = NULL, int *pcgobVisible = NULL);
	void SetSelection(Rect *prc);

	// Smooth motion: true while the simulation is running and frames
	// in between updates should be drawn with interpolated positions
	bool IsSmoothMotionActive();
	int GetInterpolationAlpha();

	// TimerMgr override

	virtual long GetTickCount();
	virtual void AddTimer(Timer *ptmr, long ct); //stub


private:

	Level *m_plvl;
	WCoord m_wxView, m_wyView;
	bool m_fGameOver;
	int m_cUpdates;		// Can count 80 ms updates for 5+ years without wrapping
	long m_tCurrent;
	bool m_fPaused;
	int m_nMiniMapScale;
	BuildMgr *m_pbldm;

	Gob *m_apgobVisible[kcpgobVisibleMax];
	int m_cgobVisible;
	WCoord m_wxViewSave;
	WCoord m_wyViewSave;
	int m_cUpdatesSave;
	int m_cupdTriggerMgrUpdateLast;
	long m_msLastUpdate;
};
extern Simulation gsim;

// Minimap coord from World coord

inline int MmcFromWc(WCoord wc) {
	return ((wc + 0x007f) & ~0x007f) >> (9 - gsim.GetMiniMapScale());	// == 7 for hires, 8 for lores
}

// Level

// NOTE: changing the sizes any of the 'int's used in the types and
// member variables below will break .ini and .lvl file reading which
// uses the '%d' feature of IniScanf which always stores parsed numbers
// as 'int's.

struct SideInfo	// sidi
{
	int nInitialCredits;
	WPoint wptInitialView;
	int nIntelligence;
	int cStructuresInitial;
	int cMobileUnitsInitial;
};

class Level // lvl
{
public:
	Level();
	~Level();
	bool Init(const char *pszLevelName, bool fConstantsOnly = false);

	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	bool LoadLevelInfo(const char *pszLevelName, IniReader *piniLoaded = NULL);

	TileMap *GetTileMap() {
		return m_ptmap;
	}

	Palette *GetPalette() {
		return m_ppal;
	}

	byte *GetShadowMap() {
		return m_mpiclriclrShadow;
	}

	FogMap *GetFogMap() {
		return m_pfogm;
	}

	TerrainMap *GetTerrainMap() {
		return m_ptrmap;
	}

	const char *GetTitle() {
		return m_szTitle;
	}

	SideInfo *GetSideInfo(Side side) {
		return &m_asidi[side];
	}

	TriggerMgr *GetTriggerMgr() {
		return &m_tgrm;
	}

	UnitGroupMgr *GetUnitGroupMgr() {
		return &m_ugm;
	}

	char *GetFilename() {
		return m_szFileLevel;
	}

	// This version is the "level file format" version

	int GetVersion() {
		return m_nVersion;
	}

	// This is the per-save unique revision number

private:
	bool LoadSideInfo(IniReader *pini, char *pszSideName, SideInfo *psidi);
	bool LoadLevelConstants(const char *pszLevelName, IniReader *pini);
	bool LoadLevelVariables(IniReader *pini);
	void CreateMissingWalls();

private:
	char m_szTitle[kcbLevelTitle];
	char m_szFileLevel[kcbFilename];
	int m_nPlayersMin, m_nPlayersMax;
	TileMap *m_ptmap;
	Palette *m_ppal;
	FileMap m_fmapPalette;
	byte *m_mpiclriclrShadow;
	FileMap m_fmapShadowMap;
	FogMap *m_pfogm;
	TerrainMap *m_ptrmap;
	bool m_fInitialized;
	SideInfo m_asidi[kcSides];
	TriggerMgr m_tgrm;
	UnitGroupMgr m_ugm;

	// This is the "level format" version. This is versioned upward over time and is here so
	// that WI knows what format the level was saved in.

	int m_nVersion;

	// This is the revision # of the level. So saved games with older revision numbers
	// aren't loaded. Note this isn't an incrementing version number. It is a unique
	// mark that every mission has.

	dword m_dwRevision;
};

// Structure/Unit type values and masks

const UnitMask kumShortRangeInfantry = 1L << kutShortRangeInfantry;
const UnitMask kumLongRangeInfantry = 1L << kutLongRangeInfantry;
const UnitMask kumTakeoverSpecialist = 1L << kutTakeoverSpecialist;
const UnitMask kumAndy = 1L << kutAndy;
const UnitMask kumFox = 1L << kutFox;
const UnitMask kumInfantry = kumShortRangeInfantry | kumLongRangeInfantry | kumTakeoverSpecialist | kumAndy | kumFox;

const UnitMask kumGalaxMiner = 1L << kutGalaxMiner;
const UnitMask kumLightTank = 1L << kutLightTank;
const UnitMask kumMediumTank = 1L << kutMediumTank;
const UnitMask kumMachineGunVehicle = 1L << kutMachineGunVehicle;
const UnitMask kumRocketVehicle = 1L << kutRocketVehicle;
const UnitMask kumMobileHeadquarters = 1L << kutMobileHeadquarters;
const UnitMask kumArtillery = 1L << kutArtillery;
const UnitMask kumVehicles = kumGalaxMiner | kumLightTank | kumMediumTank | kumMachineGunVehicle |
		kumRocketVehicle | kumMobileHeadquarters | kumArtillery;
const UnitMask kumMobileUnits = kumInfantry | kumVehicles;

const UnitMask kumHumanResourceCenter = 1L << kutHumanResourceCenter;
const UnitMask kumReactor = 1L << kutReactor;
const UnitMask kumProcessor = 1L << kutProcessor;
const UnitMask kumHeadquarters = 1L << kutHeadquarters;
const UnitMask kumResearchCenter = 1L << kutResearchCenter;
const UnitMask kumVehicleTransportStation = 1L << kutVehicleTransportStation;
const UnitMask kumRadar = 1L << kutRadar;
const UnitMask kumWarehouse = 1L << kutWarehouse;
const UnitMask kumReplicator = 1L << kutReplicator;

const UnitMask kumMachineGunTower = 1L << kutMachineGunTower;
const UnitMask kumRocketTower = 1L << kutRocketTower;
const UnitMask kumTowers = kumMachineGunTower | kumRocketTower;

// Walls aren't in kumStructures (or kumAll): they aren't built, counted
// against limits or hunted

const UnitMask kumWall = 1L << kutWall;

const UnitMask kumStructures = kumHumanResourceCenter | kumReactor | kumProcessor | kumHeadquarters |
		kumResearchCenter | kumVehicleTransportStation | kumRadar | kumWarehouse | kumMachineGunTower |
		kumRocketTower | kumReplicator;

// NOTE: we don't count the Replicator as a Builder so the BuildMgr won't start using it
const UnitMask kumBuilder = kumHeadquarters | kumVehicleTransportStation | kumHumanResourceCenter;
const UnitMask kumAll = kumInfantry | kumVehicles | kumStructures;
const UnitMask kumImpossible = 0xffffffff;

// Upgrade type values and masks (values to be moved to res.h most likely)

struct Upgrade { // upg
	UpgradeMask upgm;
	char szName[50];
	char szIconName[20];
	UpgradeMask upgmPrerequisites;
	UnitMask umPrerequisites;
	int nCost;
	int ctTimeToBuild;
	char *szDescription;
	char *szPrerequisites;
};

#define kupgtAdvancedHRC 0
#define kupgtAdvancedVTS 1
#define kupgtIncreasedBullpupSpeed 2
#define kupgtMax 2

const UpgradeMask kupgmAdvancedHRC = 1 << kupgtAdvancedHRC;
const UpgradeMask kupgmAdvancedVTS = 1 << kupgtAdvancedVTS;
const UpgradeMask kupgmIncreasedBullpupSpeed = 1 << kupgtIncreasedBullpupSpeed;
const UpgradeMask kupgmAll = kupgmAdvancedHRC | kupgmAdvancedVTS | kupgmIncreasedBullpupSpeed;

// Player-related types

class PlayerMgr;

// UNDONE: pull these from game.ini?

const long knWarehouseCapacity = 5000;
const long knProcessorCapacity = 3000;

// Who is using credits during a given update? Useful for sound effects

#define knConsumerGeneric 0
#define knConsumerRepair 1
#define knConsumerMax 1

const int kcFormalObjectivesMax = 4;

const int ksoObjectives = 1;
const int ksoWinSummary = 2;
const int ksoLoseSummary = 3;

class Player // plr
{
	friend class PlayerMgr;

public:
	Player();
	~Player();

	void Init(Pid pid);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	UnitMask GetUnitMask();
	void SetCredits(int nCredits, bool fAffectTotals, int nConsumer = knConsumerGeneric);
	void AddPowerSupplyAndDemand(int nPowerSupply, int nPowerDemand);
	void Repair(bool fOn);
	void Update(int cUpdates);
	void SetFormalObjectiveText(int iObjective, char *pszText);
	void SetFormalObjectiveStatus(int iObjective, char *pszStatus);
	void SetFormalObjectiveInfo(char *pszInfo);
	int ShowObjectives(int so, bool fForceInfoDisplay = false, bool fAborting = false);
	void ModifyNeedCreditsCount(int cDelta);
	char *GetFormalObjectiveText(int iObjective) {
		return m_aszFormalObjectiveText[iObjective];
	}

	char *GetFormalObjectiveStatus(int iObjective) {
		return m_aszFormalObjectiveStatus[iObjective];
	}

	char *GetFormalObjectiveInfo() {
		return m_szFormalObjectiveInfo;
	}

	word GetFlags() {
		return m_wf;
	}

	void SetFlags(word wf) {
		m_wf = wf;
	}

	Pid GetId() {
		return m_pid;
	}

	Side GetSide() {
		return m_side;
	}

	void SetSide(Side side) {
		m_side = side;
 		m_sidmAllies = GetSideMask(m_side);
		m_sidmDiscovered = GetSideMask(m_side);
	}

	SideMask GetAllies() {
		return m_sidmAllies;
	}

	void SetAllies(SideMask sidm) {
		m_sidmAllies = sidm | GetSideMask(m_side);
	}

	SideMask GetDiscoveredSides() {
		return m_sidmDiscovered;
	}

	void SetDiscoveredSides(SideMask sidm) {
		m_sidmDiscovered = sidm;
	}

	void SetDiscoverPoint(TPoint *ptpt) {
		m_tptDiscover = *ptpt;
	}

	TPoint GetDiscoverPoint() {
		return m_tptDiscover;
	}

	int GetCredits() {
		return m_nCredits;
	}

	word GetNeedCreditsCount() {
		return m_cStructsNeedCredits;
	}

	int GetCapacity() {
		return (m_acut[kutWarehouse] * knWarehouseCapacity) + (m_acut[kutProcessor] * knProcessorCapacity);
	}

	int GetPowerSupply() {
		return m_nPowerSupply;
	}

	int GetPowerDemand() {
		return m_nPowerDemand;
	}

	bool IsPowerLow() {
		return m_nPowerDemand > m_nPowerSupply;
	}

	void IncUnitCount(UnitType ut) {
		m_acut[ut]++;
	}

	void DecUnitCount(UnitType ut) {
		Assert(m_acut[ut] != 0);
		m_acut[ut]--;
	}

	int GetUnitCount(UnitType ut) {
		return m_acut[ut];
	}

    void IncUnitBuiltCount(UnitType ut) {
        m_acutBuilt[ut]++;
    }

    void DecUnitBuiltCount(UnitType ut) {
        m_acutBuilt[ut]--;
        if (m_acutBuilt[ut] < 0) {
            m_acutBuilt[ut] = 0;
        }
    }

	int GetUnitActiveCountFromMask(UnitMask um);
	int GetUnitInstanceCountFromMask(UnitMask um);

	word GetUpgrades() {
		return m_wfUpgrades;
	}

	void SetUpgrades(word wfUpgrades);

	UnitMask GetAllowedUnits() {
		return m_umAllowed;
	}

	void SetAllowedUnits(UnitMask um) {
		m_umAllowed = um;
	}

	void SetObjective(char *psz);

	char *GetObjective() {
		return m_szObjective;
	}

	UpgradeMask GetUpgradeMask() {
		return m_upgm;
	}

	UpgradeMask GetAllowedUpgrades() {
		return m_upgmAllowed;
	}

	void SetAllowedUpgrades(UpgradeMask um) {
		m_upgmAllowed = um;
	}

	void IncEnemyMobileUnitsKilled() {
		m_cmuntKilled++;
	}

	int GetEnemyMobileUnitsKilled() {
		return m_cmuntKilled;
	}

	void IncEnemyStructuresKilled() {
		m_cstruKilled++;
	}

	int GetEnemyStructuresKilled() {
		return m_cstruKilled;
	}

	void IncMobileUnitsLost() {
		m_cmuntLost++;
	}

	int GetMobileUnitsLost() {
		return m_cmuntLost;
	}

	void IncStructuresLost() {
		m_cstruLost++;
	}

	int GetStructuresLost() {
		return m_cstruLost;
	}

	int GetTotalCreditsAcquired() {
		return m_nTotalCreditsAcquired;
	}

	void ModifyTotalCreditsConsumed(int nAmount) {
		m_nTotalCreditsConsumed += nAmount;
	}

	int GetTotalCreditsConsumed() {
		return m_nTotalCreditsConsumed;
	}

	word GetHandicap() {
		return m_wfHandicap;
	}

	void SetHandicap(word wf) {
		m_wfHandicap = wf;
	}


private:
	word m_wfUpgrades;
	Side m_side;
	SideMask m_sidmAllies;
	SideMask m_sidmDiscovered;
	TPoint m_tptDiscover;
	char m_szName[kcbPlayerName];
	word m_wf;
	Pid m_pid;
	int m_nCredits;
	int m_nCreditsAcquired, m_nCreditsConsumed;
	int m_nDirCredits;
	int m_nConsumerCredits;
	int m_nPowerSupply;
	int m_nPowerDemand;
	int m_acut[kutMax];
	int m_acutBuilt[kutMax];
	UnitMask m_umAllowed;
	char m_szObjective[kcchObjectiveMax];
	UpgradeMask m_upgm;
	UpgradeMask m_upgmAllowed;
	char *m_aszFormalObjectiveText[kcFormalObjectivesMax];
	char *m_aszFormalObjectiveStatus[kcFormalObjectivesMax];
	char *m_szFormalObjectiveInfo;
	int m_cmuntKilled;
	int m_cstruKilled;
	int m_cmuntLost;
	int m_cstruLost;
	int m_nTotalCreditsAcquired;
	int m_nTotalCreditsConsumed;
	int m_cUpdatesRepairLast;
	word m_cStructsNeedCredits;
	word m_wfHandicap;
};

class PlayerMgr // plrm
{
public:
	PlayerMgr();
	~PlayerMgr();
	void Reset();
	bool Init(char *pszLevel);
	bool LoadState(Stream *pstm);
	bool SaveState(Stream *pstm);
	Player *AllocPlayer(word wf = 0);
	void Update(int cUpdates);
	void SetAllies(Player **applr, int cplrs, SideMask sidmAllies);

	Player *GetPlayer(Side side);

	Player *GetPlayerFromPid(Pid pid) {
        // Pid is unsigned 
        if (pid != kpidNeutral && pid < kcPlayersMax) {
            return &m_aplr[((int)pid)];
        }
        return NULL;
	}

	Player* GetNextPlayer(Player *pplr);

	int GetPlayerCount();
	int GetUnitInstanceCountFromMask(UnitMask um, word wfPlr);

protected:
	Player *m_aplr;
};

// Gob

#define kfGobNoEnemiesNearby	0x00000001	// No enemies nearby so don't waste time looking
#define kfGobRedraw 			0x00000002  // Something has changed that requires this gob to redraw
//#define kfGobInvalidated		0x00000004	// Gob has been invalidated
#define kfGobBeingUpgraded		0x00000008	// Gob is being upgraded
#define kfGobBeingBuilt			0x00000010	// Gob is in the process of being built, not yet ready for active duty
//#define kfGobOccupyingTerrain	0x00000020
#define kfGobFlashing			0x00000040	// Gob is flashing
#define kfGobDrawFlashed		0x00000080	// Gob will be drawn flashing
//#define kfGobRepairing			0x00000100	// unused
#define kfGobActive				0x00000200	// Gob is alive and doing its thing
#define kfGobStructure			0x00000400	// Gob is a fixed structure
#define kfGobMobileUnit			0x00000800	// Gob is a mobile unit
#define kfGobUnit				0x00001000	// Gob is a Unit (our version of RTTI!)
#define kfGobDebug				0x00002000	// Gobs DebugBreak at strategic points when this is enabled
#define kfGobSelected			0x00004000	// Gob is selected
#define kfGobStateMachine		0x00008000	// Gob has an active StateMachine that may want Update messages
#define kfGobVisibleLastFrame	0x00010000  // Gob was visible last frame
#define kfGobIncludeFindVisible 0x00020000  // Include gob in next FindVisibleGobs search
#define kfGobTransitioningToVisible 0x00040000 // Gob is going from non-visible to visible
#define kfGobAnimationChanged	0x00080000	// UpdateCount must not affect next AdvanceAnimation

#define kfGobLayerSurfaceDecal	0x00100000
#define kfGobLayerDepthSorted	0x00200000
#define kfGobLayerSmokeFire		0x00400000
#define kfGobLayerSymbols		0x00800000	// Repairing and need-power symbols draw on this layer
#define kfGobLayerSelection		0x01000000
#define kfGobLayerMask (kfGobLayerSurfaceDecal | kfGobLayerDepthSorted | kfGobLayerSmokeFire | kfGobLayerSymbols | kfGobLayerSelection)

// Layers

#define knLayerSurfaceDecal 0
#define knLayerDepthSorted 1
#define knLayerSmokeFire 2
#define knLayerSymbols 3
#define knLayerSelection 4
#define knLayerEnd 4

#define knLayerMiniMap -1

// When a y coordinate is passed to MakeSortKey as the first value and a Gob
// id (gid) is passed as the second value the resultant key when used for
// sorting will give perfect vertical ordering and stable horizontal ordering.
//
// NOTE: will become spotty when gid values wrap or exceed 1024 (more than
// 256 ids assigned) but this is deemed inconsequential.

#define MakeSortKey(a, b) (((int)(a) << 8) | (((b) >> 2) & 0xff))

class Gob : public StateMachine // gob
{
	friend class StateMachineMgr;
	friend class GobMgr;

public:
	Gob();
	virtual ~Gob();

	dword GetFlags();
	void Flash();
	void MarkRedraw();
	bool Invalidate();
	bool AdvanceAnimation(Animation *pani);
	void StartAnimation(Animation *pani, int nStrip, int nFrame, word wfAni);
	void SetAnimationStrip(Animation *pani, int nStrip);
	void SetAnimationFrame(Animation *pani, int nFrame);
	void SetFlags(dword ff);
	Gid GetId();
	Side GetSide();
	Player *GetOwner();
	void SetOwner(Player *pplr);

	bool IsGobWithinRange(Gob *pgobTarget, TCoord tcRange);
	bool IsTargetWithinRange(WPoint *pwptTarget, Gob *pgobTarget, TCoord tcRange);

	virtual void SetPosition(WCoord wx, WCoord wy);
	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName) = 0;
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool IsSavable();
	virtual GobType GetType() = 0;
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer) = 0;
	virtual dword GetSortKey();
	virtual void GetClippingBounds(Rect *prc);
	virtual void GetUIBounds(WRect *pwrc);
	virtual void GetPosition(WPoint *pwpt);
	virtual void GetTilePosition(TPoint *ptpt);
	virtual void GetTileRect(TRect *ptrc);
	virtual void GetTilePaddedWRect(WRect *pwrc);
	virtual void GetCenter(WPoint *pwpt);
	virtual void PopupMenu();
	virtual void InitMenu(Form *pfrm);
	virtual void OnMenuItemSelected(int id);

	// StateMachine methods


	// Used for tracking visibility

	TRectSmall m_trcBoundingLast;

	// Smooth motion. The position at the start of the latest simulation
	// update is kept so drawing can interpolate between it and the current
	// position. Not saved; kwxInvalid means don't interpolate.

	WCoord m_wxPrev, m_wyPrev;

	void SavePrevPosition() {
		m_wxPrev = m_wx;
		m_wyPrev = m_wy;
	}

	// Temporarily moves the gob to where it should be drawn nAlpha/256 of
	// the way from its previous to its current position. Returns false (and
	// changes nothing) if the gob shouldn't be interpolated.

	bool BeginInterpolatedDraw(int nAlpha) {
		if (m_wxPrev == kwxInvalidPrev)
			return false;
		int dx = m_wx - m_wxPrev;
		int dy = m_wy - m_wyPrev;
		if (dx == 0 && dy == 0)
			return false;
		if (dx > 2 * kwcTile || dx < -2 * kwcTile || dy > 2 * kwcTile || dy < -2 * kwcTile)
			return false;
		m_wxDraw = m_wx;
		m_wyDraw = m_wy;
		m_wx = (WCoord)(m_wxPrev + dx * nAlpha / 256);
		m_wy = (WCoord)(m_wyPrev + dy * nAlpha / 256);
		return true;
	}

	// Where the gob is drawn nAlpha/256 of the way between its previous
	// and current positions (like BeginInterpolatedDraw, without moving it)

	void GetDrawPosition(int nAlpha, WCoord *pwx, WCoord *pwy) {
		*pwx = m_wx;
		*pwy = m_wy;
		if (nAlpha >= 256 || m_wxPrev == kwxInvalidPrev)
			return;
		int dx = m_wx - m_wxPrev;
		int dy = m_wy - m_wyPrev;
		if (dx > 2 * kwcTile || dx < -2 * kwcTile || dy > 2 * kwcTile || dy < -2 * kwcTile)
			return;
		*pwx = (WCoord)(m_wxPrev + dx * nAlpha / 256);
		*pwy = (WCoord)(m_wyPrev + dy * nAlpha / 256);
	}

	void EndInterpolatedDraw() {
		m_wx = m_wxDraw;
		m_wy = m_wyDraw;
	}

protected:
	Gid m_gid;
	dword m_ff;
	WCoord m_wx, m_wy;
	WCoord m_wxDraw, m_wyDraw;
	Player *m_pplr;

private:
	Gob *m_pgobNext;
	Gid m_gidNext;
};

inline dword Gob::GetFlags() {
	return m_ff;
}

inline void Gob::Flash() {
	m_ff |= kfGobFlashing | kfGobRedraw;
	m_unvl.MinSkip();
}

inline void Gob::SetFlags(dword ff) {
	m_ff = ff;
}

inline Gid Gob::GetId() {
	return m_gid;
}

inline Side Gob::GetSide() {
	return m_pplr->GetSide();
}

inline Player *Gob::GetOwner() {
	return m_pplr;
}

inline void Gob::SetOwner(Player *pplr) {
	m_pplr = pplr;
}

Gob *CreateGob(GobType gt);

// GobMgr stuff

const Gid kgidNull = (Gid)-1;
const word kwfGidEndMarker = 0x8000;

// AreaMask; one bit per area, 64 areas max (was 32)

#define kcAreasMax 64
#define knAreaLastDiscovery -1
#if kcAreasMax > 64
#error AreaMask holds 64 areas max
#endif
typedef unsigned long long AreaMask;

struct AreaEntry // are
{
	word iareNext;
	Gid gid;
};

struct Area // ar
{
	TRect trc;
	word iareFree;
	word iareHead;
	word careAlloc;
	word careUsed;
	SideMask sidm;
	UnitMask um;
	AreaEntry *aare;
};

#define knLimitStruct 0
#define knLimitMobileUnit 1
#define knLimitScenery 2
#define knLimitScorch 3
#define knLimitSupport 4

class GobMgr // gobm
{
public:
	GobMgr();
	~GobMgr();

	bool Init(TCoord ctx, TCoord cty, int cpgobMax);
	void Reset();
	bool SaveState(Stream *pstm);
	bool LoadState(Stream *pstm);
	void AddGob(Gob *pgob, Gid gid = kgidNull);
	void RemoveGob(Gob *pgob);
	bool MoveGob(Gob *pgob, WCoord wxOld, WCoord wyOld, WCoord wxNew, WCoord wyNew);
	Gob *GetGob(Gid gid, bool fActiveOnly = true);
	UnitGob *GetUnitGob(TCoord tx, TCoord ty);
	void ShadowGob(Gob *pgob, TCoord tx, TCoord ty, int ctx, int cty);
	void UnshadowGob(Gob *pgob, TCoord tx, TCoord ty, int ctx, int cty);
	Gob *GetShadowGob(TCoord tx, TCoord ty);
	void GetAreaRect(int nArea, TRect *ptrc, Side side = ksideNeutral);
	bool LoadAreas(IniReader *pini);
	Gob *EnumGobsInArea(Enum *penm, int nArea, SideMask sidm, UnitMask um);
	void MoveGobBetweenAreas(Gid gid, AreaMask amOld, AreaMask amNew);
	AreaMask CalcAreaMask(TCoord tx, TCoord ty);
	AreaMask CalcAreaMask(TCoord tx, TCoord ty, int ctx, int cty);
	bool CheckUnitsInArea(int nArea, SideMask sidm, UnitMask um);
	bool IsGobWithinArea(Gob *pgobTarget, int nArea);


	Gob *GetFirstGob() {
		return m_pgobHead;
	}

	// UNDONE: will making this static generate better code?
	class Gob *GetNextGob(Gob *pgob) {
		return pgob->m_pgobNext;
	}

	// First gid at TCoord

	Gid GetFirstGid(TCoord tx, TCoord ty) {
		Gid gid = m_pgidMap[ty * m_ctx + tx];
		if (gid & kwfGidEndMarker)
			return kgidNull;
		return gid;
	}

	// Next gid in list

	Gid GetNextGid(Gid gid) {
		Gob *pgob = GetGob(gid, false);
		if (pgob == NULL)
			return kgidNull;
		return pgob->m_gidNext;
	}

	void GetMapSize(TCoord *pctx, TCoord *pcty) {
		*pctx = m_ctx;
		*pcty = m_cty;
	}

	int FindGobs(const Rect *prcBounds, Gob **apgob, int cpgobMax, byte *pbFogMap);
	bool IsStructurePlacementValid(StructConsts *pstruc, TCoord tx, TCoord ty, Player *pplr);
	int FindGobs(const TRect *ptrcBounds, Gob **apgob, int cpgobMax);
	void TrackGobCounts(Gob *pgob, bool fIncrement);
	bool IsBelowLimit(int nLimit, Player *pplr = NULL);

private:
	void AddGobToAreas(Gid gid, AreaMask am);
	void RemoveGobFromAreas(Gid gid, AreaMask am);
	int GetAreasFromMask(AreaMask am, int *pnArea);
	void FreeAreas();

	Gob *m_pgobHead;
	Gid *m_pgidMap;
	int m_cpgobMax;
	int m_cpgobActive;		// UNDONE: _DEBUG only?
	Gob **m_ppgobFreeHead;
	Gob **m_ppgobFreeTail;
	TCoord m_ctx, m_cty;
	int m_car;
	Area m_aar[kcAreasMax];
	int m_cSceneryGobs;
	int m_cScorchGobs;
	int m_cSupportGobs;

	Gob **m_apgobMaster;
};
extern GobMgr ggobm;

// GobStateMachineMgr

class GobStateMachineMgr : public StateMachineMgr
{
public:
	virtual StateMachineId GetId(StateMachine *psm) {
		return (StateMachineId)((Gob *)psm)->GetId();
	};
	virtual StateMachine *GetStateMachine(StateMachineId smid) {
		return (StateMachine *)ggobm.GetGob((Gid)smid, false);
	};
};
extern GobStateMachineMgr gsmm;

// Gob derivatives

class OvermindGob : public Gob // ovr
{
public:
	OvermindGob();
	virtual ~OvermindGob();
	static bool InitClass(IniReader *pini);
	bool Init(const char *pszName = NULL);
	void Toggle();

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool IsSavable();
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	bool m_fEnabled;
	int m_cAttackCountdown;
	int m_cBuildUnitCountdown;
	bool m_fInitializationComplete;
};

class SurfaceDecalGob : public Gob // dcl
{
public:
	SurfaceDecalGob();
	virtual ~SurfaceDecalGob();
	static bool InitClass(IniReader *pini);
	bool Init(WCoord wx, WCoord wy, dword ff, const char *pszBitmap, TBitmap *ptbm = NULL, const char *pszName = NULL);

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool IsSavable();
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);

protected:
	TBitmap *m_ptbm;
};

// ScorchGob different than SurfaceDecalGob so that m_nScorch can be persisted

class ScorchGob : public SurfaceDecalGob
{
public:
	ScorchGob();
	bool Init(WCoord wx, WCoord wy, int nScorch);

	// Scorch methods

	int GetSequence();

	// Gob methods

	virtual GobType GetType();
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool IsSavable();

private:
	int m_nScorch;
	int m_nSequence;
};

class SceneryGob : public Gob // scn
{
public:
	SceneryGob();
	virtual ~SceneryGob();
	static bool InitClass(IniReader *pini);
	bool Init(WCoord wx, WCoord wy, dword ff, const char *pszBitmap, const char *pszName = NULL);

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual GobType GetType();
	virtual dword GetSortKey();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);

private:
	TBitmap *m_ptbm;
};

class AnimGob : public Gob // anm
{
public:
	AnimGob();
	virtual ~AnimGob();
	bool Init(WCoord wx, WCoord wy, word wfAnm, const char *pszAniName, AnimationData *panid = NULL,
			int nStrip = 0, StateMachineId smidNotify = ksmidNull, const char *pszName = NULL);
	virtual bool OnStripDone();

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool IsSavable();
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

protected:
	StateMachineId m_smidNotify;
	word m_wfAnm;
	Animation m_ani;
};
AnimGob *CreateAnimGob(WCoord wx, WCoord wy, word wfAnm, const char *pszAniName, AnimationData *panid,
		int nStrip = 0, StateMachineId smidNotify = ksmidNull, const char *pszName = NULL);

const word kfAnmDeleteWhenDone = 0x0001;
const word kfAnmLoop = 0x0002;
const word kfAnmSmokeFireLayer = 0x0004;
const word kfAnmSurfaceDecalLayer = 0x0008;

const int kcbStructUnitName = 50;

struct GobConsts { // gobc
	GobType gt;
};

#define GetUnitCostMin() (gnUnitCostMin)
#define GetUnitCostMax() (gnUnitCostMax)

#define kfUntcNotifyEnemyNearby	0x0001	// wants to be notified of nearby mobile enemies
#define kfUntcNotifyPowerLowHigh 0x0002 // bld is power sensitive, needs notifications
#define kfUntcLargeDefog 0x0004 // this unit defogs a larger area
#define kfUntcMobileUnitBuilder 0x0008 // builds mobile units
#define kfUntcStructureBuilder 0x0010 // builds structures
#define kfUntcHasFullnessIndicator 0x0020 // draws fullness indicator

struct UnitConsts : public GobConsts { // untc
	UnitType ut;
	UnitMask um;
	UnitMask umPrerequisites;
	UpgradeMask upgmPrerequisites;
	Sfx sfxImpact;
	int nCost;
	fix fxArmorStrength;
	int nInfantryDamage;
	int nVehicleDamage;
	int nStructureDamage;
	int ctFiringRate;
	int tcFiringRange;	// kept as int instead of TCoord to accommodate Ini::GetPropertyValue
	int cupdTimeToBuild;
	char szAniName[kcbFilename];
	AnimationData *panid;
	WRect wrcUIBounds;
	int wdySortOffset;
	char szName[kcbStructUnitName];
	char szLongName[kcbStructUnitName];
	UnitMenu *pfrmMenu;
	word wf;
	char *szDescription;

	int GetCost() {
		return nCost;
	}
	fix GetArmorStrength() {
		return fxArmorStrength;
	}
	int GetTimeToBuild() {
		return cupdTimeToBuild;
	}
	int GetInfantryDamage() {
		return nInfantryDamage;
	}
	int GetVehicleDamage() {
		return nVehicleDamage;
	}
	int GetStructureDamage() {
		return nStructureDamage;
	}
};

struct StructConsts : public UnitConsts { // struc
	int ctx, cty;	// kept as int instead of TCoord to accommodate Ini::GetPropertyValue
	int ctxReserve, ctyReserve;	// kept as int instead of TCoord to accommodate Ini::GetPropertyValue
	int nPowerDemand;
	int nPowerSupply;
	int nUpgradeCost;
	Sfx sfxAbortRepair;
	Sfx sfxRepair;
	Sfx sfxDamaged;
	Sfx sfxDestroyed;
	Sfx sfxSelect;
	Sfx sfxUpgrade;
	Sfx sfxAbortUpgrade;
};

struct BuilderConsts : public StructConsts { // bldc
	int nBuildRate;
	Sfx sfxUnitBuild;
	Sfx sfxUnitBuildAbort;
	Sfx sfxUnitReady;
	UnitMask umCanBuild;
};

struct TowerConsts : public StructConsts { // twrc
	Sfx sfxAttack;
	Sfx sfxFire;
	int *anFiringStripIndices;
};

struct MobileUnitConsts : public UnitConsts { // muntc
	Sfx sfxFire;
	SfxCategory sfxcDestroyed;
	SfxCategory sfxcSelect;
	SfxCategory sfxcMove;
	SfxCategory sfxcAttack;
	WCoord wcMoveDistPerUpdate;
	int *anFiringStripIndices;
	int *anMovingStripIndices;
	int *anIdleStripIndices;

	WCoord GetMoveDistPerUpdate() {
		return wcMoveDistPerUpdate;
	}
};

class MobileUnitBuildForm;
struct MobileUnitBuilderConsts : public BuilderConsts { // mubc
	word fUpgrade;
	word fUpgradeInProgress;
	MobileUnitBuildForm *pfrmBuild;
};

struct SpConsts : public MobileUnitConsts { // spc
	Sfx sfxStructureCaptured;
};

struct MinerConsts : public MobileUnitConsts { // mnrc
	Sfx sfxMine;
	Sfx sfxUnderAttack;
};

const int kcmsDamageIndicatorTimeout = 4000; // 4.0 secs TUNE:

#define kcEnemyNearbyRemember 5 // # of enemies to remember

#define kcyHealthBar 4 // Height of health bar above selection

const WCoord kwcFullnessPip = WcFromTile16ths(2);

#define kcyFullnessBar (PcFromWc(kwcFullnessPip) + 2) // Height of fullness bar below miner, processor, warehouse

class UnitGob : public Gob // unt
{
public:
	UnitGob(UnitConsts *puntc);

	static bool InitClass(UnitConsts *psubc, IniReader *pini);
	static void ExitClass(UnitConsts *psubc);
	static bool LoadMenu(UnitConsts *psubc, IniReader *pini, char *pszTemplate, int idfDefault);
	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void Delete();
	virtual void DefUpdate();
	virtual void Activate();
	virtual void Deactivate();
	virtual bool IsIdle();
	virtual bool IsValidTarget(Gob *pgobTarget);
	virtual void SetTarget(Gid gid, WCoord wxTarget = 0, WCoord wyTarget = 0, WCoord wxCenter = 0, WCoord wyCenter = 0, TCoord tcRadius = 0, WCoord wcMoveDistPerUpdate = 0);
	virtual void GetAttackPoint(WPoint *pwpt);
    virtual dword GetAnimationHash();
    virtual void GetAnimationBounds(Rect *prc, bool fBase);
    virtual void DrawAnimation(DibBitmap *pbm, int x, int y);
	void NotifyNearbyAlliesOfHit(Gid gidAssailant);
	void ShowDamageIndicator();
	void ClearDamageIndicator();
	int GetDamageTo(UnitGob *puntTarget);
	int ScoreTarget(UnitGob *puntTarget, TCoord tc);
	void RecalcEnemyNearby(bool fClearOnly);

	UnitConsts *GetConsts() {
		return m_puntc;
	}

	UnitType GetUnitType() {
		return m_puntc->ut;
	}

	word GetUnitFlags() {
		return m_wfUnit;
	}

	void SetUnitFlags(word wf) {
		m_wfUnit = wf;
	}

	bool IsAlly(Side side);

	fix GetHealth() {
		return m_fxHealth;
	}

	void Select(bool fSelect);

	virtual void SetHealth(fix fxHealth);

	// Gob overrides

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual dword GetSortKey();
	virtual void GetClippingBounds(Rect *prc);
	virtual void GetUIBounds(WRect *pwrc);
	virtual void GetCenter(WPoint *pwpt);
	virtual void PopupMenu();

	// The friendly unit this unit was told to attack with Ctrl+right-click
	// (kgidNull if none); it counts as a valid target

	Gid m_gidForceTarget;

protected:
	void RememberEnemyNearby(Gid gidEnemy);
	TCoord CalcRange(TCoord tx, TCoord ty, Gob *pgob);
	virtual UnitGob *FindEnemyNearby(TCoord tc);
	void NotifyEnemyNearby();

	union {
		UnitConsts *m_puntc;
		StructConsts *m_pstruc;
		TowerConsts *m_ptwrc;
		SpConsts *m_pspc;
		MinerConsts *m_pmnrc;
		MobileUnitConsts *m_pmuntc;
		BuilderConsts *m_pbldrc;
		MobileUnitBuilderConsts *m_pmubc;
	};
	Gid m_agidEnemyNearby[kcEnemyNearbyRemember];
	fix m_fxHealth;
	Animation m_ani;
	word m_wfUnit;
	int m_cupdLastHitNotify;
	short m_cDamageCountdown;
};

const word kfUnitInvulnerable = 0x0001;
const word kfUnitDrawHealthIndicator = 0x0002;
const word kfUnitDrawRepairingSymbol = 0x0004;
const word kfUnitRepairing = 0x0008;
const word kfUnitDrawNeedsPowerSymbol = 0x0010;
const word kfUnitDrawNeedCreditsSymbol = 0x0020;
const word kfUnitNeedCredits = 0x0040;

class StructGob : public UnitGob // stru
{
public:
	StructGob(StructConsts *pstruc);
	virtual ~StructGob();
	static bool InitClass(StructConsts *pstruc, IniReader *pini);
	static void ExitClass(StructConsts *pstruc);

	void Repair(bool fOn);
	bool NeedsRepair();
	void EnableOrDisableSymbolLayer();
	virtual bool IsTakeoverable(Player *pplr);
	virtual void Takeover(Player *pplr);

	static bool LoadConfirmation();
	bool PopupConfirmation(char *pszTitle);
	void TakedownConfirmation();
	void SelfDestruct(bool fRecycleValue = true);
	bool IsAccessible(TCoord tx, TCoord ty);

	// UnitGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void Delete();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void DefUpdate();
	virtual void Activate();
	virtual void Deactivate();
	virtual bool IsValidTarget(Gob *pgobTarget);
	virtual void SetHealth(fix fxHealth);
	virtual void GetAttackPoint(WPoint *pwpt);

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual void GetTileRect(TRect *ptrc);
	virtual void GetTilePaddedWRect(WRect *pwrc);
	virtual void InitMenu(Form *pfrm);
	virtual void OnMenuItemSelected(int id);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

	// Used to track visibility in FindGobs

	word m_nSeqLastVisible;

protected:
	void OnPowerLowHigh();
	void DrawSymbol(TBitmap *ptbm, DibBitmap *pbm, int xViewOrigin, int yViewOrigin);
	virtual void WaitingForCredits(bool fNeed, bool fOverride = false, Player *pplr = NULL);

private:
	ScorchGob *GetScorchGob();

	long m_tLastSmoke;

	static TBitmap *s_ptbmRepairing;
	static TBitmap *s_ptbmNeedsPower;
	static TBitmap *s_ptbmNeedCredits;
};

// TUNE:
const fix kfxHealthUnitsRepairedPerUpdate = itofx(1);
const int knCreditsPerRepairUpdate = 1;
const long kcupdSymbolFlashRate = 8; // ideally a power of 2

#define GetReplicationCost(nCost) ((nCost) / 3)

class UnitMenu : public Form, public Timer
{
public:
	void SetOwner(UnitGob *punt, bool fPerUnitInit = true);
	UnitGob *GetOwner();

	// Form overrides

	virtual bool DoModal(int *pnResult = NULL, Sfx sfxShow = ksfxGuiFormShow, Sfx sfxHide = ksfxGuiFormHide);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnPaint(DibBitmap *pbm);
	virtual bool OnHitTest(Event *pevt);
	virtual bool OnPenEvent(Event *pevt);
	virtual bool OnRightDown(Event *pevt);

	// Timer overrides

	virtual void OnTimer(long tCurrent);

protected:
	UnitGob *m_punt;
	Rect m_rcButtons;
};

enum MobileUnitAction // mua
{
	kmuaNone,
	kmuaGuard,
	kmuaGuardVicinity,
	kmuaGuardArea,
	kmuaMove,
	kmuaAttack,
	kmuaHuntEnemies,
	kmuaGuardUnit,		// follow and protect a friendly unit (m_msgAction.AttackCommand.gidTarget)
};

struct MoveDirections // movd
{
	MobileUnitGob *pmunt;
	byte idir;
	byte cdir;
	Direction adir[8];
};

const word kfMuntChaseEnemies = 0x0001;			// Will chase enemies when attacking them
const word kfMuntReturnFire = 0x0002;			// Will attack anything that hits it
const word kfMuntAttackEnemiesWhenMoving = 0x0004; // Will attack enemies in range when moving
const word kfMuntAttackEnemiesWhenGuarding = 0x0008; // Will attack enemies in range when guarding
const word kfMuntRunAwayWhenHit = 0x0010;		// Will run away when hit
const word kfMuntMoveWait = 0x0020;				// Is waiting for a tile to free up
const word kfMuntPathPending = 0x0040;			// Will be calcing a path soon
const word kfMuntCommandPending = 0x0080;		// Has a command pending
const word kfMuntFiring	= 0x0100;				// Is shooting at something
const word kfMuntMoveWaitingNearby = 0x0200;	// Gob nearby is "move waiting" on this gob
const word kfMuntDestinationReserved = 0x0400;	// Dest tile reserved
const word kfMuntStuck = 0x0800;				// Stuck bit for debugging
const word kfMuntAtReplicatorInput = 0x1000;	// At replicator input spot
const word kfMuntStayPut = 0x2000;				// Don't move to attack enemies

// All the flags that control a MobileUnit's aggressiveness

const word kfMuntAggressivenessBits = kfMuntChaseEnemies | kfMuntReturnFire |
		kfMuntAttackEnemiesWhenMoving | kfMuntAttackEnemiesWhenGuarding |
		kfMuntRunAwayWhenHit | kfMuntStayPut;

// Most MobileUnits move (follow paths) and like to shoot

extern bool gfGodMode;
#define kcPathsCache 5

class MobileUnitGob : public UnitGob // munt
{
public:
	MobileUnitGob(MobileUnitConsts *pmuntc);
	virtual ~MobileUnitGob();
	// NOTE: derivatives should initialize the an*StripIndices arrays of the MobileUnitConsts
	static bool InitClass(MobileUnitConsts *pmuntc, IniReader *pini);
	static void ExitClass(MobileUnitConsts *pmuntc);
	static MobileUnitGob *AnyTransitionsIntoTile(TCoord tx, TCoord ty, Gob *pgobIgnore);
	static void FreeCachedPaths();
	bool IsReadyForCommand();
	bool IsMobile();
	bool IsStandingOnActivator();

	bool HasAttackTarget() {
		return m_gidTarget != kgidNull;
	}

	word GetMobileUnitFlags() {
		return m_wfMunt;
	}

	void SetMobileUnitFlags(word wf) {
		m_wfMunt = wf;
	}

	void ClearAction() {
		m_mua = kmuaNone;
	}

	bool IsHumanOrGodControlled() {
		return !(m_pplr->GetFlags() & kfPlrComputer) || gfGodMode;
	}

	bool IsMoveWaiting() {
		return (m_wfMunt & kfMuntMoveWait) != 0;
	}

	// UnitGob overrides

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
#ifdef DRAW_PATHS
	void DrawPath(DibBitmap *pbm, WCoord wxViewOrigin, WCoord wyViewOrigin);
#endif
#ifdef DRAW_LINES
	void DrawTargetLine(DibBitmap *pbm, int xViewOrigin, int yViewOrigin);
#endif
	virtual bool IsIdle();
	virtual void SetTarget(Gid gid, WCoord wxTarget = 0, WCoord wyTarget = 0, WCoord wxCenter = 0, WCoord wyCenter = 0, TCoord tcRadius = 0, WCoord wcMoveDistPerUpdate = 0);
	virtual void Activate();
	virtual void Deactivate();
	virtual void GetAttackPoint(WPoint *pwpt);

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual void SetPosition(WCoord wx, WCoord wy);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

protected:
	void MoveEnter(bool fReset = false);
	void MoveExit();
	int MoveUpdate();

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();
	virtual void PerformAction(char *szAction);
	bool InTransition();
	void EndAttackMove();
	UnitGob *FindEnemyWhenMoving(TCoord tcRange);
	UnitGob *FindEnemyStructureNearby(TCoord tcRange, Gob *pgobFrom = NULL);
	void IssueOrder(Message *pmsg);
	void StartQueuedOrder();
	UnitGob *GetGuardedUnit();
	bool GuardUnitUpdate();
	UnitGob *FindGuardTarget();

public:
	void GuardUnit(Gid gidTarget, bool fPlayerOrder = true);	// guard order (Ctrl+Alt+right-click)
	Gid GetGuardedGid() {
		return m_mua == kmuaGuardUnit ? m_msgAction.AttackCommand.gidTarget : kgidNull;
	}

protected:

private:
	bool IsAttackPointWithinFiringRangeOfTarget(UnitGob *puntTarget);
	bool PendOrProcessCommand(Message *pmsg, State stNew);
	bool PendOrProcessAction(Message *pmsg, State stNew, MobileUnitAction mua);
	bool IsTargetInRange();
	void SetStatePendingFireComplete(State st);
	void ContinueActionPendingFireComplete();
	void ReserveTile(TCoord tx, TCoord ty, bool fReserve);
	bool IsTileReserved(TCoord tx, TCoord ty);
	byte GetTileFlags(TCoord tx, TCoord ty);
	MobileUnitGob *GetReservingUnit(TCoord tx, TCoord ty);
	int GetNextLocations(TPoint *atpt);
	int GetNextLocations2(Path *ppath, int *pitptStart, int cStepsFurtherStop, TPoint *atpt);
	int FindClosestPoint(TPoint *ptptClosest, int ctptFurtherStop);
	bool PrepPath(WCoord wxDst, WCoord wyDst, bool *pfPending = NULL);
	Path *FindPath(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo);
	Path *FindGroupPath(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo);
	void AddGroupPath(TCoord txFrom, TCoord tyFrom, Path *ppath);
	int TileDistToTarget(TCoord tx, TCoord ty);
	bool GiveUpBlocked(int *pnMoveResult);
	bool PrepareNextTransition(TPoint *ptptNext, int *pnMoveResult);
	void EnterTransition(TPoint *ptptNext);
	void ContinueTransition();
	bool HandleCollision(MobileUnitGob **apmunt, byte *abf, TPoint *atpt, int ctpt);
	bool PlanMoveAsidePath(MobileUnitGob *pmunt, TCoord tx, TCoord ty);
	bool GetMoveAsideDirections(Direction dirMover, MoveDirections *pmovd);
	bool AcceptMoveAsideRequest(Direction dir);
	MobileUnitGob *GetMobileUnitAt(TCoord tx, TCoord ty);
	bool CheckDestinationReached();
	bool IsMoveWaitCycle(MobileUnitGob *pmunt);
	bool MoveWaitForUnit(MobileUnitGob *pmunt, TCoord tx, TCoord ty);
	bool ContinueMoveWaiting(MobileUnitGob **apmunt, byte *abf, TPoint *atpt, int ctpt, TPoint *ptptNext);
	void NotifyMoveWaitingNearby(WCoord wx, WCoord wy);
	bool FindLocalAvoidPath(TCoord tx, TCoord ty);
	int FindCloserTrackPoint(TrackPoint *ptrkpStart, Direction dirBlocked, bool fClockwise, int ctpt, TPoint *atpt, TrackPoint *ptrkp);
	bool CheckReplicatorPoint();
	UnitGob *FindValidTargetInArea(int nArea);

protected:
	Direction m_dir, m_dirNext;
	Gid m_gidTarget;
	long m_tLastFire;
	Message m_msgPending;
#define kcmsgQueuedMax 16
	Message m_amsgQueued[kcmsgQueuedMax];	// Shift+right-click orders waiting their turn
	int m_cmsgQueued;
	Message m_msgAction;
	int m_cCountdown;		// multi-purpose countdown timer
	int m_cMoveStepsRemaining;
	TPoint m_tptChaseInitial;
	MobileUnitAction m_mua, m_muaPending;
	word m_wfMunt;
	State m_stPending;
	int m_cupdLastHitOrNearbyAllyHit;

#ifdef DRAW_PATHS
public:
	WCoord m_wxDst, m_wyDst;
protected:
#endif

	TCoord m_txDst, m_tyDst;

#ifdef DRAW_LINES
public:
#endif
	WPoint m_wptTarget;

#ifdef DRAW_PATHS
public:
#endif
	bool m_fMovingAside;		// moving aside for another unit; resume m_msgAction after
	word m_nSeqMoveAside;
	Path *m_ppathUnit;
	Path *m_ppathAvoid;
	int m_itptPath;
	WPoint m_wptTargetCenter;
	TCoord m_tcTargetRadius;
	WCoord m_wcMoveDistPerUpdate;

	// Blocked movement (not saved): tries since the last progress toward
	// the target, transitions that made none, the best distance so far
	byte m_cBlocked;
	byte m_cNoProgress;
	bool m_fRepathedAroundUnits;
	int m_nDistBest;
	int m_itptBest;

	// A chase that got stuck (not saved): don't chase that target again
	// for a while
	Gid m_gidChaseFailed;
	int m_cupdChaseFailed;

	static Path *s_apathCached[kcPathsCache];
	static int s_cpathCached;

};

// Values returned by UnitGob::MoveUpdate()

const int knMoveMoving = 0;
const int knMoveTargetReached = 1;
const int knMoveStuck = 2;
const int knMoveWaiting = 3;

const int kxInvalid = -1000;
const WCoord kwxInvalid = -1000;
const TCoord ktxInvalid = -100;
const word kcBuildQueueMax = 50;	// was 10

class BuildQueue
{
public:
	BuildQueue();
	BuildQueue &operator=( BuildQueue &bqRHS);
	void SetSize(word cutMax);
	void Enqueue(UnitType ut);
	void Dequeue();
	bool RemoveUnit(UnitType ut);
	UnitType Peek();
	bool IsEmpty();
	bool IsFull();
	void Clear();
	int GetUnitCount(UnitType ut);

	// called by builder gob
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);

private:
	char m_achBuildQueue[kcBuildQueueMax];
	word m_cchQueueMax;
};

class BuilderGob : public StructGob // bldr
{
public:
	BuilderGob(BuilderConsts *pbldrc);
	virtual ~BuilderGob();
	static bool InitClass(BuilderConsts *pbldrc, IniReader *pini);
	static void ExitClass(BuilderConsts *pbldrc);
	void DrawUpgradeEffect(DibBitmap *pbm, int xViewOrigin, int yViewOrigin);
	void AbortBuild(bool fRefundCreditsSpent = false, UnitType ut = kutNone);
	static int GetGlobalQueuedCount(Player *pplr, word wfTest);

	// in builder gob so it will be tracked per-instance

	void SetLastSelection(short iSel) { m_iLastSelection = iSel;}
	short GetLastSelection() {return m_iLastSelection;}

	// Build management methods

	virtual void FindInitPosition(WPoint *pwpt);
	virtual void Build(UnitType ut, Gid gid = kgidNull);
	bool IsBuildInProgress() {
		return !m_bq.IsEmpty();
	}
	UnitType UnitBuildInProgress() {return m_bq.Peek();}
	void DrawBuildProgress(DibBitmap *pbm, Rect *prc);
	void SyncBuildQueue(BuildQueue *pbq);

	UnitGob *GetBuiltGob();
	void ClearBuiltGob();

	// UnitGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void DefUpdate();
	virtual void Deactivate();

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
    virtual void GetClippingBounds(Rect *prc);

protected:
    bool ShowingBuildProgress();

	short m_iLastSelection;
	Gid m_gidBuildVisible;
	fix m_fxHealthBuilding;
	int m_nCreditsSpentOnBuilding;
 	int m_nCostRemainder;
	TPoint m_tptRally;

	BuildQueue m_bq;
};

class MobileUnitBuildForm : public Form
{
public:
	BuilderGob *GetOwner() {
		return m_pbldr;
	}

	void SetOwner(BuilderGob *pbldr);
	void UpdateUnitInfo(ListItem *pli);
	void OnUnitCompleted(BuilderGob *pbldr, UnitType ut);
	void DefUpdate(BuilderGob *pbldr, bool fBuildInProgress);
	int CountQueue(UnitType ut);
	void UpdateOrderButton(bool fCalcLimit);

	// Form overrides

	void EndForm(int nResult = 0);
	virtual void OnControlSelected(word idc);
	virtual void OnControlNotify(word idc, int nNotify);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual bool OnRightDown(Event *pevt);
	virtual bool OnPenEvent(Event *pevt);
	virtual bool OnKeyTest(Event *pevt);

private:
	int m_cupdLast;
	int m_iliHover;		// item whose info shows because the mouse is over it, or -1
	bool m_fLimitReached;
	bool m_fOrderValid;
	BuilderGob *m_pbldr;
	BuildQueue m_bqPrivate;

	static int s_iliCurrent;
};

class TankGob : public MobileUnitGob // tnk
{
public:
	TankGob(MobileUnitConsts *pmuntc);
	static bool InitClass(MobileUnitConsts *pmuntc, IniReader *pini);
	static void ExitClass(MobileUnitConsts *pmuntc);
	virtual void LaunchProjectile(WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget);

	// UnitGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void DefUpdate();

	// UnitGob overrides

	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();
    virtual dword GetAnimationHash();
    virtual void GetAnimationBounds(Rect *prc, bool fBase);
    virtual void DrawAnimation(DibBitmap *pbm, int x, int y);

	// Gob overrides

	virtual void GetClippingBounds(Rect *prc);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);

protected:
	Animation m_aniTurret;

    // Note: If this is a char, iphone/llvm will encounter an alignment bug
    // between this member and MTankGob::m_pgobSecondShot. Direction16
    // has been changed to an int for this reason.

	Direction16 m_dir16Turret;
};

class SRInfantryGob : public MobileUnitGob // sri
{
public:
	SRInfantryGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// UnitGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class LRInfantryGob : public MobileUnitGob // lri
{
public:
	LRInfantryGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// UnitGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class SpInfantryGob : public MobileUnitGob // lri
{
public:
	SpInfantryGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// UnitGob overrides

	virtual bool IsValidTarget(Gob *pgobTarget);
	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class LTankGob : public TankGob // ltnk
{
public:
	LTankGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
};

class TankShotGob; // it's down there

class MTankGob : public TankGob // mtnk
{
public:
	MTankGob();
	~MTankGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	void LaunchProjectile(WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget);

	// UnitGob overrides

	virtual void DefUpdate();
	virtual void Deactivate();

private:
	TankShotGob *m_pgobSecondShot;
	short m_cShotcountdown;
};

class RTankGob : public TankGob // rtnk
{
public:
	RTankGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	void LaunchProjectile(WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget);

};

class GTankGob : public TankGob // gtnk
{
public:
	GTankGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// MobileUnitGob overrides

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

	// TankGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);

};

class MinerGob : public MobileUnitGob // mnr
{
public:
	MinerGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	int GetGalaxiteAmount();
	void SetGalaxiteAmount(int nAmount);
	void SetFavoriteProcessor(Gid gid);
	void Hide(bool fHide);
	void SendDeliverCommand(Gid gidProcessor);
	void DeliverTo(Gid gidProcessor);

	bool IsAttemptingToDeliver(Gid gid) {
		if (m_fAttemptingToDeliver && gid == m_gidFavoriteProcessor)
			return true;
		return false;
	}

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual void GetClippingBounds(Rect *prc);

	// UnitGob overrides

	virtual bool IsValidTarget(Gob *pgobTarget);
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void Deactivate();
	virtual void SetTarget(Gid gid, WCoord wxTarget = 0, WCoord wyTarget = 0, WCoord wxCenter = 0, WCoord wyCenter = 0, TCoord tcRadius = 0, WCoord wcMoveDistPerUpdate = 0);


	// MobileUnitGob overrides

	void PerformAction(char *szAction);
	virtual void Idle();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	void Mine(WCoord wx, WCoord wy);
	Gid FindClosestProcessor();

private:
	Animation m_aniVacuum;
	int m_nGalaxiteAmount;
	char m_cDelay;
	Gid m_gidFavoriteProcessor;
	TPoint m_tptGalaxite;
	bool m_fMinerUnderAttack;
	static AnimationData *s_panidVacuum;
	bool m_fAttemptingToDeliver;
	bool m_fHidden;
	bool m_fKeepMining;		// mining (or delivering) until told otherwise
	bool m_fQuietRetry;		// retrying: don't announce "needs Galaxite" again
};

inline void MinerGob::SetGalaxiteAmount(int nAmount)
{
	m_nGalaxiteAmount = nAmount;
}

inline int MinerGob::GetGalaxiteAmount()
{
	return m_nGalaxiteAmount;
}

inline void MinerGob::SetFavoriteProcessor(Gid gid)
{
	m_gidFavoriteProcessor = gid;
}

// TUNE:

const int knMinerGalaxiteMax = 100;	// two full patches of Galaxite
const int knGalaxiteValue = 10;
const int kcMinerSuckDelay = 7; // number of updates between Galaxite decrements

class MobileHqGob : public MobileUnitGob // mhq
{
public:
	MobileHqGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	bool CanTransform(TPoint *ptp = NULL);

	// Deploy: drive to where the Headquarters goes (its top-left tile) and
	// transform there
	static bool CanDeployAt(TCoord tx, TCoord ty, Gob *pgobIgnore);
	void OrderDeploy(TCoord txHq, TCoord tyHq);

	// UnitGob overrides

	virtual void SetTarget(Gid gid, WCoord wxTarget = 0, WCoord wyTarget = 0, WCoord wxCenter = 0, WCoord wyCenter = 0, TCoord tcRadius = 0, WCoord wcMoveDistPerUpdate = 0);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	int m_nDeploy;			// 0 none, 1 ordered, 2 driving there
	TCoord m_txDeploy, m_tyDeploy;
};

class MobileUnitBuilderGob : public BuilderGob // mub
{
public:
	MobileUnitBuilderGob(MobileUnitBuilderConsts *pmubc);

	// Gob overrides

	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void InitMenu(Form *pfrm);
	virtual void OnMenuItemSelected(int id);
	virtual void DefUpdate();

	// UnitGob overrides

	virtual void Deactivate();

	// StructGob overrides

	virtual void Takeover(Player *pplr);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class HrcGob : public MobileUnitBuilderGob // hrc
{
public:
	HrcGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// StateMachine methods

private:
	static MobileUnitBuildForm *s_pfrmBuild;
};

class ReactorGob : public StructGob // rctr
{
public:
	ReactorGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// StateMachine methods

};

class ProcessorGob : public StructGob // prcr
{
public:
	ProcessorGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

    // UnitGob overrides

    virtual dword GetAnimationHash();
    virtual void GetAnimationBounds(Rect *prc, bool fBase);
    virtual void DrawAnimation(DibBitmap *pbm, int x, int y);

	// StructGob overrides

	virtual bool IsTakeoverable(Player *pplr);
	virtual void Takeover(Player *pplr);

	// Gob overrides

	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual void GetClippingBounds(Rect *prc);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	void NotifyMinersAttemptingDelivery(bool fDying);

	Animation m_aniOverlay;
	Gid m_gidMiner;
	WPoint m_wptFakeMiner;
	bool m_fProcessingAnimationInProgress;
	bool m_fDoorMoving;
};

class StructureBuildForm;

class HqGob : public BuilderGob // hq
{
public:
	HqGob();
	virtual ~HqGob();

	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// Gob overrides

	virtual void InitMenu(Form *pfrm);
	virtual void OnMenuItemSelected(int id);

	// UnitGob overrides

	virtual void Deactivate();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
private:

	void RemoveScorch(StructGob *pstru);

	static StructureBuildForm *s_pfrmBuild;
};

class PlaceStructureForm : public Form, Timer
{
public:
	PlaceStructureForm();

	void SetOwner(BuilderGob *pgobOwner, UnitType ut);
	BuilderGob *GetOwner() {
		return m_pgobOwner;
	}

	// Deploy mode: where a Dominion should drive and become a Headquarters
	void SetDeployer(MobileHqGob *pmhq);
	void UpdatePlacementIndicator(WCoord wxViewStart, WCoord wyViewStart, WCoord wxView, WCoord wyView);

    bool IsBusy() { return m_pgobOwner != NULL || m_gidDeploy != kgidNull; }
	void OnPaintSimUI(DibBitmap *pbm);
	void OnPaintControlsSimUI(DibBitmap *pbm, UpdateMap *pupd);
    void UpdatePosition(Event *pevt);

	// Timer implementation

	virtual void OnTimer(long tCurrent);

	// Form overrides

	virtual void OnControlSelected(word idc);
	virtual bool OnPenEvent(Event *pevt);
	virtual void OnScroll(int dx, int dy);

private:
    void GetSubRects(WCoord wx, WCoord wy, Rect *prcInside, Rect *prcOutside);
    bool IsTileGood(TCoord tx, TCoord ty, bool fPlacementValid);
    bool IsPlacementValid();
    void StartPlacement();

	BuilderGob *m_pgobOwner;
	Gid m_gidDeploy;		// the Dominion in deploy mode
	Player *m_pplr;
	StructConsts *m_pstruc;
	TCoord m_tx, m_ty;
	TCoord m_wxPen, m_wyPen;
	int m_wxDragStart, m_wyDragStart;
	TCoord m_txStart, m_tyStart;
	bool m_fDragging;
	bool m_fHaveMouse;		// the placement follows the mouse
	bool m_fValid;			// the building fits where it is
	int m_xMouse, m_yMouse;	// last mouse position on the playfield
	void FollowMouse(int x, int y);
};


class RadarGob : public StructGob // rdr
{
public:
	RadarGob();

	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// Struct gob methods

	virtual void Activate();
	virtual void Deactivate();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class ResearchGob : public StructGob // rsrch
{
public:
	ResearchGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	virtual void UpgradeUpdate();
	virtual void AbortUpgrade();

	// Gob overrides - these need to become virtual if this gets a derived class!

	virtual void InitMenu(Form *pfrm);
	virtual void OnMenuItemSelected(int id);
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void Deactivate();
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	int m_nCreditsSpentOnUpgrade;
	UnitType m_utTarget;
};

class VtsGob : public MobileUnitBuilderGob // vts
{
public:
	VtsGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

private:
	static MobileUnitBuildForm *s_pfrmBuild;
};

// TUNE: tankshot movement rate

const WCoord kwcTankShotRate = kwcTile / 2;

class TankShotGob : public Gob
{
public:
	TankShotGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	bool Init(AnimationData *panid, WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget, WCoord wcMoveRate);
	void Launch();

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);
	virtual bool IsSavable();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	Gid m_gidOwner;
	Gid m_gidTarget;
	WLineIterator m_li;
	Animation m_ani;
	int m_nDamage;

public:
	static AnimationData *s_panidShot;
};
TankShotGob *CreateTankShotGob(WCoord wx, WCoord wy, WCoord wxTarget, WCoord yTarget, int nDamage, Gid gidOwner, Gid gidTarget);

class BulletGob : public TankShotGob
{
public:
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// Gob methods

	virtual GobType GetType();

	// StateMachine methods


public:
	static AnimationData *s_panidShot;
};
BulletGob *CreateBulletGob(WCoord wx, WCoord wy, WCoord wxTarget, WCoord yTarget, int nDamage, Gid gidOwner, Gid gidTarget, bool fEagle = false);

class AndyShotGob : public TankShotGob
{
public:
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// Gob methods

	virtual GobType GetType();

	// StateMachine methods


public:
	static AnimationData *s_panidShot;
};
AndyShotGob *CreateAndyShotGob(WCoord wx, WCoord wy, WCoord wxTarget, WCoord yTarget, int nDamage, Gid gidOwner, Gid gidTarget, bool fEagle = false);

class RocketGob : public Gob
{
public:
	RocketGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	bool Init(WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget);

	// Gob methods

	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool IsSavable();
	virtual GobType GetType();
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	Gid m_gidOwner;
	Gid m_gidTarget;
	WLineIterator m_li;
	Animation m_ani;
	int m_nDamage;

	static AnimationData *s_panidRocket;
	static int s_nTrailStrip;
};
RocketGob *CreateRocketGob(WCoord wx, WCoord wy, WCoord wxTarget, WCoord wyTarget, int nDamage, Gid gidOwner, Gid gidTarget);

// SmokeGob

class SmokeGob : public AnimGob
{
public:
	SmokeGob(int cLoops);
	bool Init(WCoord wx, WCoord wy, StateMachineId smidNotify);

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool IsSavable();
	virtual GobType GetType();

	// AnimGob overrides

	virtual bool OnStripDone();

private:
	int m_cLoops;
};

// PuffGob

class PuffGob : public AnimGob
{
public:
	PuffGob();
	bool Init(WCoord wx, WCoord wy, StateMachineId smidNotify);

	// Gob overrides

	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool IsSavable();
	virtual GobType GetType();
};


// WarehouseGob

class WarehouseGob : public StructGob
{
public:
	WarehouseGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// StructGob overrides

	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void Takeover(Player *pplr);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

// WallGob: one tile of wall, owned by a side (or neutral). Its picture joins
// the walls next to it. Units attack walls only when ordered to.

class WallGob : public StructGob
{
public:
	WallGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	static WallGob *CreateAt(TCoord tx, TCoord ty, Player *pplr, fix fxHealth);
	static bool IsWallAt(TCoord tx, TCoord ty);

	// StructGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void SetHealth(fix fxHealth);
	virtual bool IsTakeoverable(Player *pplr);
	virtual void PopupMenu();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	int CalcNeighbourMask();
	void RedrawNeighbours();
};

// TowerGob

class TowerGob : public StructGob
{
public:
	TowerGob(TowerConsts *ptwrc);
	static bool InitClass(TowerConsts *ptwrc, IniReader *pini);
	static void ExitClass(TowerConsts *ptwrc);
	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy) = 0;
#ifdef DRAW_LINES
	void DrawTargetLine(DibBitmap *pbm, int xViewOrigin, int yViewOrigin);
#endif

	// Gob overrides

	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual void GetClippingBounds(Rect *prc);

	// UnitGob overrides

	virtual void Activate();
	virtual void DefUpdate();
	virtual void SetTarget(Gid gid, WCoord wxTarget = 0, WCoord wyTarget = 0, WCoord wxCenter = 0, WCoord wyCenter = 0, TCoord tcRadius = 0, WCoord wcMoveDistPerUpdate = 0);
	virtual bool IsValidTarget(Gob *pgobTarget);
    virtual dword GetAnimationHash();
    virtual void GetAnimationBounds(Rect *prc, bool fBase);
    virtual void DrawAnimation(DibBitmap *pbm, int x, int y);

	// StructGob override

	virtual bool IsTakeoverable(Player *pplr);

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

protected:

	// UnitGob protected override

	bool IsBetterTarget(UnitGob *puntNew, UnitGob *puntCurrent);
	virtual UnitGob *FindEnemyNearby(TCoord tcRange);

	// members

	Animation m_aniTurret;
	Direction16 m_dir16Turret;
	Gid m_gidTargetPrimary;
	Gid m_gidTargetSecondary;
	long m_tLastFire;

#ifdef DRAW_LINES
public:
#endif
	WPoint m_wptTarget;
};

// GunTowerGob

class GunTowerGob : public TowerGob
{
public:
	GunTowerGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// TowerGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);

};

// RocketTowerGob

class RocketTowerGob : public TowerGob
{
public:
	RocketTowerGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// TowerGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);

};

class ArtilleryGob : public MobileUnitGob // art
{
public:
	ArtilleryGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// UnitGob overrides

	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);

	// StateMachine methods

//	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class ArtilleryShotGob : public TankShotGob
{
public:
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// Gob methods

	virtual GobType GetType();

	// StateMachine methods


public:
	static AnimationData *s_panidShot;
};
ArtilleryShotGob *CreateArtilleryShotGob(WCoord wx, WCoord wy, WCoord wxTarget, WCoord yTarget, int nDamage, Gid gidOwner, Gid gidTarget);

class AndyGob : public MobileUnitGob
{
public:
	AndyGob();
	AndyGob(MobileUnitConsts *pmuntc);
	static bool InitClass(IniReader *pini);
	static void ExitClass();

	// UnitGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual bool Fire(UnitGob *puntTarget, WCoord wx, WCoord wy, WCoord wdx, WCoord wdy);
	virtual void Idle();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);
};

class FoxGob : public AndyGob // fox
{
public:
	FoxGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();

};

class ReplicatorGob : public StructGob // rep
{
public:
	ReplicatorGob();
	static bool InitClass(IniReader *pini);
	static void ExitClass();
	void GetInputTilePosition(TPoint *ptpt);
	void Enable(bool fEnable);

	// UnitGob overrides

	virtual bool Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName);
	virtual void Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer);
	virtual void GetClippingBounds(Rect *prc);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);

	// StructGob overrides

	virtual void Activate();
	virtual void Deactivate();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

	// For query of replicator input points

	static void ClearReplicatorCount() {
		s_cReplicators = 0;
	}

	static int GetReplicatorCount() {
		return s_cReplicators;
	}
	static void GetReplicatorInputPoint(int n, TPoint *ptpt) {
		*ptpt = s_atptInput[n];
	}

protected:
	virtual void WaitingForCredits(bool fNeed, bool fOverride = false, Player *pplr = NULL);

private:
	bool ClearOutputBay(TCoord txBay, TCoord tyBay, TCoord txOut, TCoord tyOut);

	bool m_fEnabled;
	bool m_fReplicating;
	int m_ifrmLights;
	Player *m_pplrNeedCredits;

	// For management of replicator input points

	static void AddReplicatorInputPoint(TCoord tx, TCoord ty);
	static void RemoveReplicatorInputPoint(TCoord tx, TCoord ty);
	static int s_cReplicators;
	static TPoint s_atptInput[10];
};

const int kdtxReplicatorInput = 2;
const int kdtyReplicatorInput = 0;
const int kdtxReplicatorOutput1 = 1;
const int kdtyReplicatorOutput1 = 2; // 1;
const int kdtxReplicatorOutput2 = 3;
const int kdtyReplicatorOutput2 = 2; // 1;

// ActivatorGob

class ActivatorGob : public AnimGob
{
public:
	// Gob overrides

	ActivatorGob() {m_fActivated = false;};
	virtual bool Init(IniReader *pini, FindProp *pfind, const char *pszName);
	virtual bool LoadState(Stream *pstm);
	virtual bool SaveState(Stream *pstm);
	virtual bool IsSavable();
	virtual GobType GetType();

	// StateMachine methods

	virtual int ProcessStateMachineMessage(State st, Message *pmsg);

private:
	bool m_fActivated;
};

// MemMgr - for allocs that are mostly read-only.

typedef void *HMem;
typedef word HandleEntry;
typedef word HandleEntryRef;

struct HMemStruct
{
	word her;
	byte iheap;
};

struct MemHeader
{
	word w;
	word cbBlock;
};

#define kcHandlesPerTable 64
struct HandleTable
{
	int iheFree;
	HandleEntry ahe[kcHandlesPerTable];
};

struct Heap
{
	byte *pbHeap;
	word cbHeap;
	MemHeader *pmhdrFreeFirst;
	word cbLargestFreeBlock;
	word wf;
	word nRec;
	void *hMem;
};

#define kfHintWillLock 0x1

class MemMgr
{
public:
	MemMgr();
	~MemMgr();

	void Init(dword cbDynReserve, dword cbMaxNeeded, dword *pcbTotal);
	void Exit();

	HMem AllocHandle(word cb, word wfHints = 0);
	void FreeHandle(HMem hmem);
	void SetLocked(HMem hmem);
	void ClearLocked(HMem hmem);
	void *GetPtr(HMem hmem);
	void WriteHandle(HMem hmem, word ib, void *pvSrc, word cb);

	void *AllocPtr(word cb);
	void FreePtr(void *pv);
	void WritePtr(void *pvDst, word ib, void *pvSrc, word cb);

	dword GetFreeSize(word *pcbLargestFree = NULL);
	dword GetTotalSize(dword *pcbDyn, dword *pcbDb);
	static void GetInitSize(dword cbDynReserve, dword *pcbDyn, dword *pcbStorage);

#ifdef INCL_VALIDATEHEAP
	void Validate();
	void ValidateHandleTableFreeEntries();
#endif

private:
	HandleTable *NewHandleTable();
	bool AllocHandleEntry(word *pher);
	void FreeHandleEntry(word her);
	Heap *NewHeap(void *pvHeap, word cbHeap, bool fDbRam, word nRec, void *hMem);
	void WriteHeader(Heap *pheap, MemHeader *pmhdrDst, MemHeader *pmhdrSrc, word cb = sizeof(MemHeader));
	HMem AllocBlock(int iheap, MemHeader *pmhdr, MemHeader *pmhdrFreeLast, word cbBlock, word wfHints);
	MemHeader *Compact(Heap *pheap, word cbBlock, MemHeader **ppmhdrFreeLast, word wfHints);
	MemHeader *SplitFreeBlock(Heap *pheap, MemHeader *pmhdr, MemHeader *pmhdrFreeLast, MemHeader *pmhdrNew, word wfHints);
	word GetLargestFreeBlockSize(Heap *pheap);

#ifdef INCL_VALIDATEHEAP
	void ValidateHandle(HMem hmem);
#endif

	Heap *m_apheap[250];	// HMemStruct::iheap is a byte
	int m_cheap;
	HandleTable **m_aphtab;
	int m_chtab;
	int m_chtabAlloced;
	dword m_cbTotalFreeBlocks;

};
extern MemMgr gmmgr;

// Cache Manager.
// 16 bytes for quick lookup

struct CacheEntry // ce
{
	CacheEntry *pceNext;
	CacheEntry *pcePrev;
	word wUniqueLock;
	word cbSize;
	HMem hmem;
};

#define kcCacheEntries 512
#define kwIndexMask 0x1ff
#define kwUniqueMask 0xfe00
#define kwLockMask 0x001ff
#define kwIncUnique 0x0200
#define kwIncLock 0x0001

class CacheMgr {
public:
	CacheMgr();
	~CacheMgr();
	bool Init();
	void Exit();

	void *GetPtr(CacheHandle hc);
	void *Lock(CacheHandle hc);
	void Unlock(CacheHandle hc);
	bool IsValid(CacheHandle hc);
	CacheHandle NewObject(void *pv, word cb, word wfHints = 0);
	void Write(CacheHandle hc, word ib, void *pvSrc, word cb);
	word GetSize(CacheHandle hc);
	bool MakeSpace(dword cb);
	dword GetTotalSize();
	dword GetLimit();
	void SetLimit(dword cbLimit);

private:
	CacheEntry *m_pceList;
	CacheEntry *m_pceFirst;
	CacheEntry *m_pceFree;
	dword m_cbTotalSize;
	dword m_cbLimit;

	inline CacheHandle MakeHandle(CacheEntry *pce)
	{
		return (CacheHandle)((pce - m_pceList) | (pce->wUniqueLock & kwUniqueMask));
	}

	inline CacheEntry *ValidateHandle(CacheHandle hc)
	{
		if (hc == 0)
			return NULL;
		word ice = (hc & kwIndexMask);
		if (ice >= kcCacheEntries)
			return NULL;
		CacheEntry *pce = &m_pceList[ice];
		if ((pce->wUniqueLock & kwUniqueMask) != (hc & kwUniqueMask))
			return NULL;
		return pce;
	}

	inline void Add(CacheEntry *pce)
	{
		if (m_pceFirst != NULL) {
			pce->pceNext = m_pceFirst;
			m_pceFirst->pcePrev->pceNext = pce;
			pce->pcePrev = m_pceFirst->pcePrev;
			m_pceFirst->pcePrev = pce;
		} else {
			pce->pceNext = pce;
			pce->pcePrev = pce;
		}
		m_pceFirst = pce;
	}

	inline void Remove(CacheEntry *pce)
	{
		pce->pcePrev->pceNext = pce->pceNext;
		pce->pceNext->pcePrev = pce->pcePrev;
		if (pce == m_pceFirst) {
			m_pceFirst = pce->pceNext;
			if (m_pceFirst == pce)
				m_pceFirst = NULL;
		}
	}

	inline void AddToFreeList(CacheEntry *pce)
	{
		if (m_pceFree != NULL) {
			pce->pceNext = m_pceFree;
			m_pceFree->pcePrev->pceNext = pce;
			pce->pcePrev = m_pceFree->pcePrev;
			m_pceFree->pcePrev = pce;
		} else {
			pce->pceNext = pce;
			pce->pcePrev = pce;
		}
		m_pceFree = pce;
	}

	inline void RemoveFromFreeList(CacheEntry *pce)
	{
		pce->pcePrev->pceNext = pce->pceNext;
		pce->pceNext->pcePrev = pce->pcePrev;
		if (pce == m_pceFree) {
			m_pceFree = pce->pceNext;
			if (m_pceFree == pce)
				m_pceFree = NULL;
		}
	}

	void Discard(CacheEntry *pce);
};
extern CacheMgr gcam;

// SoundMgr

#define kcChannelsMax 32	// at most 32: SoundDevice::GetBusyChannels is a bit mask

} // namespace wi
    
#include "sounddevice.h"

namespace wi {

struct PcmHeader // pcmh
{
	byte *pb;
	word cb;
};

struct SfxEntry // sfxe
{
	byte nSound;
	byte nChannel; // unused
	byte nPriority;
};
#define kcbSfxEntry 3

class SoundMgr // sndm
{
public:
	SoundMgr();
	~SoundMgr();

	bool Init();
	void Exit();
	void Enable(bool fEnable);
	bool IsEnabled();
	// Sound groups, each with its own volume (Options > Sound): Interface
	// (clicks), Battle & Base (PlaySfxAt, credits, repairs, unloading),
	// Unit Voices (PlayResponseSfx) and Announcements (everything else)

	void PlaySfx(Sfx sfx, int nPan = 0, int nVolume = 256);	// group chosen by sfx
	void PlaySfxAt(Sfx sfx, WCoord wx, WCoord wy);	// battlefield sound
	void PlaySfxIfSeen(Sfx sfx, Gob *pgob);		// placed like PlaySfxAt, only if in view
	void PlayResponseSfx(Sfx sfx);	// a unit's reply to the player
	void PlaySfxRaw(Sfx sfx, int nPan, int nVolume);	// no group volume
	void WaitSilence();
	long FilterSleepTicks(long ct);
	void SetVolume(int nVolume);
	int GetVolume();

private:
	int m_cChannels;
	byte m_anPriorityChannel[kcChannelsMax];
	byte m_anSoundChannel[kcChannelsMax];	// which sound each channel last started
	short m_anVolumeChannel[kcChannelsMax];	// and how loud
	long m_amsStartChannel[kcChannelsMax];	// and when
	FileMap *m_afmap;
	PcmHeader *m_apcmh;
	int m_cpcmh;
	SfxEntry *m_asfxe;
	int m_csfxe;
	FileMap m_fmapSfxEntries;
	SoundDevice *m_psndd;
};
extern SoundMgr gsndm;
extern int gnVolumeEffects;		// 0..256
extern int gnVolumeBattle;		// 0..256
extern int gnVolumeResponses;	// 0..256
extern int gnVolumeMenu;		// 0..256

// StringTable Class

class StringTable
{
public:
	StringTable();
	~StringTable();

	bool Init(char *pszFilename);
	bool GetString(int id, char *psz, int cb);

private:
	File *m_pfil;
};
extern StringTable *gpstrtbl;


// AlertControl

#define kcchAlertText 64
class AlertControl : public LabelControl, public Timer
{
public:
	AlertControl();
	~AlertControl();

	void AddText(const char *psz);

	// Control methods

	virtual bool Init(Form *pfrm, IniReader *pini, FindProp *pfind);

	// Timer methods

	virtual void OnTimer(long tCurrent);

private:
	word m_wf;
};

//
// Preferences support
//

// Preferences are saved as is to prefs.bin. The version and size identify
// the layout; a file with a different layout is ignored.

struct Preferences // prefs
{
	dword dwVersion;
	dword cbSize;
	word nVolume;					// volume
	word wfPerfOptions;
	word wfHandicap;				// Difficulty-affecting flags
	int ctGameSpeed;				// game speed
	word fShowFPS;		// frames-per-second counter (was fLassoSelection, always saved 0)
	short nHueOffset;				// HSL settings
	short nSatMultiplier;
	short nLumOffset;
	word cxModeBest;				// Remember what mode / data is best. Check against in case config changes
	word cyModeBest;
	word nDepthModeBest;
	word nDepthDataBest;
	word nSizeDataBest;
	word nDegreeOrientationBest;
	word cxModeLast;				// Remember what mode / data was chosen
	word cyModeLast;
	word nDepthModeLast;
	word nDepthDataLast;
	word nSizeDataLast;
	word nDegreeOrientationLast;
	double nScrollSpeed;
	word fFullscreen;				// added in 0x201
	word fSmoothMotion;
	word nVolumeBattle;				// added in 0x202: 0..256
	word nVolumeResponses;
	word nVolumeEffects;			// added in 0x203: 0..256
	word nVolumeMenu;
};

#define knVersionPreferences 0x203
#define knVersionPreferences202 0x202	// without nVolumeEffects / nVolumeMenu
#define knVersionPreferences201 0x201	// without nVolumeBattle / nVolumeResponses
#define knVersionPreferences200 0x200	// same layout, without the fields added since
extern bool gfSmoothMotion;
extern Preferences gprefsInit;
bool HostSavePreferences(void *pv, int cb);
int HostLoadPreferences(void *pv, int cb);

// Our in-game MessageBox

bool HtMessageBox(word wf, const char *pszTitle, const char *pszFormat, ...);
bool HtMessageBox(word idf, word wf, const char *pszTitle, const char *pszBody);
DialogForm *CreateHtMessageBox(word idf, word wf, const char *pszTitle, const char *pszBody);

const word kfMbWhiteBorder = 0x0001;
const word kfMbClearDib = 0x0002;
const word kfMbKeepTimersEnabled = 0x0004;

// Globals

extern byte grvlp[];
extern byte grvlpLarge[];
extern IniReader *gpiniForms;
extern IniReader *gpiniGame;
extern Font *gapfnt[];
extern Player *gpplrLocal;
extern byte *gmpiclriclrShadow;
extern UnitConsts *gapuntc[kutMax];
extern PlaceStructureForm *gpfrmPlace;
#ifdef DRAW_PATHS
extern bool gfDrawPaths;
#endif
#ifdef DRAW_LINES
extern bool gfDrawLines;
#endif
extern bool gfPlayingBack;
extern CommandQueue gcmdq;
extern bool gfOvermindEnabled;
extern PlayerMgr gplrm;
extern Player gplrDummy;
extern TBitmap *gaptbmScorches[4];
extern BuilderConsts gbldcHq;
extern byte *gpbScratch;
extern word gcbScratch;
extern int g_mpDirToDx[8];
extern int g_mpDirToDy[8];
extern int gcxTile;
extern int gcyTile;
extern int gcxyBorder;
extern bool gfAutosave;
extern DibBitmap *gpbmClip;
extern FormMgr *gpfrmmSim;
extern bool gfAttackMoveOrder;
extern bool gfQueueOrder;
extern bool gfForceAttackOrder;	// Ctrl+right-click: attack the unit even if it is a friend	// Shift+right-click: queue the order after the current ones

// Allies share vision: the local player sees what their allies' units and
// buildings see
bool SharesVisionWithLocal(Player *pplr);
void RevealAroundUnit(UnitGob *punt);

// Where the last "under attack" alert happened, for the Space key
extern WPoint gwptLastAlert;
extern bool gfLastAlertValid;
void NoteAlertLocation(Gob *pgob);
extern FormMgr *gpfrmmInput;
extern MultiFormMgr *gpmfrmm;
#ifdef STATS_DISPLAY
extern long gcPathScoresCalced;
#endif
extern int gnHueOffset;
extern int gnSatMultiplier;
extern int gnLumOffset;
extern double gnScrollSpeed;
extern int gtGameSpeed;
extern AnimationData *g_panidMoveTarget;
extern UpdateMap *gpupdSim;
extern Color *gaclrFixed;
extern Color gaclr8bpp[];
extern Color gaclr4bpp[];
extern byte gmpDistFromDxy[10][10];
extern TRect *gptrcMapOpaque;
extern TRect gtrcMapOpaque;
extern Upgrade gaupg[kupgtMax];
extern MiniMapControl *gpmm;
#ifdef STRESS
extern bool gfStress;
extern long gtStressTimeout;
#endif
extern byte gmpdirdirOpposite[8];
extern int ganSquared[256];
extern bool gfClearFog;
extern int gnUnitCostMin;
extern int gnUnitCostMax;
extern fix gfxMobileUnitArmorStrengthMin;
extern fix gfxMobileUnitArmorStrengthMax;
extern fix gfxStructureArmorStrengthMin;
extern fix gfxStructureArmorStrengthMax;
extern fix gfxStructureDamageMin;
extern fix gfxStructureDamageMax;
extern fix gfxVehicleDamageMin;
extern fix gfxVehicleDamageMax;
extern fix gfxInfantryDamageMin;
extern fix gfxInfantryDamageMax;
extern TCoord gtcFiringRangeMin;
extern TCoord gtcFiringRangeMax;
extern WCoord gwcMoveDistPerUpdateMin;
extern WCoord gwcMoveDistPerUpdateMax;
extern int gnPowerSupplyMin;
extern int gnPowerSupplyMax;
extern int gnPowerDemandMin;
extern int gnPowerDemandMax;
extern int gaiclrSide[kcSides];
extern int gatGameSpeeds[15];
extern char *gszVersion;
extern char gszBuildDate[];
extern char gszBuildTime[];
extern bool gfDragSelecting;
extern WPoint s_wptSelect1, s_wptSelect2;
extern WRect gwrcSelection;
extern word gwfPerfOptions;
extern word gwfHandicap;
extern int gcStructGobsHumanLimitSP;
extern int gcStructGobsComputerLimitSP;
extern int gcMuntGobsHumanLimitSP;
extern int gcMuntGobsComputerLimitSP;
extern int gcStructGobsComputerDeltaSP;
extern int gcStructGobsHumanDeltaSP;
extern int gcSceneryGobsLimit;
int SetGobLimitsForMap(int ctx, int cty);
extern int gcScorchGobsLimit;
extern int gcSupportGobsLimit;
extern bool gfGrayscale;

inline Color GetColor(int iclr) {
	return gaclrFixed[iclr];
}

inline Color GetSideColor(Side side) {
	return GetColor(gaiclrSide[side]);
}

// Helpers
inline UnitType UnitTypeFromPVoid(void *pv) {
    return *((UnitType *)&pv);
}

// Out-of-band data for gameOverEvent (knGoLoadSavedGame case)
extern Stream *gpstmSavedGame;
extern int gimodeReinitialize;
extern bool gfLoadReinitializeSave;

void HtStrupr(char *psz);

#ifdef DEBUG
int _GetRandom(char *pszFile, int nLine);
#define GetRandom() _GetRandom(__FILE__, __LINE__)
#else
int _GetRandom();
#define GetRandom() _GetRandom()
#endif
void SetRandomSeed(unsigned long nSeed);
int GetAsyncRandom();
void DrawBuildProgressIndicator(DibBitmap *pbm, Rect *prc, int nNumerator, int nDenominator);
void DrawSelectionIndicator(DibBitmap *pbm, Rect *prc, int nNumerator, int nDenominator);
void DrawHealthIndicator(DibBitmap *pbm, Rect *prc, int nNumerator, int nDenominator);
void DrawFullnessIndicator(DibBitmap *pbm, Rect *prc, int nPips, int nPipsMax);
void DrawBorder(DibBitmap *pbm, Rect *prc, int nThickness, Color clr, UpdateMap *pupd = NULL);
void DrawBitmapBorder(DibBitmap *pbm, UpdateMap *pupd, Rect *prc, AnimationData *panid, int ifrm = 0, Side side = ksideNeutral);
int DrawFancyText(DibBitmap *pbm, Font *pfntDefault, char *psz, int x, int y, int cch = 0);
int GetFancyTextExtent(Font *pfntDefault, char *psz, int cch = 0);
Direction CalcDir(int dx, int dy);
Direction16 CalcDir16(int dx, int dy);
bool FormDragger(Form *pfrm, Event *pevt);
Direction TurnToward(Direction dirTo, Direction dirFrom);
int CalcCreditsShare(Player *pplr);
void DrawTileMap(byte **ppbMap, int ctx, int cty, byte *pbDst, int cbDstStride, int cxLeftTile, int cyTopTile, int cxRightTile, int cyBottomTile, int ctxInside, int ctyInside, int cxTile, int cyTile);
FormMgr *CreateFormMgr(DibBitmap *pbm);
void ShadowHelper(DibBitmap *pbm, UpdateMap *pupd, Rect *prc);
void FillHelper(DibBitmap *pbm, UpdateMap *pupd, Rect *prc, Color clr);
void BltHelper(DibBitmap *pbm, HtBitmap *phtbm, UpdateMap *pupd, int xDst, int yDst);
void RgbToHsl(byte bR, byte bG, byte bB, word *pnH, word *pnS, word *pnL);
void HslToRgb(word nH, word nS, word nL, byte *pbR, byte *pbG, byte *pbB);
void SetHslAdjustedPalette(Palette *ppal, short nHueOffset, short nSatMultiplier, short nLumOffset);
UnitConsts *GetUnitConsts(GobType gt);
Sfx SfxFromCategory(SfxCategory sfxc);
bool ParseNumber(char **ppsz, int *pn);
bool ParseArea(char **ppsz, int *pn);
bool ParseUnitMask(char **ppsz, UnitMask *pum);
bool ParseUpgradeMask(char **ppsz, UpgradeMask *pupgm);
bool ParseString(char **ppsz, char *psz);
SideMask GetSideMaskFromCaSideMask(Side sideCur, word wfCaSideMask);
int GetPlayersListFromCaSideMask(Side sideCur, word wfMask, Player **applr);
void Ecom(int nCharFrom, int nCharTo, char *pszMessage, int nBackground, bool fMore);
bool DoModalGameOptionsForm(Palette *ppal, bool fInGame);
bool IsTileFree(TCoord tx, TCoord ty, byte bf = kbfStructure, Gob **ppgob = NULL);
void ShowAlert(int id);
void ShowAlert(const char *psz);
void SendAttackCommand(Gid gidReceiver, Gid gidTarget);
word AggBitsFromAgg(int nAggressiveness);
void FindNearestFreeTile(TCoord tx, TCoord ty, WPoint *pwpt, byte bf = kbfStructure | kbfMobileUnit);
void GetPrerequisiteString(char *psz, UnitConsts *puntc);
void BringInBounds(WCoord *pwx, WCoord *pwy);
Direction DirectionFromLocations(TCoord txOld, TCoord tyOld, TCoord txNew, TCoord tyNew);
int RadiusFromUnitCount(int cUnits);
void MoveUnitsToArea(MobileUnitGob **apmunt, int cpmunt, TRect *ptrc);
void ExpandVars(char *pszSrc, char *pszBuff, int cbBuff);
void GetRankTitle(char *psz, int cb);
void FormatButtons(Form *pfrm, word *aidc, int cidc, int idcRef, int idcRefNext);
void GetSpeedMultiplierString(char *psz, long tGameSpeed);


// Misc runtime stuff

char *itoa (int val, char *buf, int radix);

#ifdef DRAW_PATHS
void LoadArrows();
void FreeArrows();
void DrawArrow(DibBitmap *pbm, int x, int y, Direction dir, Side side);
#endif


struct ClipInfo
{
	int cxClip;
	int cx;
	int cbRowDst;
	int cyClip;
};


extern "C" word DecompressChunk(byte **ppbCompressed, byte *pbDecompressed, byte *pbCacheEnd, word cbMax);
extern "C" void DecompressToCache(byte *pbCompressed, CacheHandle hc, CompressionHeader *pcoh);

// Thunks

void FillShadowThunk(byte *pbDst, int cbRowDst, int cx, int cy, byte *aclrMap);
void FillThunk(byte *pbDst, int cx, int cy, int cbStride, byte bFill);
void LeftToRightBltThunk(byte *pbSrc, int cxSrcStride, byte *pbDst, int cxDstStride, int cx, int cy);
void RightToLeftBltThunk(byte *pbSrc, int cxSrcStride, byte *pbDst, int cxDstStride, int cx, int cy);
void DrawTileMapThunk(byte **ppbMap, int ctx, int cty, byte *pbDst, int cbDstStride, int cxLeftTile, int cyTopTile, int cxRightTile, int cyBottomTile, int ctxInside, int ctyInside, int cxTile, int cyTile);
word Compile8Thunk(byte *pb, ScanData *psd, bool fOdd);
void DrawDispatchThunk(byte *pb, byte *pbSrc, byte *pbDst, int cbReturn, dword *mpscaiclrSide, byte *mpiclriclrShadow);

int YClipToScan(int yClip, int cx, byte*& pop, byte*& pbSrc, word*& psc);
int DrawScan(byte *pbDst, int cx, byte*& pbSrc, byte*& pop, word*& psc,
        dword *mpscaiclr, byte *mpiclriclrShadow);

// ARM stuff

void InitArmCode();
void ExitArmCode();
void DrawTileMapArm(byte **ppbMap, int ctx, int cty, byte *pbDst, int cbDstStride, int cxLeftTile, int cyTopTile, int cxRightTile, int cyBottomTile, int ctxInside, int ctyInside, int cxTile, int cyTile);
void LeftToRightBltArm(byte *pbSrc, int cxSrcStride, byte *pbDst, int cxDstStride, int cx, int cy);
void RightToLeftBltArm(byte *pbSrc, int cxSrcStride, byte *pbDst, int cxDstStride, int cx, int cy);
void memsetArm(byte *pbDst, byte b, dword cb);
void FillArm(byte *pbDst, int cx, int cy, int cbStride, byte bFill);
void FillShadowArm(byte *pbDst, int cbRowDst, int cx, int cy, byte *aclrMap);
word Compile8Arm(byte *pb, ScanData *psd, bool fOdd);
void DrawDispatchArm(byte *pb, byte *pbSrc, byte *pbDst, int cbReturn, dword *mpscaiclrSide, byte *mpiclriclrShadow);
extern bool gfArmPresent;

// Host stuff

bool HostInit();
void HostExit();
bool HostGetEvent(Event *pevt, long ctWait = -1);
void HostOutputDebugString(char *pszFormat, ...);
long HostGetTickCount();
dword HostGetModifiers();
bool HostGetMousePosition(int *px, int *py);	// game screen pixels
long HostGetMillisecondCount();
dword HostGetCurrentKeyState(dword keyBit);
void HostMessageBox(TCHAR *pszFormat, ...);
Display *HostCreateDisplay();
const char *HostGetMainDataDir();
void HostNotEnoughMemory(bool fStorage, dword cbFree, dword cbNeed);
SoundDevice *HostOpenSoundDevice();
void HostSleep(dword ct);

// Mission packs (custom missions). HostMountMissionPacks pushes the packs
// in the Missions folders, and any given with -pdb on the command line, onto
// gpakr. HostGetStartupLevel returns the level given with -l, or NULL.

void HostSetCommandLine(int argc, char *argv[]);
void HostMountMissionPacks();
const char *HostGetStartupLevel();

// Date

struct Date
{
	int nYear;
	int nMonth;
	int nDay;
};
int CompareDates(Date *pdate1, Date *pdate2);



// Host file IO

const word kfOfRead = 0x0001;		// same as "rb"
const word kfOfWrite = 0x0002;		// same as "wb"


// Save game

class Stream
{
public:
	virtual ~Stream() {}
	virtual void Close() = 0;
	virtual dword Read(void *pv, dword cb) = 0;
	virtual dword Write(void *pv, dword cb) = 0;
	virtual bool IsSuccess() = 0;

	// Helpers

	void ReadString(char *psz, int cb);
	void ReadBytesRLE(byte *pb, int cb);
	inline byte ReadByte()
	{
		byte b = 0;
		Read(&b, sizeof(b));
		return b;
	}
	inline word ReadWord()
	{
		word w = 0;
		Read(&w, sizeof(w));
		return w;
	}
	inline dword ReadDword()
	{
		dword dw = 0;
		Read(&dw, sizeof(dw));
		return dw;
	}
	inline int ReadCoord()	// TCoord / WCoord (32 bit since maps grew to 256 tiles)
	{
		return (int)ReadDword();
	}

	void WriteString(char *psz);
	void WriteBytesRLE(byte *pb, int cb);
	inline void WriteByte(byte b)
	{
		Write(&b, sizeof(b));
	}
	inline void WriteWord(word w)
	{
		Write(&w, sizeof(w));
	}
	inline void WriteDword(dword dw)
	{
		Write(&dw, sizeof(dw));
	}
	inline void WriteCoord(int n)
	{
		WriteDword((dword)n);
	}

private:
	byte *FindRLERepeat(byte *pbStart, byte *pbMax, int cbMin);
	void WriteRLEChunk(byte *pb, int cb, bool fRepeat);
};

const int knGameReinitializeSave = -3;
const int knGameAutosave = 10;

#define kszTempName "htsavetemp"
bool HostGetSaveGameName(int nGame, char *psz, int cb, Date *pdate, int *pnHours24, int *pnMinutes, int *pnSeconds);
Stream *HostNewSaveGameStream(int nGame, char *pszName);
Stream *HostOpenSaveGameStream(int nGame, bool fDelete = false);
Stream *PickLoadGameStream();
bool PickSaveGameStream(Stream **ppstm);
void DeleteStaleSaveGames();
// Version of the saved game format. Saved games of other versions can't be
// loaded.
const byte knVerGameState = 12;
bool CheckSaveGameVersion(char *pszVersion, byte bVer);
bool HostDeleteSaveGame(char *psz, int nGame);

// Debug stuff

#define MpTrace 1 ? (void)0 : ::wi::HostOutputDebugString

#ifdef INCL_TRACE
#define Trace ::wi::HostOutputDebugString
//#define Trace(fmt, args...) LOG() << base::Log::Format(fmt, ## args)
#else
#define Trace 1 ? (void)0 : ::wi::HostOutputDebugString
#endif

#ifdef STATUS_LINE
void Status(const char *psz);
#else
#define Status(psz)
#endif



} // namespace wi

#endif // __HT_H__
