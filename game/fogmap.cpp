#include <unordered_map>
#include "ht.h"

namespace wi {

// Soft fog edge tiles by 3x3 neighbourhood (see FogMap::GetSoftTile)

typedef std::unordered_map<unsigned long long, byte *> SoftCache;

FogMap::FogMap()
{
	m_pbMap = NULL;
	m_pbGxOriginal = NULL;
	m_nRegrowth = knGalaxiteRegrowthOff;
	m_tRegrowthLast = 0;
	m_panidWalls = NULL;
	memset(m_aptbm, 0, sizeof(m_aptbm));
	memset(m_apnCoverage, 0, sizeof(m_apnCoverage));
	m_fSoftInit = false;
	m_pvSoftCache = NULL;
	memset(m_aptbmGalax, 0, sizeof(m_aptbmGalax));

	// Each tile holds 4 bits. 0 is right most, 1 is top most, 2 is bottom most,
	// 3 is left most
	//
	// Init the pattern results. The only reason a table exists
	// (rather than using mask exclusively) is to disallow
	// unwanted results. 1001 and 0110 are the only two illegal
	// results with the current scheme.

	for (byte bSrc = 0; bSrc < 16; bSrc++) {
		for (byte bDst = 0; bDst < 16; bDst++) {
			byte bRes = ~(~bDst & ~bSrc) & 0xf;
			if (bRes == 9 || bRes == 6)
				bRes = 0;
			m_mpSrcDstResult[(((~bSrc) & 0xf) << 4) | bDst] = bRes;
		}
	}
}

FogMap::~FogMap()
{
	delete m_panidWalls;
	delete[] m_pbMap;
	delete[] m_pbGxOriginal;
	int n;
	for (n = 0; n < 16; n++) {
		delete m_aptbm[n];
		delete[] m_apnCoverage[n];
	}
	SoftCache *pcache = (SoftCache *)m_pvSoftCache;
	if (pcache != NULL) {
		for (SoftCache::iterator it = pcache->begin(); it != pcache->end(); ++it)
			delete[] it->second;
		delete pcache;
	}

	for (n = 0; n < 9; n++)
		delete m_aptbmGalax[n];
}

bool FogMap::Init(Size *psizTile, Size *psizMap)
{
	m_cxTile = psizTile->cx;
	m_cyTile = psizTile->cy;
	m_ctxMap = psizMap->cx / m_cxTile;
	m_ctyMap = psizMap->cy / m_cyTile;

	// Alloc the map and make it all "fogged" and Galaxite-free

	int cb = m_ctxMap * m_ctyMap;
	m_pbMap = new byte[cb];
	Assert(m_pbMap != NULL, "out of memory!");
	if (m_pbMap == NULL)
		return false;
	memset(m_pbMap, kbOpaque, cb);

	// Load the edges

	m_aptbm[15] = NULL;
	m_aptbm[14] = LoadTBitmap("fog0001.tbm");
	m_aptbm[13] = LoadTBitmap("fog0010.tbm");
	m_aptbm[12] = LoadTBitmap("fog0011.tbm");
	m_aptbm[11] = LoadTBitmap("fog0100.tbm");
	m_aptbm[10] = LoadTBitmap("fog0101.tbm");
	m_aptbm[9] = NULL;
	m_aptbm[8] = LoadTBitmap("fog0111.tbm");
	m_aptbm[7] = LoadTBitmap("fog1000.tbm");
	m_aptbm[6] = NULL;
	m_aptbm[5] = LoadTBitmap("fog1010.tbm");
	m_aptbm[4] = LoadTBitmap("fog1011.tbm");
	m_aptbm[3] = LoadTBitmap("fog1100.tbm");
	m_aptbm[2] = LoadTBitmap("fog1101.tbm");
	m_aptbm[1] = LoadTBitmap("fog1110.tbm");
	m_aptbm[0] = LoadTBitmap("fog1111.tbm");

	int c = 0;
	int n;
	for (n = 0; n < 16; n++) {
		if (m_aptbm[n] == NULL)
			c++;
	}
	if (c != 3)
		return false;
	
	m_aptbmGalax[0] = LoadTBitmap("galax1a.tbm");
	m_aptbmGalax[1] = LoadTBitmap("galax1b.tbm");
	m_aptbmGalax[2] = LoadTBitmap("galax1c.tbm");
	m_aptbmGalax[3] = LoadTBitmap("galax2a.tbm");
	m_aptbmGalax[4] = LoadTBitmap("galax2b.tbm");
	m_aptbmGalax[5] = LoadTBitmap("galax2c.tbm");
	m_aptbmGalax[6] = LoadTBitmap("galax3a.tbm");
	m_aptbmGalax[7] = LoadTBitmap("galax3b.tbm");
	m_aptbmGalax[8] = LoadTBitmap("galax3c.tbm");


	for (n = 0; n < 9; n++) {
		if (m_aptbmGalax[n] == NULL) {
			Assert("Failed to load one of the Galaxite bitmaps");
			return false;
		}
	}

	// Load the wall bitmaps

	m_panidWalls = LoadAnimationData("walls.anir");
	if (m_panidWalls == NULL) {
		Assert("Failed to load wall.anir");
		return false;
	}

	return true;
}

#define knVerFogMapState 5
bool FogMap::LoadState(Stream *pstm)
{
	// Note at this point the fogmap has already been "initialized" from
	// level constants so we just need to load in the contents of the map
	// which includes both fog and galaxite state

	byte nVer = pstm->ReadByte();
	if (nVer != knVerFogMapState)
		return false;
	pstm->ReadBytesRLE(m_pbMap, m_ctxMap * m_ctyMap);
	if (m_pbGxOriginal == NULL)
		m_pbGxOriginal = new byte[m_ctxMap * m_ctyMap];
	pstm->ReadBytesRLE(m_pbGxOriginal, m_ctxMap * m_ctyMap);
	m_tRegrowthLast = (long)pstm->ReadDword();
	return pstm->IsSuccess();
}

bool FogMap::SaveState(Stream *pstm)
{
	pstm->WriteByte(knVerFogMapState);
	pstm->WriteBytesRLE(m_pbMap, m_ctxMap * m_ctyMap);
	if (m_pbGxOriginal == NULL) {
		m_pbGxOriginal = new byte[m_ctxMap * m_ctyMap];
		memset(m_pbGxOriginal, 0, m_ctxMap * m_ctyMap);
	}
	pstm->WriteBytesRLE(m_pbGxOriginal, m_ctxMap * m_ctyMap);
	pstm->WriteDword((dword)m_tRegrowthLast);
	return pstm->IsSuccess();
}

// Galaxite regrowth
//
// Galaxite values come in three looks (1-5, 6-10, 11-15), each in five
// sizes. A mined-out tile only regrows where the level put Galaxite, only
// next to Galaxite that's still there (it spreads), and only back to 60% of
// the size it started with. One size step per tile per period.

void FogMap::CaptureGalaxiteOriginal(long tCurrent)
{
	int cb = m_ctxMap * m_ctyMap;
	if (m_pbGxOriginal == NULL)
		m_pbGxOriginal = new byte[cb];
	for (int i = 0; i < cb; i++)
		m_pbGxOriginal[i] = (m_pbMap[i] & kbfGalaxiteMask) >> kcGalaxiteShift;
	m_tRegrowthLast = tCurrent;
}

void FogMap::UpdateGalaxiteRegrowth(long tCurrent)
{
	if (m_nRegrowth == knGalaxiteRegrowthOff || m_pbGxOriginal == NULL)
		return;
	long ctPeriod = m_nRegrowth == knGalaxiteRegrowthSlow ? 200 * 100 : 100 * 100;
	if (tCurrent - m_tRegrowthLast < ctPeriod)
		return;
	m_tRegrowthLast = tCurrent;

	// Decide first, then grow, so Galaxite spreads one tile per period

	int cb = m_ctxMap * m_ctyMap;
	byte *pbGrow = new byte[cb];
	memset(pbGrow, 0, cb);
	TerrainMap *ptrmap = gsim.GetLevel()->GetTerrainMap();
	for (TCoord ty = 0; ty < m_ctyMap; ty++) {
		for (TCoord tx = 0; tx < m_ctxMap; tx++) {
			int off = ty * m_ctxMap + tx;
			int nOrig = m_pbGxOriginal[off];
			if (nOrig == 0)
				continue;
			int nBase = ((nOrig - 1) / 5) * 5 + 1;
			int cOrig = nOrig - nBase + 1;
			int cCap = (cOrig * 6 + 9) / 10;	// 60%, rounded up
			int nCur = (m_pbMap[off] & kbfGalaxiteMask) >> kcGalaxiteShift;
			int cCur = nCur == 0 ? 0 : nCur - nBase + 1;
			if (cCur >= cCap)
				continue;

			// Not under a building

			if (ptrmap->TestFlags(tx, ty, 1, 1, kbfStructure))
				continue;

			// A mined-out tile needs Galaxite next to it

			if (cCur == 0) {
				bool fNeighbor = false;
				for (int dy = -1; dy <= 1 && !fNeighbor; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						if ((dx != 0 || dy != 0) && GetGalaxite(tx + dx, ty + dy) != 0) {
							fNeighbor = true;
							break;
						}
					}
				}
				if (!fNeighbor)
					continue;
			}
			pbGrow[off] = (byte)(cCur == 0 ? nBase : nCur + 1);
		}
	}

