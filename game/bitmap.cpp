#include "ht.h"

namespace wi {

// Dib bitmaps

DibBitmap *CreateDibBitmap(byte *pb, int cx, int cy)
{
	DibBitmap *pbm = new DibBitmap;
	Assert(pbm != NULL, "out of memory!");
	if (pbm == NULL)
		return NULL;
	if (!pbm->Init(pb, cx, cy)) {
		delete pbm;
		return NULL;
	}
	return pbm;
}

DibBitmap::DibBitmap()
{
    m_cbRow = 0;
    m_cb = 0;
	m_pb = NULL;
	m_siz.cx = 0;
	m_siz.cy = 0;
	m_wf = 0;
	m_pdwTC = NULL;
	m_pbTCUnder = NULL;
	m_fFreeTC = false;
}

DibBitmap::~DibBitmap()
{
	if (m_wf & kfDibFreeMem)
		delete[] m_pb;
	m_pb = NULL;
	if (m_fFreeTC) {
		delete[] m_pdwTC;
		delete[] m_pbTCUnder;
	}
	m_pdwTC = NULL;
	m_pbTCUnder = NULL;
}

bool DibBitmap::InitTrueColor(dword *pdw, byte *pbUnder)
{
	if (m_fFreeTC) {
		delete[] m_pdwTC;
		delete[] m_pbTCUnder;
	}
	m_fFreeTC = false;
	if (pdw == NULL) {
		pdw = new dword[m_cb];
		pbUnder = new byte[m_cb];
		memset(pdw, 0, m_cb * sizeof(dword));
		memset(pbUnder, 0, m_cb);
		m_fFreeTC = true;
	}
	m_pdwTC = pdw;
	m_pbTCUnder = pbUnder;
	return true;
}

void DibBitmap::ClearTrueColor(Rect *prc)
{
	if (m_pdwTC == NULL)
		return;
	Rect rc;
	Rect rcDib;
	rcDib.Set(0, 0, m_siz.cx, m_siz.cy);
	if (prc == NULL)
		prc = &rcDib;
	if (!rc.Intersect(prc, &rcDib))
		return;
	for (int y = rc.top; y < rc.bottom; y++)
		memset(&m_pdwTC[(long)y * m_cbRow + rc.left], 0, rc.Width() * sizeof(dword));
}

// Copies the true colour layer along with a blt (prcSrc already clipped)

void DibBitmap::BltTrueColor(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst)
{
	if (m_pdwTC == NULL || prcSrc->IsEmpty())
		return;
	int cx = prcSrc->Width();
	if (pbmSrc->m_pdwTC == NULL) {
		Rect rcDst;
		rcDst.Set(xDst, yDst, xDst + cx, yDst + prcSrc->Height());
		ClearTrueColor(&rcDst);
		return;
	}

	// Row order matters when scrolling within one buffer

	int cy = prcSrc->Height();
	bool fUp = !(pbmSrc->m_pdwTC == m_pdwTC && yDst > prcSrc->top);
	for (int i = 0; i < cy; i++) {
		int dy = fUp ? i : cy - 1 - i;
		long iSrc = (long)(prcSrc->top + dy) * pbmSrc->m_cbRow + prcSrc->left;
		long iDst = (long)(yDst + dy) * m_cbRow + xDst;
		memmove(&m_pdwTC[iDst], &pbmSrc->m_pdwTC[iSrc], cx * sizeof(dword));
		memmove(&m_pbTCUnder[iDst], &pbmSrc->m_pbTCUnder[iSrc], cx);
	}
}

bool DibBitmap::Init(byte *pb, int cx, int cy)
{
	m_cbRow = (cx + 1) & ~1;
	m_cb = (int)cy * m_cbRow;
	if (pb == NULL) {
		m_pb = new byte[m_cb];
		Assert(m_pb != NULL, "out of memory!");
		m_wf |= kfDibFreeMem;
	} else {
		m_pb = pb;
	}
	m_siz.cx = cx;
	m_siz.cy = cy;
	return m_pb != NULL;
}

byte *DibBitmap::GetBits()
{
	return m_pb;
}

int DibBitmap::GetRowBytes()
{
	return m_cbRow;
}

#define TopToBottomBltThunk LeftToRightBltThunk
#define FastestBltThunk LeftToRightBltThunk
#define BottomToTopBltThunk RightToLeftBltThunk

void DibBitmap::Blt(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst)
{
	// Clip as BltBits does, so the true colour layer gets the same rect

	Size sizSrc;
	pbmSrc->GetSize(&sizSrc);
	Rect rc = *prcSrc;
	if (rc.left < 0)
		rc.left = 0;
	if (rc.top < 0)
		rc.top = 0;
	if (rc.right > sizSrc.cx)
		rc.right = sizSrc.cx;
	if (rc.bottom > sizSrc.cy)
		rc.bottom = sizSrc.cy;
	int x = xDst, y = yDst;
	if (x < 0) {
		rc.left -= x;
		x = 0;
	}
	if (y < 0) {
		rc.top -= y;
		y = 0;
	}
	if (x + rc.Width() > m_siz.cx)
		rc.right -= x + rc.Width() - m_siz.cx;
	if (y + rc.Height() > m_siz.cy)
		rc.bottom -= y + rc.Height() - m_siz.cy;

	BltBits(pbmSrc, prcSrc, xDst, yDst);
	if (m_pdwTC != NULL && !rc.IsEmpty())
		BltTrueColor(pbmSrc, &rc, x, y);
}

void DibBitmap::BltBits(DibBitmap *pbmSrc, Rect *prcSrc, int xDst, int yDst)
{
	// Get src dib dimensions

	Size sizSrc;
	pbmSrc->GetSize(&sizSrc);

	// Clip to source rect

	if (prcSrc->left < 0)
		prcSrc->left = 0;
	if (prcSrc->top < 0)
		prcSrc->top = 0;
	if (prcSrc->right > sizSrc.cx)
		prcSrc->right = sizSrc.cx;
	if (prcSrc->bottom > sizSrc.cy)
		prcSrc->bottom = sizSrc.cy;

	// Clip to dest

	if (xDst < 0) {
		prcSrc->left -= xDst;
		xDst = 0;
	}
	if (yDst < 0) {
		prcSrc->top -= yDst;
		yDst = 0;
	}

	int xRightDst = xDst + prcSrc->Width();
	if (xRightDst > m_siz.cx)
		prcSrc->right -= xRightDst - m_siz.cx;
	int yBottomDst = yDst + prcSrc->Height();
	if (yBottomDst > m_siz.cy)
		prcSrc->bottom -= yBottomDst - m_siz.cy;

	// Anything to blt?

	if (prcSrc->IsEmpty())
		return;

	// Figure out direction to blt if we're doing a blt in the same dib

	if (this == pbmSrc) {
		// Calc addresses

		byte *pbSrc = m_pb + (long)prcSrc->top * m_cbRow + prcSrc->left;
		byte *pbDst = m_pb + (long)yDst * m_cbRow + xDst;

		// If same y, ...

		if (yDst == prcSrc->top) {
			// Overlap?

			int cxInterval = prcSrc->right - xDst;
			if (cxInterval > 0 && cxInterval < prcSrc->Width() * 2) {
				// Overlap. If bltting to the left, copy from left to right

				if (xDst < prcSrc->left) {
					LeftToRightBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
				} else {
					RightToLeftBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
				}
			} else {
				// No overlap. Do the fastest blt

				FastestBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
			}
		} else {
			int cyInterval = prcSrc->bottom - yDst;
			if (cyInterval > 0 && cyInterval < prcSrc->Height() * 2) {
				// Overlap. If bltting upwards, copy from top to bottom

				if (yDst < prcSrc->top) {
					TopToBottomBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
				} else {
					BottomToTopBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
				}
			} else {
				// No overlap. Do the fastest blt

				FastestBltThunk(pbSrc, m_cbRow, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
			}
		}
	} else {
		// No overlap. Do the fastest blt

		int cbRowBytesSrc = pbmSrc->GetRowBytes();
		byte *pbSrc = pbmSrc->GetBits() + (long)prcSrc->top * cbRowBytesSrc + prcSrc->left;
		byte *pbDst = m_pb + (long)yDst * m_cbRow + xDst;

		FastestBltThunk(pbSrc, cbRowBytesSrc, pbDst, m_cbRow, prcSrc->Width(), prcSrc->Height());
	}
}

void DibBitmap::Clear(Color clr)
{
	memset(m_pb, (byte)clr, m_cb);
}

void DibBitmap::Fill(int x, int y, int cx, int cy, Color clr)
{
	// Destination clipping

	if (x < 0) {
		cx += x;
		x = 0;
	}
	if (y < 0) {
		cy += y;
		y = 0;
	}
	if (x + cx > m_siz.cx)
		cx = m_siz.cx - x;
	if (y + cy > m_siz.cy)
		cy = m_siz.cy - y;

	if (cx <= 0 || cy <= 0)
		return;

	// Drawing

	byte *pbDst = m_pb + (long)y * m_cbRow + x;
	FillThunk(pbDst, cx, cy, m_cbRow, (byte)clr);

	// A fill covers the true colour layer

	if (m_pdwTC != NULL) {
		Rect rc;
		rc.Set(x, y, x + cx, y + cy);
		ClearTrueColor(&rc);
	}
}

void DibBitmap::Shadow(int x, int y, int cx, int cy)
{
	// Destination clipping

	if (x < 0) {
		cx += x;
		x = 0;
	}
	if (y < 0) {
		cy += y;
		y = 0;
	}
	if (x + cx > m_siz.cx)
		cx = m_siz.cx - x;
	if (y + cy > m_siz.cy)
		cy = m_siz.cy - y;

	if (cx <= 0 || cy <= 0)
		return;

	byte *pbRow = m_pb + (long)y * m_cbRow + x;
	FillShadowThunk(pbRow, m_cbRow, cx, cy, gmpiclriclrShadow);

	// Keep the true colour layer over shadowed pixels it was laid over

	if (m_pdwTC != NULL && gmpiclriclrShadow != NULL) {
		for (int yy = y; yy < y + cy; yy++) {
			long i = (long)yy * m_cbRow + x;
			for (int xx = 0; xx < cx; xx++, i++) {
				if ((m_pdwTC[i] >> 24) != 0 && gmpiclriclrShadow[m_pbTCUnder[i]] == m_pb[i])
					m_pbTCUnder[i] = m_pb[i];
			}
		}
	}
}

void DibBitmap::GetSize(Size *psiz)
{
	*psiz = m_siz;
}

#ifdef DRAW_LINES

#define LEFT            1
#define RIGHT           2
#define BOTTOM          4
#define TOP             8

#define SWAP(x, y)      { int _t = x; x = y; y = _t; }

#define OUTCODE(x, y, outcode, type)                                     \
{                                                                        \
  if (x < xl) outcode = LEFT, type = 1;                                  \
  else if (x > xr) outcode = RIGHT, type = 1;                            \
  else outcode = type = 0;                                               \
  if (y < yb) outcode |= BOTTOM, type++;                                 \
  else if (y > yt) outcode |= TOP, type++;                               \
}

#define CLIP(a1, a2, b1, da, da2, db2, as, bs, sa, sb,                   \
             amin, AMIN, amax, AMAX, bmin, BMIN, bmax, BMAX)             \
{                                                                        \
  if (out1) {                                                            \
    if (out1 & AMIN) { ca = db2 * (amin - a1); as = amin; }              \
    else if (out1 & AMAX) { ca = db2 * (a1 - amax); as = amax; }         \
    if (out1 & BMIN) { cb = da2 * (bmin - b1); bs = bmin; }              \
    else if (out1 & BMAX) { cb = da2 * (b1 - bmax); bs = bmax; }         \
    if (type1 == 2)                                                      \
      out1 &= (ca + da < cb) ? ~(AMIN | AMAX) : ~(BMAX | BMIN);   \
    if (out1 & (AMIN | AMAX)) {                                          \
      cb = (ca + da) / da2;                                       \
      if (sb >= 0) { if ((bs = b1 + cb) > bmax) return; }                \
      else { if ((bs = b1 - cb) < bmin) return; }                        \
      r += ca - da2 * cb;                                                \
    }                                                                    \
    else {                                                               \
      ca = (cb - da + db2 - 1) / db2;                                  \
      if (sa >= 0) { if ((as = a1 + ca) > amax) return; }                \
      else { if ((as = a1 - ca) < amin) return; }                        \
      r += db2 * ca - cb;                                                \
    }                                                                    \
  }                                                                      \
  else { as = a1; bs = b1; }                                             \
  alt = 0;                                                               \
  if (out2) {                                                            \
    if (type2 == 2) {                                                    \
      ca = db2 * ((out2 & AMIN) ? a1 - amin : amax - a1);                \
      cb = da2 * ((out2 & BMIN) ? b1 - bmin : bmax - b1);                \
      out2 &= (cb + da < ca + 1) ? ~(AMIN | AMAX) : ~(BMIN | BMAX);    \
    }                                                                    \
    if (out2 & (AMIN | AMAX)) n = (out2 & AMIN) ? as - amin : amax - as; \
    else { n = (out2 & BMIN) ? bs - bmin : bmax - bs; alt = 1; }         \
  }                                                                      \
  else n = (a2 >= as) ? a2 - as : as - a2;                               \
}

#define plot(x, y) 	(*(m_pb + (y * m_cbRow) + x) = (byte)clr)

void DibBitmap::DrawLine(short x1, short y1, short x2, short y2, Color clr)
{
	int xl = 0, yb = 0;
	int xr = m_siz.cx - 1;
	int yt = m_siz.cy - 1;

	int adx, ady, adx2, ady2, sx, sy;
	int out1, out2, type1, type2;
	int ca, cb, r, diff, xs, ys, n, alt;

	OUTCODE(x1, y1, out1, type1);
	OUTCODE(x2, y2, out2, type2);
	if (out1 & out2) return;
	if ((type1 != 0 && type2 == 0) || (type1 == 2 && type2 == 1)){
		SWAP(out1, out2);
		SWAP(type1, type2);
		SWAP(x1, x2);
		SWAP(y1, y2);
	}
	xs = x1;
	ys = y1;
	sx = 1;
	adx = x2 - x1;
	if (adx < 0) { adx = -adx; sx = -1; }
	sy = 1;
	ady = y2 - y1;
	if (ady < 0) { ady = -ady; sy = -1; }
	adx2 = adx + adx;
	ady2 = ady + ady;
	if (adx >= ady) {
		/*
		*      line is semi-horizontal
		*/
		r = ady2 - adx;
		CLIP(x1, x2, y1, adx, adx2, ady2, xs, ys, sx, sy,
				xl, LEFT, xr, RIGHT, yb, BOTTOM, yt, TOP);
		diff = ady2 - adx2;
		if (alt) {
			for (;; xs += sx) {       /* alternate Bresenham */
			plot(xs, ys);
			if (r >= 0 ) {
				if (--n < 0) break;
				r += diff;
				ys += sy;
			}
			else r += ady2;
			}
		}
		else{
			for (;; xs += sx) {       /* standard Bresenham */
			plot(xs, ys);
			if (--n < 0) break;
			if (r >= 0 ) { r += diff; ys += sy; }
			else r += ady2;
			}
		}
	}
	else {
		/*
		*      line is semi-vertical
		*/
		r = adx2 - ady;
		CLIP(y1, y2, x1, ady, ady2, adx2, ys, xs, sy, sx,
				yb, BOTTOM, yt, TOP, xl, LEFT, xr, RIGHT);
		diff = adx2 - ady2;
		if (alt) {
			for (;; ys += sy) {       /* alternate Bresenham */
			plot(xs, ys);
			if (r >= 0 ) {
				if (--n < 0) break;
				r += diff;
				xs += sx;
			}
			else r += adx2;
			}
		}
		else {
			for (;; ys += sy) {       /* standard Bresenham */
			plot(xs, ys);
			if (--n < 0) break;
			if (r >= 0 ) { r += diff; xs += sx; }
			else r += adx2;
			}
		}
	}
}
#endif // DRAW_LINES

DibBitmap *DibBitmap::Suballoc(int yTop, int cy)
{
	Assert(yTop < m_siz.cy && yTop + cy <= m_siz.cy);
	byte *pb = m_pb + (long)m_siz.cx * yTop;
	DibBitmap *pbm = CreateDibBitmap(pb, m_siz.cx, cy);
	if (pbm != NULL && m_pdwTC != NULL)
		pbm->InitTrueColor(m_pdwTC + (long)m_siz.cx * yTop, m_pbTCUnder + (long)m_siz.cx * yTop);
	return pbm;
}

void DibBitmap::Scroll(Rect *prcSrc, int xDst, int yDst)
{
    // Some implementations do blts differently (such as rotated dibs).
    // That is why this method - essentially a blt from and to the
    // destination - exists.
    Blt(this, prcSrc, xDst, yDst);
}
        
void DibBitmap::BltTiles(DibBitmap *pbmSrc, UpdateMap *pupd, int yTopDst)
{
	bool fFirst = true;
	Rect rc;
	while (pupd->EnumUpdateRects(fFirst, NULL, &rc)) {
		fFirst = false;
		Blt(pbmSrc, &rc, rc.left, rc.top + yTopDst);
	}
}


} // namespace wi
