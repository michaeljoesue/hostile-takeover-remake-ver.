#include "../ht.h"
#include "game/sdl/sdlspritemgr.h"
#include "game/sdl/hosthelpers.h"
#include <cmath>

namespace wi {

// The window and renderer live for the whole process. The game recreates its
// Display when the resolution changes (it restarts itself and resumes from a
// save), and keeping the window avoids it disappearing and reappearing.

static SDL_Window *s_window;
static SDL_Renderer *s_renderer;
static SdlSpriteManager *s_psprm;
static Size s_siz;

bool gfFullscreen = true;

// Game resolutions offered, as long as they fit on the desktop

static const struct { int cx, cy; } s_asizModes[] = {
    { 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1280, 800 },
    { 1366, 768 }, { 1440, 900 }, { 1600, 900 }, { 1680, 1050 },
    { 1920, 1080 }, { 1920, 1200 }, { 2560, 1080 }, { 2560, 1440 },
    { 2560, 1600 }, { 3440, 1440 }, { 3840, 2160 },
};

Display *HostCreateDisplay() {
	Display *pdisp = new Display();
	if (pdisp == NULL)
		return NULL;
	if (!pdisp->Init()) {
		delete pdisp;
		return NULL;
	}
	return pdisp;
}

void HostDestroyWindow()
{
    if (s_renderer != NULL) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    if (s_window != NULL) {
        SDL_DestroyWindow(s_window);
        s_window = NULL;
    }
}

Display::Display()
{
	m_cx = 0;
	m_cy = 0;
	memset(m_amodeInfo, 0, sizeof(m_amodeInfo));
	m_imode = -1;
	m_cmodes = 0;
	m_pbmBack = NULL;
	m_pbmFront = NULL;
	m_pbmClip = NULL;
    m_texture = NULL;
    m_ppal = NULL;
    m_fPaletteTexture = false;
    m_gamePixels = NULL;
    m_gamePixels32 = NULL;
    memset(m_palette, 0, sizeof(m_palette));
}

Display::~Display()
{
	delete m_pbmBack;
	m_pbmBack = NULL;
	delete m_pbmFront;
	m_pbmFront = NULL;
	delete m_pbmClip;
	m_pbmClip = NULL;

    if (m_texture != NULL) {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
    }
    if (m_ppal != NULL) {
        SDL_DestroyPalette(m_ppal);
        m_ppal = NULL;
    }
    delete[] m_gamePixels;
    m_gamePixels = NULL;
    delete[] m_gamePixels32;
    m_gamePixels32 = NULL;
}

static void AddMode(ModeInfo *amode, int *pcmodes, int cx, int cy,
        bool fNative)
{
    cx &= ~1;
    for (int i = 0; i < *pcmodes; i++) {
        if (amode[i].cx == cx && amode[i].cy == cy) {
            amode[i].fNative |= fNative;
            return;
        }
    }
    if (*pcmodes >= kcmodesMax)
        return;
    cx &= ~1;   // DibBitmap rows are an even number of bytes
    ModeInfo *pmode = &amode[(*pcmodes)++];
    pmode->cx = cx;
    pmode->cy = cy;
    pmode->cyGraffiti = 0;
    pmode->nDepth = 8;
    pmode->fNative = fNative;
    pmode->nDegreeOrientation = 0;
}

bool Display::Init()
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        LOG() << "Couldn't initialize SDL video: " << SDL_GetError();
        return false;
    }

    // Desktop size in pixels

    int cxDesktop = 1920;
    int cyDesktop = 1080;
    SDL_DisplayID idDisplay = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *pdm = SDL_GetDesktopDisplayMode(idDisplay);
    if (pdm != NULL && pdm->w > 0 && pdm->h > 0) {
        float density = pdm->pixel_density > 0 ? pdm->pixel_density : 1.0f;
        cxDesktop = (int)(pdm->w * density + 0.5f);
        cyDesktop = (int)(pdm->h * density + 0.5f);
    }

    // The recommended game resolution is the desktop size, divided down on
    // very high resolution screens so the art isn't tiny (e.g. 4K -> 1080p).

    int nScale = (int)std::lround(cyDesktop / 1080.0);
    if (nScale < 1)
        nScale = 1;
    int cxNative = cxDesktop / nScale;
    int cyNative = cyDesktop / nScale;
    if (cxNative < 800 || cyNative < 600) {
        cxNative = 800;
        cyNative = 600;
    }

    m_cmodes = 0;
    for (int i = 0; i < ARRAYSIZE(s_asizModes); i++) {
        if (s_asizModes[i].cx <= cxDesktop && s_asizModes[i].cy <= cyDesktop)
            AddMode(m_amodeInfo, &m_cmodes, s_asizModes[i].cx,
                    s_asizModes[i].cy, false);
    }
    AddMode(m_amodeInfo, &m_cmodes, cxNative, cyNative, true);
    if (cxDesktop != cxNative)
        AddMode(m_amodeInfo, &m_cmodes, cxDesktop, cyDesktop, false);

    // Create the window on first use

    if (s_window == NULL) {
        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
        int cxWindow = cxNative;
        int cyWindow = cyNative;
        if (pdm != NULL) {
            // Window size is in screen coordinates; fit in 85% of the desktop
            float density = pdm->pixel_density > 0 ? pdm->pixel_density : 1.0f;
            cxWindow = (int)(cxNative / density);
            cyWindow = (int)(cyNative / density);
            float scale = std::fmin(pdm->w * 0.85f / cxWindow,
                    pdm->h * 0.85f / cyWindow);
            if (scale < 1.0f) {
                cxWindow = (int)(cxWindow * scale);
                cyWindow = (int)(cyWindow * scale);
            }
        }
        s_window = SDL_CreateWindow("Hostile Takeover", cxWindow, cyWindow,
                flags);
        if (s_window == NULL) {
            LOG() << "Couldn't create window: " << SDL_GetError();
            return false;
        }
        SDL_SetWindowMinimumSize(s_window, 400, 300);

        s_renderer = SDL_CreateRenderer(s_window, NULL);
        if (s_renderer == NULL) {
            LOG() << "Couldn't create renderer: " << SDL_GetError();
            return false;
        }
        SDL_SetRenderVSync(s_renderer, 1);
        LOG() << "Renderer: " << SDL_GetRendererName(s_renderer);

        SDL_SetWindowFullscreen(s_window, gfFullscreen);
        SDL_ShowWindow(s_window);
    }

    HostOutputDebugString("Display::Init %d modes", m_cmodes);
    return true;
}