	for (TCoord ty = 0; ty < m_ctyMap; ty++) {
		for (TCoord tx = 0; tx < m_ctxMap; tx++) {
			int off = ty * m_ctxMap + tx;
			if (pbGrow[off] == 0)
				continue;
			SetGalaxite(pbGrow[off], tx, ty);
			if (gptrcMapOpaque == NULL || !gptrcMapOpaque->PtIn(tx, ty))
				gpupdSim->InvalidateTile(tx, ty);
			if (gpmm != NULL)
				gpmm->RedrawTile(tx, ty);
		}
	}
	delete[] pbGrow;
}

        
void FogMap::RevealAll(UpdateMap *pupd)
{
	byte *pbT = m_pbMap;
	byte *pbEnd = m_pbMap + (m_ctxMap * m_ctyMap);
	while (pbT < pbEnd) {
//		*pbT++ = (*pbT & ~kbfFogMask) | kbEmpty;
		// BUGBUG: if we post-increment pbT as above the CodeWarrior ARM compiler
		// screws it up such that the RHS pbT is one byte ahead of the LHS pbT,
		// the end result being that all the Galaxite in the map gets shifted one
		// tile to the left!

		*pbT = (*pbT & ~kbfFogMask) | kbEmpty;
		pbT++;
	}

	// Invalidate the whole update map

	pupd->InvalidateRect();

	// Redraw the whole minimap

	gpmm->Redraw();
}

