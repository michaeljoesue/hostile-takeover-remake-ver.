#include "../ht.h"
#include <cmath>
#include <string>
#include <vector>

// Text overlay: see class TextOverlay in game/ht.h.
//
// The game frame is drawn at the game resolution and scaled up to the window.
// Text drawn with a vector font into one of the form managers' back buffers
// becomes an item here instead of pixels in the frame. An item is rendered
// with the window's real pixels and drawn on top of the scaled frame.
//
// Z order: when an item is added, it keeps a copy of the frame pixels under
// it. When drawing, it shows only where those pixels are still the same, so
// whatever was drawn over the text afterwards (a dialog, a list's highlight)
// still covers it.
//
// Lifetime: the game only redraws invalid areas. An item whose text area
// becomes invalid was either redrawn this frame (a new item) or erased, so
// older items there go away when the frame completes.

namespace wi {

struct OverlayItem
{
	Font *pfnt;
	DibBitmap *pbm;
	std::string str;
	int x, y;
	Rect rcInk;     // game pixels the text can touch (back buffer coords)
	Rect rcAnchor;  // the text line itself
	std::vector<byte> abUnder; // back buffer pixels under rcInk when added
	long nFrame;

	// Rendered texture
	SDL_Texture *ptex;
	float nScaleTex;
	int nPaletteSerial;
	int yTopTex;                  // where pbm was on screen
	int xOut, yOut, cxOut, cyOut; // in output pixels, relative to the frame
	std::vector<dword> argb;       // unmasked pixels
	std::vector<byte> abMask;      // last visibility mask, game pixels
	bool fMasked;
};

struct OverlayTarget
{
	DibBitmap *pbm;
	int yTop;
};

static std::vector<OverlayItem *> s_apitem;
static std::vector<OverlayTarget> s_atgt;
static long s_nFrame;

// Enough items for very text heavy screens; beyond this the oldest go

#define kcItemsMax 4000

static void FreeItem(OverlayItem *pitem)
{
	if (pitem->ptex != NULL)
		SDL_DestroyTexture(pitem->ptex);
	delete pitem;
}

bool TextOverlay::IsTarget(DibBitmap *pbm)
{
	for (size_t i = 0; i < s_atgt.size(); i++) {
		if (s_atgt[i].pbm == pbm)
			return true;
	}
	return false;
}

static bool CopyUnder(DibBitmap *pbm, Rect *prc, std::vector<byte> *pab)
{
	Size siz;
	pbm->GetSize(&siz);
	Rect rcDib;
	rcDib.Set(0, 0, siz.cx, siz.cy);
	Rect rc;
	if (!rc.Intersect(prc, &rcDib))
		return false;
	*prc = rc;
	int cbRow = pbm->GetRowBytes();
	byte *pbBits = pbm->GetBits();
	pab->resize(rc.Width() * rc.Height());
	for (int y = rc.top; y < rc.bottom; y++) {
		memcpy(&(*pab)[(y - rc.top) * rc.Width()],
				pbBits + (long)y * cbRow + rc.left, rc.Width());
	}
	return true;
}

void TextOverlay::AddText(Font *pfnt, DibBitmap *pbm, const char *psz,
		int cch, int x, int y)
{
	if (cch <= 0)
		return;
	OverlayItem *pitem = new OverlayItem;
	pitem->pfnt = pfnt;
	pitem->pbm = pbm;
	pitem->str.assign(psz, cch);
	pitem->x = x;
	pitem->y = y;
	pfnt->GetVectorBounds(psz, cch, x, y, &pitem->rcInk);
	int cx = pfnt->GetTextExtent(pitem->str.c_str());
	pitem->rcAnchor.Set(x, y, x + cx, y + pfnt->GetHeight());
	pitem->nFrame = s_nFrame;
	pitem->ptex = NULL;
	pitem->nScaleTex = 0;
	pitem->nPaletteSerial = -1;
	pitem->yTopTex = 0;
	pitem->xOut = pitem->yOut = pitem->cxOut = pitem->cyOut = 0;
	pitem->fMasked = false;
	if (pitem->rcInk.IsEmpty() ||
			!CopyUnder(pbm, &pitem->rcInk, &pitem->abUnder)) {
		// Nothing visible (blank text, or off the buffer)
		FreeItem(pitem);
		return;
	}

	// Text drawn again over the same line replaces it. (Lines of a
	// paragraph may overlap a little, so only mostly overlapping lines.)

	for (size_t i = 0; i < s_apitem.size(); ) {
		OverlayItem *pitemT = s_apitem[i];
		Rect rcT;
		if (pitemT->pbm == pbm &&
				rcT.Intersect(&pitemT->rcAnchor, &pitem->rcAnchor)) {
			int nArea = rcT.Width() * rcT.Height();
			int nAreaMin = pitemT->rcAnchor.Width() *
					pitemT->rcAnchor.Height();
			int nAreaNew = pitem->rcAnchor.Width() * pitem->rcAnchor.Height();
			if (nAreaNew < nAreaMin)
				nAreaMin = nAreaNew;
			if (nArea * 2 > nAreaMin) {
				FreeItem(pitemT);
				s_apitem.erase(s_apitem.begin() + i);
				continue;
			}
		}
		i++;
	}

	s_apitem.push_back(pitem);
	if (s_apitem.size() > kcItemsMax) {
		FreeItem(s_apitem[0]);
		s_apitem.erase(s_apitem.begin());
	}
}

void TextOverlay::BeginFrame(int cdib, DibBitmap **apbm, int *ayTop)
{
	s_nFrame++;
	s_atgt.clear();
	for (int n = 0; n < cdib; n++) {
		OverlayTarget tgt;
		tgt.pbm = apbm[n];
		tgt.yTop = ayTop[n];
		s_atgt.push_back(tgt);
	}

	// Items in buffers that are gone

	for (size_t i = 0; i < s_apitem.size(); ) {
		if (!IsTarget(s_apitem[i]->pbm)) {
			FreeItem(s_apitem[i]);
			s_apitem.erase(s_apitem.begin() + i);
			continue;
		}
		i++;
	}
}

void TextOverlay::EndFrame(int cdib, DibBitmap **apbm, UpdateMap **apupd)
{
	// Older text in areas repainted this frame is gone (or was redrawn as a
	// new item)

	for (int n = 0; n < cdib; n++) {
		UpdateMap *pupd = apupd[n];
		if (!pupd->IsInvalid())
			continue;
		for (size_t i = 0; i < s_apitem.size(); ) {
			OverlayItem *pitem = s_apitem[i];
			if (pitem->pbm == apbm[n] && pitem->nFrame != s_nFrame) {
				Rect rcT = pitem->rcAnchor;
				rcT.Inflate(-1, -1);
				if (!rcT.IsEmpty() && pupd->IsRectInvalid(&rcT)) {
					FreeItem(pitem);
					s_apitem.erase(s_apitem.begin() + i);
					continue;
				}
			}
			i++;
		}
	}
}

void TextOverlay::Reset()
{
	for (size_t i = 0; i < s_apitem.size(); i++)
		FreeItem(s_apitem[i]);
	s_apitem.clear();
	s_atgt.clear();
}

// Which game pixels under the item are unchanged since the text was added.
// Returns 0 if none, 1 if some, 2 if all.

static int CalcMask(OverlayItem *pitem, std::vector<byte> *pabMask)
{
	Rect &rc = pitem->rcInk;
	int cbRow = pitem->pbm->GetRowBytes();
	byte *pbBits = pitem->pbm->GetBits();
	int cx = rc.Width();
	int cy = rc.Height();
	pabMask->resize(cx * cy);
	int cSame = 0;
	for (int y = 0; y < cy; y++) {
		const byte *pbRow = pbBits + (long)(rc.top + y) * cbRow + rc.left;
		const byte *pbUnder = &pitem->abUnder[y * cx];
		byte *pbMask = &(*pabMask)[y * cx];
		for (int x = 0; x < cx; x++) {
			pbMask[x] = pbRow[x] == pbUnder[x];
			cSame += pbMask[x];
		}
	}
	if (cSame == 0)
		return 0;
	return cSame == cx * cy ? 2 : 1;
}

static bool BuildTexture(SDL_Renderer *prend, OverlayItem *pitem,
		float nScale, int yTop)
{
	// Frame coordinates (the buffer is yTop down the frame)

	Rect rc = pitem->rcInk;
	rc.Offset(0, yTop);
	int xOut = (int)floor(rc.left * nScale);
	int yOut = (int)floor(rc.top * nScale);
	int cxOut = (int)ceil(rc.right * nScale) - xOut;
	int cyOut = (int)ceil(rc.bottom * nScale) - yOut;
	if (cxOut <= 0 || cyOut <= 0)
		return false;
	pitem->argb.assign(cxOut * cyOut, 0);
	pitem->pfnt->RenderOverlay(pitem->str.c_str(), (int)pitem->str.size(),
			pitem->x, pitem->y + yTop, nScale, xOut, yOut, cxOut, cyOut,
			&pitem->argb[0]);
	if (pitem->ptex != NULL &&
			(pitem->cxOut != cxOut || pitem->cyOut != cyOut)) {
		SDL_DestroyTexture(pitem->ptex);
		pitem->ptex = NULL;
	}
	if (pitem->ptex == NULL) {
		pitem->ptex = SDL_CreateTexture(prend, SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_STATIC, cxOut, cyOut);
		if (pitem->ptex == NULL)
			return false;
		SDL_SetTextureBlendMode(pitem->ptex, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(pitem->ptex, SDL_SCALEMODE_NEAREST);
	}
	SDL_UpdateTexture(pitem->ptex, NULL, &pitem->argb[0], cxOut * 4);
	pitem->xOut = xOut;
	pitem->yOut = yOut;
	pitem->cxOut = cxOut;
	pitem->cyOut = cyOut;
	pitem->nScaleTex = nScale;
	pitem->yTopTex = yTop;
	pitem->nPaletteSerial = gpdisp->GetPaletteSerial();
	pitem->fMasked = false;
	pitem->abMask.clear();
	return true;
}

// Partly covered: hide the covered pixels

static void ApplyMask(OverlayItem *pitem, std::vector<byte> &abMask)
{
	if (pitem->fMasked && pitem->abMask == abMask)
		return;
	Rect &rc = pitem->rcInk;
	int cx = rc.Width();
	int cy = rc.Height();
	float nScale = pitem->nScaleTex;
	std::vector<dword> argb(pitem->argb.size());
	for (int oy = 0; oy < pitem->cyOut; oy++) {
		int y = (int)floor((oy + pitem->yOut + 0.5f) / nScale) -
				(rc.top + pitem->yTopTex);
		if (y < 0)
			y = 0;
		if (y >= cy)
			y = cy - 1;
		for (int ox = 0; ox < pitem->cxOut; ox++) {
			int x = (int)floor((ox + pitem->xOut + 0.5f) / nScale) - rc.left;
			if (x < 0)
				x = 0;
			if (x >= cx)
				x = cx - 1;
			int i = oy * pitem->cxOut + ox;
			argb[i] = abMask[y * cx + x] ? pitem->argb[i] : 0;
		}
	}
	SDL_UpdateTexture(pitem->ptex, NULL, &argb[0], pitem->cxOut * 4);
	pitem->abMask = abMask;
	pitem->fMasked = true;
}

static void RestoreUnmasked(OverlayItem *pitem)
{
	if (!pitem->fMasked)
		return;
	SDL_UpdateTexture(pitem->ptex, NULL, &pitem->argb[0], pitem->cxOut * 4);
	pitem->fMasked = false;
	pitem->abMask.clear();
}

void TextOverlay::Render(float nScale, float xFrame, float yFrame)
{
	SDL_Renderer *prend = gpdisp->GetRenderer();
	if (prend == NULL || s_apitem.empty())
		return;

	// Draw in window pixels

	int cxLogical, cyLogical;
	SDL_RendererLogicalPresentation mode;
	SDL_GetRenderLogicalPresentation(prend, &cxLogical, &cyLogical, &mode);
	SDL_SetRenderLogicalPresentation(prend, 0, 0,
			SDL_LOGICAL_PRESENTATION_DISABLED);

	int nSerial = gpdisp->GetPaletteSerial();
	std::vector<byte> abMask;
	for (size_t i = 0; i < s_apitem.size(); ) {
		OverlayItem *pitem = s_apitem[i];
		int yTop = 0;
		for (size_t n = 0; n < s_atgt.size(); n++) {
			if (s_atgt[n].pbm == pitem->pbm)
				yTop = s_atgt[n].yTop;
		}

		int nVisible = CalcMask(pitem, &abMask);
		if (nVisible == 0) {
			// Covered completely: it won't come back
			FreeItem(pitem);
			s_apitem.erase(s_apitem.begin() + i);
			continue;
		}
		i++;

		if (pitem->ptex == NULL || pitem->nScaleTex != nScale ||
				pitem->nPaletteSerial != nSerial || pitem->yTopTex != yTop) {
			if (!BuildTexture(prend, pitem, nScale, yTop))
				continue;
		}
		if (nVisible == 1)
			ApplyMask(pitem, abMask);
		else
			RestoreUnmasked(pitem);

		SDL_FRect rcDst;
		rcDst.x = (float)((int)floor(xFrame + 0.5f) + pitem->xOut);
		rcDst.y = (float)((int)floor(yFrame + 0.5f) + pitem->yOut);
		rcDst.w = (float)pitem->cxOut;
		rcDst.h = (float)pitem->cyOut;
		SDL_RenderTexture(prend, pitem->ptex, NULL, &rcDst);
	}

	SDL_SetRenderLogicalPresentation(prend, cxLogical, cyLogical, mode);
}

} // namespace wi