void Display::SetPalette(Palette *ppal)
{
	int cEntries = BigWord(ppal->cEntries);
    if (cEntries > 256)
        cEntries = 256;
	byte *pb = (byte *)ppal->argb;
    SDL_Color aclr[256];
    memset(aclr, 0, sizeof(aclr));
	for (int i = 0; i < cEntries; i++) {
		aclr[i].r = *pb++;
		aclr[i].g = *pb++;
		aclr[i].b = *pb++;
        aclr[i].a = 255;
        m_palette[i] = 0xff000000 | (aclr[i].r << 16) | (aclr[i].g << 8) |
                aclr[i].b;
	}
    if (m_ppal != NULL)
        SDL_SetPaletteColors(m_ppal, aclr, 0, cEntries);
}

int Display::GetModeCount()
{
    return m_cmodes;
}

void Display::GetModeInfo(int imode, ModeInfo *pmode)
{
	memset(pmode, 0, sizeof(*pmode));
	if (imode >= 0 && imode < m_cmodes)
		*pmode = m_amodeInfo[imode];
}

int Display::GetMode(ModeInfo *pmode)
{
	if (pmode != NULL) {
		if (m_imode == -1) {
			memset(pmode, 0, sizeof(*pmode));
		} else {
			*pmode = m_amodeInfo[m_imode];
		}
	}
	return m_imode;
}