void FogMap::Reveal(TRect *ptrc, UpdateMap *pupd, WCoord wxView, WCoord wyView) 
{
	int ctxT = ptrc->Width();

	// Top row and corners

	byte *pbDst = &m_pbMap[(ptrc->top * m_ctxMap) + ptrc->left];
	int ctx = ctxT - 2;
	*pbDst = *pbDst | (15 - 10);
	pbDst++;
	while (ctx-- > 0) {
//		*pbDst++ = *pbDst | (15 - 2);
		// BUGBUG: if we post-increment pbDst as above the CodeWarrior ARM compiler
		// screws it up such that the RHS pbDst is one byte ahead of the LHS pbDst,
		// the end result being that defog pattern isn't applied properly.
		*pbDst = *pbDst | (15 - 2);
		pbDst++;
	}
	*pbDst = *pbDst | (15 - 3);

	// Bottom row and corners

	pbDst = &m_pbMap[((ptrc->bottom - 1) * m_ctxMap) + ptrc->left];
	ctx = ctxT - 2;
	*pbDst = *pbDst | (15 - 12);
	pbDst++;
	while (ctx-- > 0) {
//		*pbDst++ = *pbDst | (15 - 4);
		// BUGBUG: if we post-increment pbDst as above the CodeWarrior ARM compiler
		// screws it up such that the RHS pbDst is one byte ahead of the LHS pbDst,
		// the end result being that defog pattern isn't applied properly.
		*pbDst = *pbDst | (15 - 4);
		pbDst++;
	}
	*pbDst = *pbDst | (15 - 5);

	// Left column

	pbDst = &m_pbMap[((ptrc->top + 1) * m_ctxMap) + ptrc->left];
	ctx = ptrc->Height() - 2;
	while (ctx-- > 0) {
//		*pbDst++ = *pbDst | (15 - 8);
		// BUGBUG: if we post-increment pbDst as above the CodeWarrior ARM compiler
		// screws it up such that the RHS pbDst is one byte ahead of the LHS pbDst,
		// the end result being that defog pattern isn't applied properly.
		*pbDst = *pbDst | (15 - 8);
		pbDst += m_ctxMap;
	}

	// Right column

	pbDst = &m_pbMap[((ptrc->top + 1) * m_ctxMap) + ptrc->right - 1];
	ctx = ptrc->Height() - 2;
	while (ctx-- > 0) {
//		*pbDst++ = *pbDst | (15 - 1);
		// BUGBUG: if we post-increment pbDst as above the CodeWarrior ARM compiler
		// screws it up such that the RHS pbDst is one byte ahead of the LHS pbDst,
		// the end result being that defog pattern isn't applied properly.
		*pbDst = *pbDst | (15 - 1);
		pbDst += m_ctxMap;
	}

	// Interior

	TRect trcT;
	trcT = *ptrc;
	trcT.Inflate(-1, -1);
	ctxT = trcT.Width();

	pbDst = &m_pbMap[(trcT.top * m_ctxMap) + trcT.left];
	int cty = trcT.Height();
	while (cty-- > 0) {
		ctx = ctxT;
		while (ctx-- > 0) {
			*pbDst = *pbDst | kbEmpty;
			pbDst++;
		}
		pbDst += m_ctxMap - ctxT;
	}

	// Invalidate

	gpmm->RedrawTRect(ptrc);
	Rect rcT;
	rcT.FromTileRect(ptrc);
	int xView = PcFromWc(wxView) & 0xfffe;
	int yView = PcFromWc(wyView) & 0xfffe;
	rcT.Offset(-xView, -yView);
	pupd->InvalidateRect(&rcT);
}

void FogMap::Reveal(TCoord txMap, TCoord tyMap, RevealPattern *prvlp, UpdateMap *pupd, WCoord wxView, WCoord wyView)
{
	Assert(txMap >= 0 && txMap < m_ctxMap && tyMap >= 0 && tyMap < m_ctyMap);

	// RevealPattern is centered around txMap and tyMap

	int ctx = prvlp->ctx;
	int cty = prvlp->cty;
	int tx = txMap - ctx / 2;
	int ty = tyMap - cty / 2;

	// Clip to map edges

	int txPattern = 0;
	int tyPattern = 0;

	if (tx < 0) {
		ctx = tx + ctx;
		txPattern = -tx;
		tx = 0;
	}
	if (ty < 0) {
		cty = ty + cty;
		tyPattern = -ty;
		ty = 0;
	}
	if (tx + ctx > m_ctxMap)
		ctx = m_ctxMap - tx;
	if (ty + cty > m_ctyMap)
		cty = m_ctyMap - ty;

	Assert(ctx > 0 && ctx <= prvlp->ctx && cty > 0 && cty <= prvlp->cty);

	// Reveal

	int cbNextSrc = prvlp->ctx - ctx;
	int cbNextDst = m_ctxMap - ctx;
	byte *pbDst = &m_pbMap[ty * m_ctxMap + tx];
	byte *pbSrc = &prvlp->ab[tyPattern * prvlp->ctx + txPattern];
	bool fChanged = false;
	for (int tyT = ty; tyT < ty + cty; tyT++) {
		for (int txT = tx; txT < tx + ctx; txT++) {
			byte bDst = *pbDst;
			byte bDstNew = (*pbDst & ~kbfFogMask) | m_mpSrcDstResult[((*pbSrc) << 4) | (*pbDst & kbfFogMask)];
			if (bDst != bDstNew) {
				*pbDst = bDstNew;

				// Soft edges: the neighbours' look depends on this tile too

				for (int tyN = tyT - 1; tyN <= tyT + 1; tyN++) {
					for (int txN = txT - 1; txN <= txT + 1; txN++) {
						if (txN >= 0 && tyN >= 0 && txN < m_ctxMap && tyN < m_ctyMap)
							pupd->InvalidateTile(txN, tyN);
					}
				}
				gpmm->RedrawTile(txT, tyT);
			}
			pbDst++;
			pbSrc++;
		}
		pbSrc += cbNextSrc;
		pbDst += cbNextDst;
	}
}

// Soft fog edges. The edge tiles are 1 bit masks with hard, stepped
// outlines. A tile is drawn from the coverage of it and its 8 neighbours,
// blurred. On the true colour layer the coverage is black's alpha, a smooth
// fade; without it, steps of darkening (the palette's shadow map applied
// 1 to 7 times, then black) with an ordered dither between the steps. The
// soft tiles (coverage 0..255 per pixel) are cached by neighbourhood.

const int kcSoftCacheMax = 4096;
const int kcSoftLevels = 8;
const int kctEdgeFade = 2;		// tiles at the map's edge that get the edge fade
const int kbEdgeFadeMax = 215;	// its darkness at the edge (255: black)	// 0: clear, 1..7: shadow map applied that many times, 8: black

static const byte s_abBayer4[16] = {
	0, 8, 2, 10,
	12, 4, 14, 6,
	3, 11, 1, 9,
	15, 7, 13, 5
};

void FogMap::InitSoftEdges()
{
	m_fSoftInit = true;
	m_pvSoftCache = new SoftCache;
	int cx = m_cxTile;
	int cy = m_cyTile;
	DibBitmap *pbmT = CreateDibBitmap(NULL, cx, cy);
	if (pbmT == NULL)
		return;
	Color clrBlack = GetColor(kiclrBlack);
	Color clrSentinel = (Color)((clrBlack + 1) & 0xff);
	for (int n = 0; n < 16; n++) {
		int *pnCov = new int[cx * cy];
		m_apnCoverage[n] = pnCov;
		if (n == kbOpaque || n == kbEmpty || m_aptbm[n] == NULL) {
			for (int i = 0; i < cx * cy; i++)
				pnCov[i] = n == kbOpaque ? 256 : 0;
			continue;
		}
		pbmT->Fill(0, 0, cx, cy, clrSentinel);
		m_aptbm[n]->BltTo(pbmT, 0, 0);
		byte *pb = pbmT->GetBits();
		int cbRow = pbmT->GetRowBytes();
		for (int y = 0; y < cy; y++) {
			for (int x = 0; x < cx; x++) {
				byte b = pb[y * cbRow + x];
				pnCov[y * cx + x] = b == clrBlack ? 256 : (b == clrSentinel ? 0 : 128);
			}
		}
	}
	delete pbmT;
}

// Off the map, the fog is the map's fog mirrored across the edge tile (the
// tile past the edge looks like the tile before it). Revealed ground shows
// right up to the edge, and fog near the edge fades the same as anywhere
// else, instead of running into the edge as a solid block.

static int MirrorTc(int tc, int ctc)
{
	if (tc < 0)
		tc = -tc;
	if (tc >= ctc)
		tc = 2 * (ctc - 1) - tc;
	return tc < 0 ? 0 : (tc >= ctc ? ctc - 1 : tc);
}

byte FogMap::GetFogAt(int tx, int ty)
{
	tx = MirrorTc(tx, m_ctxMap);
	ty = MirrorTc(ty, m_ctyMap);
	return m_pbMap[ty * m_ctxMap + tx] & kbfFogMask;
}

// Which sides of a tile are the map's edge (kfSoftEdge*)

#define kfSoftEdgeLeft 1
#define kfSoftEdgeTop 2
#define kfSoftEdgeRight 4
#define kfSoftEdgeBottom 8

byte *FogMap::GetSoftTile(byte *abNeighbours, int ffEdges)
{
	unsigned long long key = ffEdges;
	for (int i = 0; i < 9; i++)
		key = (key << 5) | abNeighbours[i];
	SoftCache *pcache = (SoftCache *)m_pvSoftCache;
	SoftCache::iterator it = pcache->find(key);
	if (it != pcache->end())
		return it->second;
	if ((int)pcache->size() >= kcSoftCacheMax) {
		for (it = pcache->begin(); it != pcache->end(); ++it)
			delete[] it->second;
		pcache->clear();
	}

	// Coverage of the 3x3 tiles, blurred (box blur, twice each way; its
	// reach, 2 * nRadius, stays within one tile)

	int cx = m_cxTile;
	int cy = m_cyTile;
	int cxBig = cx * 3;
	int cyBig = cy * 3;
	int *pnCov = new int[cxBig * cyBig];
	int *pnTmp = new int[cxBig * cyBig];
	for (int ty = 0; ty < 3; ty++) {
		for (int tx = 0; tx < 3; tx++) {
			byte bN = abNeighbours[ty * 3 + tx];
			int *pnSrc = m_apnCoverage[bN];
			for (int y = 0; y < cy; y++)
				memcpy(&pnCov[(ty * cy + y) * cxBig + tx * cx], &pnSrc[y * cx], cx * sizeof(int));
		}
	}

	// Past the map's edge: mirror the coverage across the middle of this
	// tile, pixel by pixel (the neighbour slots past the edge hold the
	// mirrored tiles' fog, unflipped)

	if (ffEdges & kfSoftEdgeLeft) {
		for (int y = 0; y < cyBig; y++)
			for (int x = 0; x < cx; x++)
				pnCov[y * cxBig + x] = pnCov[y * cxBig + (3 * cx - 1 - x)];
	}
	if (ffEdges & kfSoftEdgeRight) {
		for (int y = 0; y < cyBig; y++)
			for (int x = 2 * cx; x < cxBig; x++)
				pnCov[y * cxBig + x] = pnCov[y * cxBig + (3 * cx - 1 - x)];
	}
	if (ffEdges & kfSoftEdgeTop) {
		for (int y = 0; y < cy; y++)
			memcpy(&pnCov[y * cxBig], &pnCov[(3 * cy - 1 - y) * cxBig], cxBig * sizeof(int));
	}
	if (ffEdges & kfSoftEdgeBottom) {
		for (int y = 2 * cy; y < cyBig; y++)
			memcpy(&pnCov[y * cxBig], &pnCov[(3 * cy - 1 - y) * cxBig], cxBig * sizeof(int));
	}
	int nRadius = cx / 3;
	if (nRadius < 1)
		nRadius = 1;
	int cSpan = 2 * nRadius + 1;
	for (int nPass = 0; nPass < 2; nPass++) {
		for (int y = 0; y < cyBig; y++) {
			for (int x = 0; x < cxBig; x++) {
				int nSum = 0;
				for (int d = -nRadius; d <= nRadius; d++) {
					int xT = x + d;
					xT = xT < 0 ? 0 : (xT >= cxBig ? cxBig - 1 : xT);
					nSum += pnCov[y * cxBig + xT];
				}
				pnTmp[y * cxBig + x] = nSum / cSpan;
			}
		}
		for (int y = 0; y < cyBig; y++) {
			for (int x = 0; x < cxBig; x++) {
				int nSum = 0;
				for (int d = -nRadius; d <= nRadius; d++) {
					int yT = y + d;
					yT = yT < 0 ? 0 : (yT >= cyBig ? cyBig - 1 : yT);
					nSum += pnTmp[yT * cxBig + x];
				}
				pnCov[y * cxBig + x] = nSum / cSpan;
			}
		}
	}

	// Coverage of the middle tile

	byte *pbSoft = new byte[cx * cy];
	for (int y = 0; y < cy; y++) {
		for (int x = 0; x < cx; x++) {
			int nCov = pnCov[(cy + y) * cxBig + cx + x];

			if (nCov > 256)
				nCov = 256;

			// Ease the ends so the fade meets clear ground and black
			// without a visible seam (smoothstep)

			int n = nCov;
			n = (n * n * (3 * 256 - 2 * n)) / (256 * 256);
			if (nCov <= 2)
				n = 0;
			if (nCov >= 254)
				n = 255;
			pbSoft[y * cx + x] = (byte)(n > 255 ? 255 : n);
		}
	}
	delete[] pnCov;
	delete[] pnTmp;
	(*pcache)[key] = pbSoft;
	return pbSoft;
}