bool Display::SetMode(int imode)
{
    if (imode < 0 || imode >= m_cmodes)
        return false;
	ModeInfo *pmode = &m_amodeInfo[imode];
    int cx = pmode->cx;
    int cy = pmode->cy;

    // Game frame buffer (8 bits per pixel)

    byte *pbPixels = new byte[cx * cy];
    memset(pbPixels, 0, cx * cy);
	DibBitmap *pbmBack = CreateDibBitmap(NULL, cx, cy);
	if (pbmBack == NULL) {
        delete[] pbPixels;
		return false;
    }
    DibBitmap *pbmFront = CreateDibBitmap(pbPixels, cx, cy);
	if (pbmFront == NULL) {
		delete pbmBack;
        delete[] pbPixels;
		return false;
	}

    // Texture. A palettized texture lets the GPU do the color lookup; fall
    // back to converting to 32 bits per pixel on the CPU.

    if (m_texture != NULL)
        SDL_DestroyTexture(m_texture);
    m_fPaletteTexture = false;
    m_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_INDEX8,
            SDL_TEXTUREACCESS_STREAMING, cx, cy);
    if (m_texture != NULL) {
        if (m_ppal == NULL)
            m_ppal = SDL_CreatePalette(256);
        if (m_ppal != NULL && SDL_SetTexturePalette(m_texture, m_ppal)) {
            m_fPaletteTexture = true;
        } else {
            SDL_DestroyTexture(m_texture);
            m_texture = NULL;
        }
    }
    if (m_texture == NULL) {
        m_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_XRGB8888,
                SDL_TEXTUREACCESS_STREAMING, cx, cy);
    }
    if (m_texture == NULL) {
        LOG() << "Couldn't create texture: " << SDL_GetError();
        delete pbmBack;
        delete pbmFront;
        delete[] pbPixels;
        return false;
    }
    SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_PIXELART);
    LOG() << "Game resolution " << cx << "x" << cy
            << (m_fPaletteTexture ? " (palette texture)" : " (32bpp texture)");

    delete[] m_gamePixels32;
    m_gamePixels32 = NULL;
    if (!m_fPaletteTexture)
        m_gamePixels32 = new dword[cx * cy];

	delete m_pbmBack;
	delete m_pbmFront;
    delete[] m_gamePixels;
	m_pbmBack = pbmBack;
	m_pbmFront = pbmFront;
    m_gamePixels = pbPixels;
	m_imode = imode;
    m_cx = cx;
    m_cy = cy;
    s_siz.cx = cx;
    s_siz.cy = cy;

    // Scale the game resolution to the window, keeping the aspect ratio

    SDL_SetRenderLogicalPresentation(s_renderer, cx, cy,
            SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return true;
}

void Display::DrawText(const char *psz, int x, int y, word wf)
{
}

void Display::DrawFrameInclusive(Rect *prc)
{
}

DibBitmap *Display::GetBackDib()
{
    return m_pbmBack;
}

DibBitmap *Display::GetFrontDib()
{
    return m_pbmFront;
}

DibBitmap *Display::GetClippingDib()
{
    DibBitmap *pbm = CreateDibBitmap(NULL, kcCopyBy4Procs * 4, kcCopyBy4Procs * 4);
    if (pbm == NULL)
		return NULL;
    delete m_pbmClip;
	m_pbmClip = pbm;
	return pbm;
}

void Display::GetHslAdjustments(short *pnHueOffset, short *pnSatMultiplier, short *pnLumOffset)
{
	*pnHueOffset = 0;
	*pnSatMultiplier = 0;
	*pnLumOffset = 0;
}

void Display::FrameStart()
{
    m_pbmFront->Init(m_gamePixels, m_cx, m_cy);
}

void Display::FrameComplete(int cfrmm, UpdateMap **apupd, Rect *arc,
        bool fScrolled)
{
    RenderGameSurface();
}

void Display::RenderGameSurface() {
    if (m_texture == NULL || m_gamePixels == NULL)
        return;

    // Upload the frame

    if (m_fPaletteTexture) {
        SDL_UpdateTexture(m_texture, NULL, m_gamePixels, m_cx);
    } else {
        int cp = m_cx * m_cy;
        for (int i = 0; i < cp; i++)
            m_gamePixels32[i] = m_palette[m_gamePixels[i]];
        SDL_UpdateTexture(m_texture, NULL, m_gamePixels32, m_cx * 4);
    }

    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    SDL_RenderTexture(s_renderer, m_texture, NULL, NULL);

    // Sprites draw on top, in game coordinates

    if (s_psprm != NULL)
        s_psprm->DrawSprites(s_renderer, s_siz);

    SDL_RenderPresent(s_renderer);
}

void Display::ResetScrollOffset()
{
}

SpriteManager *Display::GetSpriteManager()
{
	if (s_psprm == NULL)
		s_psprm = new SdlSpriteManager();
	return s_psprm;
}

bool Display::IsFullscreen()
{
    return gfFullscreen;
}

void Display::SetFullscreen(bool fFullscreen)
{
    gfFullscreen = fFullscreen;
    if (s_window != NULL)
        SDL_SetWindowFullscreen(s_window, fFullscreen);
}

SDL_Renderer *Display::GetRenderer()
{
    return s_renderer;
}

void Display::GetSize(Size *psiz)
{
    psiz->cx = m_cx;
    psiz->cy = m_cy;
}

} // namespace wi