void FogMap::DrawSoftEdge(DibBitmap *pbm, byte *pbSoft, int x, int y)
{
	Size siz;
	pbm->GetSize(&siz);
	int xL = x < 0 ? -x : 0;
	int yT = y < 0 ? -y : 0;
	int xR = x + m_cxTile > siz.cx ? siz.cx - x : m_cxTile;
	int yB = y + m_cyTile > siz.cy ? siz.cy - y : m_cyTile;
	if (xL >= xR || yT >= yB)
		return;
	byte *mpShadow = gmpiclriclrShadow;
	byte bBlack = (byte)GetColor(kiclrBlack);
	int cbRow = pbm->GetRowBytes();
	bool fTrueColor = pbm->GetTrueColor() != NULL;
	for (int yy = yT; yy < yB; yy++) {
		byte *pbDst = pbm->GetBits() + (long)(y + yy) * cbRow + x;
		byte *pbSrc = pbSoft + yy * m_cxTile;
		for (int xx = xL; xx < xR; xx++) {
			int nCov = pbSrc[xx];
			if (nCov == 0)
				continue;
			if (nCov == 255) {
				pbDst[xx] = bBlack;
				continue;
			}
			if (fTrueColor) {
				pbm->SetTrueColor(x + xx, y + yy, (dword)nCov << 24);
				continue;
			}
			int nLevel = (nCov * kcSoftLevels * 16 + s_abBayer4[(yy & 3) * 4 + (xx & 3)] * 255) / (255 * 16);
			if (nLevel == 0)
				continue;
			if (nLevel >= kcSoftLevels) {
				pbDst[xx] = bBlack;
				continue;
			}
			byte b = pbDst[xx];
			while (nLevel-- > 0)
				b = mpShadow[b];
			pbDst[xx] = b;
		}
	}
}

void FogMap::Draw(DibBitmap *pbm, int xMap, int yMap, UpdateMap *pupd)
{
	if (!m_fSoftInit)
		InitSoftEdges();

	Size siz;
	pbm->GetSize(&siz);
	Rect rcDib;
	rcDib.left = 0;
	rcDib.top = 0;
	rcDib.right = siz.cx;
	rcDib.bottom = siz.cy;

	int tx = xMap / gcxTile;
	int ty = yMap / gcyTile;
	int ctx = (siz.cx + (gcxTile - 1)) / gcxTile + 1;
	if (tx + ctx > m_ctxMap)
		ctx = m_ctxMap - tx;
	int cty = (siz.cy + (gcyTile - 1)) / gcyTile + 1;
	if (ty + cty > m_ctyMap)
		cty = m_ctyMap - ty;
	Color clrBlack = GetColor(kiclrBlack);

	bool *pfInvalid = pupd->GetInvalidMap();
	Size sizMap;
	pupd->GetMapSize(&sizMap);
	int cfInvalidNextScan = sizMap.cx - ctx;
	Assert(sizMap.cx >= ctx && sizMap.cy >= cty);

	byte *pbDib = pbm->GetBits();
	byte *pbMapT = &m_pbMap[ty * m_ctxMap + tx];
	int cbNextScan = m_ctxMap - ctx;
	int xTile = tx * gcxTile - xMap;
	int yTile = ty * gcyTile - yMap;
	int xTileStart = xTile;
	byte abNeighbours[9];
	for (int tyT = ty; tyT < ty + cty; tyT++) {
		int cEmpty = 0;
		int xStart;
		for (int txT = tx; txT < tx + ctx; txT++) {
			byte bFog = *pbMapT++ & kbfFogMask;
			if (*pfInvalid++ == false) {
				bFog = kbEmpty;
			} else {
				// Fog and its 8 neighbours. All the same: black or
				// nothing; otherwise a soft tile.

				bool fUniform = true;
				for (int i = 0; i < 9; i++) {
					abNeighbours[i] = GetFogAt(txT + i % 3 - 1, tyT + i / 3 - 1);
					if (abNeighbours[i] != bFog)
						fUniform = false;
				}
				if (!fUniform && gmpiclriclrShadow != NULL) {
					if (cEmpty != 0) {
						pbm->Fill(xStart, yTile, cEmpty * gcxTile, gcyTile, clrBlack);
						cEmpty = 0;
					}
					int ffEdges = (txT == 0 ? kfSoftEdgeLeft : 0) | (tyT == 0 ? kfSoftEdgeTop : 0) |
							(txT == m_ctxMap - 1 ? kfSoftEdgeRight : 0) | (tyT == m_ctyMap - 1 ? kfSoftEdgeBottom : 0);
					DrawSoftEdge(pbm, GetSoftTile(abNeighbours, ffEdges), xTile, yTile);
					xTile += gcxTile;
					continue;
				}
			}

			// Opaque?

			if (bFog == kbOpaque) {
				if (cEmpty == 0)
					xStart = xTile;
				cEmpty++;
				xTile += gcxTile;
				continue;
			} 

			// Not opaque; fill block if there is one

			if (cEmpty != 0) {
				pbm->Fill(xStart, yTile, cEmpty * gcxTile, gcyTile, clrBlack);
				cEmpty = 0;
			}

			// Not opaque; draw unique tile

			TBitmap *ptbm = m_aptbm[bFog];
			if (ptbm == NULL) {
				xTile += gcxTile;
				continue;
			}

			ptbm->BltTo(pbm, xTile, yTile);
			xTile += gcxTile;
		}
		if (cEmpty != 0) {
			pbm->Fill(xStart, yTile, cEmpty * gcxTile, gcyTile, clrBlack);
			cEmpty = 0;

		}

		pbMapT += cbNextScan;
		pfInvalid += cfInvalidNextScan;
		yTile += gcxTile;
		xTile = xTileStart;
	}

	// The map's edge fades out like fog (redrawn with the tiles it covers)

	pfInvalid = pupd->GetInvalidMap();
	yTile = ty * gcyTile - yMap;
	for (int tyT = ty; tyT < ty + cty; tyT++, yTile += gcyTile) {
		bool *pfRow = pfInvalid + (tyT - ty) * sizMap.cx;
		bool fEdgeRow = tyT < kctEdgeFade || tyT >= m_ctyMap - kctEdgeFade;
		xTile = xTileStart;
		for (int txT = tx; txT < tx + ctx; txT++, xTile += gcxTile) {
			if (!pfRow[txT - tx])
				continue;
			if (fEdgeRow || txT < kctEdgeFade || txT >= m_ctxMap - kctEdgeFade)
				DrawEdgeFade(pbm, txT, tyT, xTile, yTile);
		}
	}
}

// Fade at the map's edge: dark like fog right at the edge (never solid
// black), clearing over a tile and a half, the same all along the edge.

void FogMap::DrawEdgeFade(DibBitmap *pbm, int txT, int tyT, int x, int y)
{
	Size siz;
	pbm->GetSize(&siz);
	int xL = x < 0 ? -x : 0;
	int yT = y < 0 ? -y : 0;
	int xR = x + m_cxTile > siz.cx ? siz.cx - x : m_cxTile;
	int yB = y + m_cyTile > siz.cy ? siz.cy - y : m_cyTile;
	if (xL >= xR || yT >= yB)
		return;

	int cpBand = m_cxTile * 3 / 2;
	int cxMap = m_ctxMap * m_cxTile;
	int cyMap = m_ctyMap * m_cyTile;
	int cbRow = pbm->GetRowBytes();
	byte *pbBits = pbm->GetBits();
	dword *pdwTC = pbm->GetTrueColor();
	byte *mpShadow = gmpiclriclrShadow;
	for (int yy = yT; yy < yB; yy++) {
		int my = tyT * m_cyTile + yy;
		int dy = my < cyMap - 1 - my ? my : cyMap - 1 - my;
		int nY = dy >= cpBand ? 0 : EdgeFadeAlpha(dy, cpBand);
		for (int xx = xL; xx < xR; xx++) {
			int mx = txT * m_cxTile + xx;
			int dx = mx < cxMap - 1 - mx ? mx : cxMap - 1 - mx;
			int nX = dx >= cpBand ? 0 : EdgeFadeAlpha(dx, cpBand);
			if (nX == 0 && nY == 0)
				continue;

			// Both edges near a corner: combine as two layers of shade

			int nA = 255 - (255 - nX) * (255 - nY) / 255;
			long i = (long)(y + yy) * cbRow + x + xx;
			if (pdwTC != NULL) {
				// Over fog already laid here: the darker of the two

				int nAOld = pdwTC[i] >> 24;
				if (nAOld != 0 && pbm->GetTrueColorUnder()[i] == pbBits[i]) {
					if (nAOld >= nA)
						continue;
				}
				pbm->SetTrueColor(x + xx, y + yy, (dword)nA << 24);
			} else if (mpShadow != NULL) {
				int nLevel = (nA * kcSoftLevels * 16 + s_abBayer4[(yy & 3) * 4 + (xx & 3)] * 255) / (255 * 16);
				byte b = pbBits[i];
				while (nLevel-- > 0)
					b = mpShadow[b];
				pbBits[i] = b;
			}
		}
	}
}

// Shade at a distance from the map's edge (0 at the edge pixel)

int FogMap::EdgeFadeAlpha(int d, int cpBand)
{
	int n = 256 - d * 256 / cpBand;		// 256 at the edge .. 0
	n = (n * n * (3 * 256 - 2 * n)) / (256 * 256);	// smoothstep
	return n * kbEdgeFadeMax / 256;
}

byte s_abGxTranslate[] = { 0, 2, 1, 1, 0, 0, 5, 4, 4, 3, 3, 8, 7, 7, 6, 6 };
byte s_abGxInc[] = { 0, 2, 3, 4, 5, 5, 7, 8, 9, 10, 10, 12, 13, 14, 15, 15 };
byte s_abGxDec[] = { 0, 0, 1, 2, 3, 4, 0, 6, 7, 8, 9, 0, 11, 12, 13, 14 };

void FogMap::DrawGalaxite(DibBitmap *pbm, int xMap, int yMap, UpdateMap *pupd, byte *pbTrMap)
{
	Size siz;
	pbm->GetSize(&siz);
	Rect rcDib;
	rcDib.left = 0;
	rcDib.top = 0;
	rcDib.right = siz.cx;
	rcDib.bottom = siz.cy;

	int tx = xMap / gcxTile;
	int ty = yMap / gcyTile;
	int ctx = (siz.cx + (gcxTile - 1)) / gcxTile + 1;
	if (tx + ctx > m_ctxMap)
		ctx = m_ctxMap - tx;
	int cty = (siz.cy + (gcyTile - 1)) / gcyTile + 1;
	if (ty + cty > m_ctyMap)
		cty = m_ctyMap - ty;
	byte *pbDib = pbm->GetBits();
	byte *pbMapT = &m_pbMap[ty * m_ctxMap + tx];
	int cbNextScan = m_ctxMap - ctx;
	int xTile = tx * gcxTile - xMap;
	int yTile = ty * gcyTile - yMap;
	int xTileStart = xTile;

	bool *pfInvalid = pupd->GetInvalidMap();
	Size sizMap;
	pupd->GetMapSize(&sizMap);
	int cfInvalidNextScan = sizMap.cx - ctx;
	Assert(sizMap.cx >= ctx && sizMap.cy >= cty);
	int nServiceSfx = 0;

	for (int tyT = ty; tyT < ty + cty; tyT++) {
		for (int txT = tx; txT < tx + ctx; txT++, pbMapT++) {
			byte b = *pbMapT;
			if (*pfInvalid++ == false) {
				xTile += gcxTile;
				continue;
			}

			// Is no Galaxite/Wall present or is this tile completely obscured by fog?

			byte *ptt = &pbTrMap[pbMapT - m_pbMap];
			if (((b & kbfGalaxiteMask) == 0 && *ptt != kttWall) || IsFogOpaque(b)) {
				xTile += gcxTile;
				continue;
			}

			// Galaxite?

			if (HasGalaxite(b)) {
				// Galaxite

				TBitmap *ptbm = m_aptbmGalax[s_abGxTranslate[(b & kbfGalaxiteMask) >> kcGalaxiteShift]];
				ptbm->BltTo(pbm, xTile, yTile);


			} 

			// Wall?

			if (*ptt == kttWall) {

#define HasWall(tt) ((tt) == kttWall)

				// Check neighbor cells for walls to decide what shape this
				// segment should take.

				int ifrm = 0;
				if (tyT > 0) {
					if (HasWall(*(ptt - m_ctxMap)))
						ifrm |= 1;
				}
				if (txT < m_ctxMap - 1) {
					if (HasWall(*(ptt + 1)))
						ifrm |= 2;
				}
				if (tyT < m_ctyMap - 1) {
					if (HasWall(*(ptt + m_ctxMap)))
						ifrm |= 4;
				}
				if (txT > 0) {
					if (HasWall(*(ptt - 1)))
						ifrm |= 8;
				}
			
				// The kwfWallMask >> 1 test makes walls with less than half their health
				// draw from the 'damaged' strip.

//				m_panidWalls->DrawFrame(w <= (kwfWallMask >> 1) ? 1 : 0, ifrm, pbm, xTile, yTile, ksideNeutral);
				m_panidWalls->DrawFrame(0, ifrm, pbm, xTile, yTile, ksideNeutral);

			}

			xTile += gcxTile;
		}

		pfInvalid += cfInvalidNextScan;
		pbMapT += cbNextScan;
		yTile += gcxTile;
		xTile = xTileStart;
	}
}

void FogMap::SetGalaxite(int nGx, TCoord tx, TCoord ty)
{
	Assert(tx >= 0 && ty >= 0 && tx < m_ctxMap && ty < m_ctyMap);

	byte *pb = &m_pbMap[(ty * m_ctxMap) + tx];
	*pb = (*pb & ~kbfGalaxiteMask) | (nGx << kcGalaxiteShift);
}

int FogMap::GetGalaxite(TCoord tx, TCoord ty)
{
	// Any attempt to look for Galaxite off the edge of the map returns none

	if (tx < 0 || ty < 0 || tx >= m_ctxMap || ty >= m_ctyMap)
		return 0;

	byte *pb = &m_pbMap[(ty * m_ctxMap) + tx];
	int nGx = (*pb & kbfGalaxiteMask) >> kcGalaxiteShift;


	return nGx;
}

bool FogMap::DecGalaxite(TCoord tx, TCoord ty)
{
	Assert(tx >= 0 && ty >= 0 && tx < m_ctxMap && ty < m_ctyMap);

	byte *pb = &m_pbMap[(ty * m_ctxMap) + tx];
	byte bNew = s_abGxDec[(*pb & kbfGalaxiteMask) >> kcGalaxiteShift] << kcGalaxiteShift;
	*pb = (*pb & ~kbfGalaxiteMask) | bNew;

	// Cause this tile to redraw

	if (gptrcMapOpaque == NULL || !gptrcMapOpaque->PtIn(tx, ty))
		gpupdSim->InvalidateTile(tx, ty);
	if (gpmm != NULL)
		gpmm->RedrawTile(tx, ty);

	return bNew != 0;
}

bool FogMap::FindNearestGalaxite(TCoord txOrigin, TCoord tyOrigin, TPoint *ptpt, bool fIgnoreFog)
{
	Assert(txOrigin >= 0 && tyOrigin >= 0 && txOrigin < m_ctxMap && tyOrigin < m_ctyMap);

	TPoint *atptGx = (TPoint *)gpbScratch;
	TPoint *ptptGx = atptGx;
	int ctptGx;

	int nMax1 = _max((int)txOrigin, m_ctxMap - txOrigin);
	int nMax2 = _max((int)tyOrigin, m_ctyMap - tyOrigin);
	int nRadiusMax = _max(nMax1, nMax2);

	for (int nRadius = 1; nRadius < nRadiusMax; nRadius++) {

		// We treat Galaxite directly under the Miner as being
		// the equivalent of 2.5 tiles away.

		if (nRadius == 3) {
			byte *pb = &m_pbMap[(tyOrigin * m_ctxMap) + txOrigin];
			if (HasGalaxite(*pb)) {
				ptpt->tx = txOrigin;
				ptpt->ty = tyOrigin;
//				MpTrace("  -- found %d, %d", txOrigin, tyOrigin);
				return true;
			}
		}

		// look for galaxite across the top

		TCoord txMin = txOrigin - nRadius;
		if (txMin < 0)
			txMin = 0;
		TCoord txMax = txOrigin + nRadius;
		if (txMax > m_ctxMap)
			txMax = m_ctxMap;

		TCoord tx, ty;
		ty = tyOrigin - nRadius;
		if (ty >= 0) {
			byte *pb = &m_pbMap[(ty * m_ctxMap) + txMin];
			for (tx = txMin; tx < txMax; tx++, pb++) {
				if (!HasGalaxite(*pb))	// Galaxite?
					continue;			// no
				if (IsFogOpaque(*pb) && !fIgnoreFog)	// Fogged?
					continue;			// yes
//				MpTrace(" t %d, %d, 0x%lx", tx, ty, ptptGx);
				ptptGx->tx = tx;
				ptptGx->ty = ty;
				ptptGx++;
			}
		}

		// look for galaxite across the bottom

		txMax += 1;
		if (txMax > m_ctxMap)
			txMax = m_ctxMap;
		txMin = txOrigin - nRadius + 1;
		if (txMin < 0)
			txMin = 0;

		ty = tyOrigin + nRadius;
		if (ty < m_ctyMap) {
			byte *pb = &m_pbMap[(ty * m_ctxMap) + txMin];
			for (tx = txMin; tx < txMax; tx++, pb++) {
				if (!HasGalaxite(*pb))
					continue;
				if (IsFogOpaque(*pb) && !fIgnoreFog)
					continue;
//				MpTrace(" b %d, %d, 0x%lx", tx, ty, ptptGx);
				ptptGx->tx = tx;
				ptptGx->ty = ty;
				ptptGx++;
			}
		}

		// look for galaxite down the right side

		TCoord tyMin = tyOrigin - nRadius;
		if (tyMin < 0)
			tyMin = 0;
		TCoord tyMax = tyOrigin + nRadius;
		if (tyMax > m_ctyMap)
			tyMax = m_ctyMap;

		tx = txOrigin + nRadius;
		if (tx < m_ctxMap) {
			byte *pb = &m_pbMap[(tyMin * m_ctxMap) + tx];
			for (ty = tyMin; ty < tyMax; ty++, pb += m_ctxMap) {
				if (!HasGalaxite(*pb))
					continue;
				if (IsFogOpaque(*pb) && !fIgnoreFog)
					continue;
//				MpTrace(" r %d, %d, 0x%lx", tx, ty, ptptGx);
				ptptGx->tx = tx;
				ptptGx->ty = ty;
				ptptGx++;
			}
		}

		// look for galaxite down the left side

		tyMin = tyOrigin - nRadius + 1;
		if (tyMin < 0)
			tyMin = 0;
		tyMax += 1;
		if (tyMax > m_ctyMap)
			tyMax = m_ctyMap;

		tx = txOrigin - nRadius;
		if (tx >= 0) {
			byte *pb = &m_pbMap[(tyMin * m_ctxMap) + tx];
			for (ty = tyMin; ty < tyMax; ty++, pb += m_ctxMap) {
				if (!HasGalaxite(*pb))
					continue;
				if (IsFogOpaque(*pb) && !fIgnoreFog)
					continue;
//				MpTrace(" l %d, %d, 0x%lx", tx, ty, ptptGx);
				ptptGx->tx = tx;
				ptptGx->ty = ty;
				ptptGx++;
			}
		}

		// If Galaxite has been found at this radius randomly pick one
		// of the found locations and return it.

		if (ptptGx != atptGx) {
			ctptGx = (int)(ptptGx - atptGx);
			int i = GetRandom() % ctptGx;
			*ptpt = atptGx[i];
//			MpTrace("  -- found %d, %d, 0x%lx [%d of %d]", ptpt->tx, ptpt->ty, &atptGx[i], i, ctptGx);
			return true;
		}
	}
	
//	MpTrace("  -- found none");
	return false;
}

//
// Wall methods
//


} // namespace wi
