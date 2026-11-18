// HD structures: the artist's reworked buildings, towers and walls drawn as
// full-colour pictures at the window's resolution over the 8-bit game image.
//
// How it fits the engine
//
// - The game still draws every frame of the playfield into its 8-bit image.
//   For a frame that has an HD picture, AnimationData::DrawFrame draws only
//   the classic frame's shadow there (the ground darkened through the level's
//   shadow table, exactly as the classic sprite would), and records the HD
//   picture with a snapshot of the 8-bit pixels under it (HdRecord).
// - Whatever the game draws later on top (units in front, selection,
//   health bars, smoke, opaque fog, dialogs) changes those 8-bit pixels. When
//   the frame is shown, each picture is drawn only where the pixels are still
//   the snapshot (or were only darkened by another HD building's shadow), so
//   it keeps the game's own drawing order without knowing about it.
// - The soft fog edges are the true colour layer (black with an alpha); they
//   are applied to the pictures through the same mask, so fog covers HD
//   buildings as it covers the ground.
// - Pictures are scaled down on the CPU (area average) to the window's size
//   once and kept as textures per side, so they stay sharp without shimmer.
//   Team colour: the blue paint (a mask made from the sheets) takes the side's
//   colour, keeping the painted shading.
// - Animations stay the game's: the classic strip and frame pick the
//   picture, and all the artist's in-between pictures play between classic
//   frames in game time (see PickPic), so the game logic and timing don't
//   change and the motion follows the game speed.
//
// Data: hd/manifest.txt (strip -> pictures) and hd/<pack>.hdp (pictures, QOI
// compressed, with a team colour mask each). Made by tools/hd/build_hd.py from
// the sheets in assets/hd. Without the hd folder the game runs as before;
// F9 switches between HD and classic.

#include "../ht.h"
#include "game/sdl/hosthelpers.h"
#include "game/sdl/hdsprites.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace wi {

extern byte *gmpiclriclrShadow;

// --------------------------------------------------------------------------
// QOI decoder (https://qoiformat.org), RGBA output

static bool QoiDecode(const byte *pb, int cb, std::vector<byte> *pvb, int *pcx, int *pcy)
{
    if (cb < 22 || pb[0] != 'q' || pb[1] != 'o' || pb[2] != 'i' || pb[3] != 'f')
        return false;
    int cx = (pb[4] << 24) | (pb[5] << 16) | (pb[6] << 8) | pb[7];
    int cy = (pb[8] << 24) | (pb[9] << 16) | (pb[10] << 8) | pb[11];
    if (cx <= 0 || cy <= 0 || cx > 8192 || cy > 8192)
        return false;
    pvb->resize((size_t)cx * cy * 4);
    byte *pbOut = &(*pvb)[0];
    byte aclr[64][4];
    memset(aclr, 0, sizeof(aclr));
    byte r = 0, g = 0, b = 0, a = 255;
    int run = 0;
    int i = 14;
    int cp = cx * cy;
    for (int p = 0; p < cp; p++) {
        if (run > 0) {
            run--;
        } else if (i < cb) {
            byte b1 = pb[i++];
            if (b1 == 0xfe) {
                r = pb[i]; g = pb[i + 1]; b = pb[i + 2]; i += 3;
            } else if (b1 == 0xff) {
                r = pb[i]; g = pb[i + 1]; b = pb[i + 2]; a = pb[i + 3]; i += 4;
            } else if ((b1 & 0xc0) == 0x00) {
                r = aclr[b1][0]; g = aclr[b1][1]; b = aclr[b1][2]; a = aclr[b1][3];
            } else if ((b1 & 0xc0) == 0x40) {
                r += ((b1 >> 4) & 3) - 2;
                g += ((b1 >> 2) & 3) - 2;
                b += (b1 & 3) - 2;
            } else if ((b1 & 0xc0) == 0x80) {
                byte b2 = pb[i++];
                int vg = (b1 & 0x3f) - 32;
                r += vg - 8 + ((b2 >> 4) & 0x0f);
                g += vg;
                b += vg - 8 + (b2 & 0x0f);
            } else {
                run = b1 & 0x3f;
            }
            int h = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
            aclr[h][0] = r; aclr[h][1] = g; aclr[h][2] = b; aclr[h][3] = a;
        }
        pbOut[p * 4] = r;
        pbOut[p * 4 + 1] = g;
        pbOut[p * 4 + 2] = b;
        pbOut[p * 4 + 3] = a;
    }
    *pcx = cx;
    *pcy = cy;
    return true;
}

// --------------------------------------------------------------------------
// Data

#define kcHdSides (kcSides + 1)     // the sides, and white for a hit flash
#define kiHdFlash kcSides

struct HdTex {
    SDL_Texture *ptex;
    int cx, cy;                 // texture size (scaled)
    float flScale;              // window pixels per game pixel it was made for
};

struct HdPic {
    float x, y;                 // top-left, game pixels from the frame's draw point
    float scale;                // picture pixels per game pixel
    int cx, cy;                 // picture size
    const byte *pbQoi;
    int cbQoi;
    const byte *pbTeam;
    int cbTeam;
    HdTex atex[kcHdSides];
};

struct HdPack {
    std::vector<byte> vb;
    std::map<std::string, HdPic *> mappic;
};

struct HdStrip {
    std::vector<HdPic *> apic[3];   // by variant letter: [0] h / plain, [1] d
    bool fVariantPics;          // pics depend on the variant drawn just before
    int n;
    int var;                    // -1, or the variant this strip sets
    bool fBlend;
    bool fNone;
    int cycle;                  // > 0: one classic frame; the pictures loop every this many updates
    int cupdMotion;             // updates a step of the animation takes (-1: not worked out yet)
    std::vector<int> amap, abmap;
};

struct HdAnim {
    std::map<std::string, HdStrip *> mapstrip;
};

struct HdBinding {
    std::vector<HdStrip *> apstrip;
};

struct HdItem {
    HdPic *ppic;
    int side;
    float x, y;                 // picture top-left, game pixels, front buffer coordinates
    Rect rc;                    // game pixels it may cover (clipped to the playfield)
    int ibSnap;                 // its snapshot in s_vbSnap
    bool fClipped;              // rc is less than the whole picture
};

struct HdTrack {
    int nStrip;
    int nFrame;
    double tCreate;
    double tChange;
    double tDur;
    int picFrom, picTo;
    int nDir;                   // 1 forward, -1 back, 0 none
    int nLastDir;
    bool fRunning;              // seen stepping forward by itself
    double tLast;               // when last drawn
    double tCycle;              // own clock of a picture loop (cycle=)

    int cFrameSeen;
};

static bool s_fLoaded;
static bool s_fTriedLoad;
static bool s_fEnabled = true;
static std::map<std::string, HdPack *> s_mappack;
static std::map<std::string, HdAnim *> s_mapanim;
static std::vector<HdItem> s_vitem;
static std::vector<byte> s_vbSnap;
static std::vector<byte> s_vbHdShadow;  // front pixels darkened by an HD building's shadow pass
static std::vector<byte> s_vbHdCovered; // front pixels covered by opaque fog
static int s_cxHdShadow, s_cyHdShadow;
static std::map<Animation *, HdTrack> s_maptrack;
static std::set<Animation *> s_setHeld;
static int s_cFrame;
static int s_nVarLast;
static bool s_fMoving;
static bool s_fPaintPending;        // a paint started and the playfield hasn't drawn yet

static std::string Lower(const char *psz) {
    std::string str(psz);
    for (size_t i = 0; i < str.size(); i++)
        str[i] = (char)tolower((byte)str[i]);
    return str;
}

static bool ReadFile(const char *pszPath, std::vector<byte> *pvb) {
    SDL_IOStream *pio = SDL_IOFromFile(pszPath, "rb");
    if (pio == NULL)
        return false;
    Sint64 cb = SDL_GetIOSize(pio);
    if (cb <= 0) {
        SDL_CloseIO(pio);
        return false;
    }
    pvb->resize((size_t)cb);
    size_t cbRead = SDL_ReadIO(pio, &(*pvb)[0], (size_t)cb);
    SDL_CloseIO(pio);
    return cbRead == (size_t)cb;
}

static HdPack *LoadPack(const std::string &strPack) {
    std::map<std::string, HdPack *>::iterator it = s_mappack.find(strPack);
    if (it != s_mappack.end())
        return it->second;
    HdPack *ppack = new HdPack;
    s_mappack[strPack] = ppack;
    char szPath[PATH_MAX];
    snprintf(szPath, sizeof(szPath), "%s/hd/%s.hdp", HostHelpers::GetMainDataDir(), strPack.c_str());
    if (!ReadFile(szPath, &ppack->vb) || ppack->vb.size() < 8 || memcmp(&ppack->vb[0], "HDP1", 4) != 0) {
        HostOutputDebugString((char *)"HD: can't read %s", szPath);
        return ppack;
    }
    const byte *pb = &ppack->vb[0];
    size_t cb = ppack->vb.size();
    size_t i = 4;
    dword cpic;
    memcpy(&cpic, pb + i, 4); i += 4;
    for (dword n = 0; n < cpic && i < cb; n++) {
        int cch = pb[i++];
        std::string strName((const char *)pb + i, cch); i += cch;
        HdPic *ppic = new HdPic;
        memset(ppic, 0, sizeof(*ppic));
        memcpy(&ppic->x, pb + i, 4); i += 4;
        memcpy(&ppic->y, pb + i, 4); i += 4;
        memcpy(&ppic->scale, pb + i, 4); i += 4;
        dword cbQ;
        memcpy(&cbQ, pb + i, 4); i += 4;
        ppic->pbQoi = pb + i; ppic->cbQoi = cbQ; i += cbQ;
        memcpy(&cbQ, pb + i, 4); i += 4;
        if (cbQ != 0) {
            ppic->pbTeam = pb + i; ppic->cbTeam = cbQ; i += cbQ;
        }
        if (i > cb || ppic->cbQoi < 14) {
            delete ppic;
            break;
        }
        const byte *pbQ = ppic->pbQoi;
        ppic->cx = (pbQ[4] << 24) | (pbQ[5] << 16) | (pbQ[6] << 8) | pbQ[7];
        ppic->cy = (pbQ[8] << 24) | (pbQ[9] << 16) | (pbQ[10] << 8) | pbQ[11];
        ppack->mappic[strName] = ppic;
    }
    return ppack;
}

static HdPic *FindPic(const std::string &strPack, const std::string &strName) {
    HdPack *ppack = LoadPack(strPack);
    std::map<std::string, HdPic *>::iterator it = ppack->mappic.find(strName);
    return it == ppack->mappic.end() ? NULL : it->second;
}

static void ParseList(const std::string &str, std::vector<int> *pv) {
    const char *psz = str.c_str();
    while (*psz != 0) {
        pv->push_back(atoi(psz));
        while (*psz != 0 && *psz != ',')
            psz++;
        if (*psz == ',')
            psz++;
    }
}

static void LoadManifest() {
    s_fTriedLoad = true;
    char szPath[PATH_MAX];
    snprintf(szPath, sizeof(szPath), "%s/hd/manifest.txt", HostHelpers::GetMainDataDir());
    std::vector<byte> vb;
    if (!ReadFile(szPath, &vb))
        return;
    vb.push_back(0);
    int cStrips = 0;
    char *psz = (char *)&vb[0];
    while (*psz != 0) {
        char *pszEnd = psz;
        while (*pszEnd != 0 && *pszEnd != '\n')
            pszEnd++;
        char chSave = *pszEnd;
        *pszEnd = 0;
        if (pszEnd > psz && pszEnd[-1] == '\r')
            pszEnd[-1] = 0;

        // strip<TAB>anir<TAB>strip name<TAB>key=value...

        std::vector<std::string> astr;
        for (char *pszT = psz; ; ) {
            char *pszTab = strchr(pszT, '\t');
            if (pszTab == NULL) {
                astr.push_back(pszT);
                break;
            }
            astr.push_back(std::string(pszT, pszTab - pszT));
            pszT = pszTab + 1;
        }
        if (astr.size() >= 3 && astr[0] == "strip") {
            HdStrip *pstrip = new HdStrip;
            pstrip->n = 1;
            pstrip->var = -1;
            pstrip->fBlend = true;
            pstrip->fNone = false;
            pstrip->fVariantPics = false;
            pstrip->cycle = 0;
            pstrip->cupdMotion = -1;
            std::string strPics;
            for (size_t k = 3; k < astr.size(); k++) {
                size_t iEq = astr[k].find('=');
                if (iEq == std::string::npos)
                    continue;
                std::string strKey = astr[k].substr(0, iEq);
                std::string strVal = astr[k].substr(iEq + 1);
                if (strKey == "pics") strPics = strVal;
                else if (strKey == "n") pstrip->n = atoi(strVal.c_str());
                else if (strKey == "var") pstrip->var = atoi(strVal.c_str());
                else if (strKey == "blend") pstrip->fBlend = atoi(strVal.c_str()) != 0;
                else if (strKey == "none") pstrip->fNone = atoi(strVal.c_str()) != 0;
                else if (strKey == "cycle") pstrip->cycle = atoi(strVal.c_str());
                else if (strKey == "map") ParseList(strVal, &pstrip->amap);
                else if (strKey == "bmap") ParseList(strVal, &pstrip->abmap);
            }

            // Resolve the pictures: pack/prefix + 3-digit index; {V} = h or d

            bool fOk = pstrip->fNone;
            if (!strPics.empty()) {
                size_t iSlash = strPics.find('/');
                std::string strPack = strPics.substr(0, iSlash);
                std::string strPrefix = strPics.substr(iSlash + 1);
                size_t iV = strPrefix.find("{V}");
                pstrip->fVariantPics = iV != std::string::npos;
                fOk = true;
                for (int v = 0; v < (pstrip->fVariantPics ? 2 : 1); v++) {
                    std::string strP = strPrefix;
                    if (pstrip->fVariantPics)
                        strP.replace(iV, 3, v == 0 ? "h" : "d");
                    for (int i = 0; i < pstrip->n; i++) {
                        char szIdx[8];
                        snprintf(szIdx, sizeof(szIdx), "%03d", i);
                        HdPic *ppic = FindPic(strPack, strP + szIdx);
                        if (ppic == NULL)
                            fOk = false;
                        pstrip->apic[v].push_back(ppic);
                    }
                }
            }
            if (fOk) {
                std::string strAnir = Lower(astr[1].c_str());
                HdAnim *panim = s_mapanim[strAnir];
                if (panim == NULL) {
                    panim = new HdAnim;
                    s_mapanim[strAnir] = panim;
                }
                panim->mapstrip[astr[2]] = pstrip;
                cStrips++;
            } else {
                HostOutputDebugString((char *)"HD: pictures missing for %s %s", astr[1].c_str(), astr[2].c_str());
                delete pstrip;
            }
        }
        *pszEnd = chSave;
        psz = *pszEnd != 0 ? pszEnd + 1 : pszEnd;
    }
    s_fLoaded = cStrips != 0;
    HostOutputDebugString((char *)"HD: %d strips of %d animations", cStrips, (int)s_mapanim.size());
}

// --------------------------------------------------------------------------
// Game side

HdBinding *HdBindAnimation(const char *pszAniName, AnimationData *panid) {
    if (!s_fTriedLoad)
        LoadManifest();
    if (!s_fLoaded)
        return NULL;
    std::map<std::string, HdAnim *>::iterator it = s_mapanim.find(Lower(pszAniName));
    if (it == s_mapanim.end())
        return NULL;
    HdBinding *pbind = new HdBinding;
    int cStrips = panid->GetStripCount();
    bool fAny = false;
    for (int n = 0; n < cStrips; n++) {
        std::map<std::string, HdStrip *>::iterator itS = it->second->mapstrip.find(panid->GetStripName(n));
        HdStrip *pstrip = itS == it->second->mapstrip.end() ? NULL : itS->second;
        pbind->apstrip.push_back(pstrip);
        if (pstrip != NULL)
            fAny = true;
    }
    if (!fAny) {
        delete pbind;
        return NULL;
    }
    return pbind;
}

void HdToggle() {
    s_fEnabled = !s_fEnabled;
    s_vitem.clear();
}

bool HdIsActive() {
    return s_fEnabled && s_fLoaded;
}

bool HdWantsStrip(HdBinding *pbind, int nStrip, DibBitmap *pbm) {
    if (!HdIsActive() || pbind == NULL || nStrip < 0 || nStrip >= (int)pbind->apstrip.size())
        return false;
    if (pbind->apstrip[nStrip] == NULL)
        return false;

    // Only on the playfield (not menus or forms)

    return gpfrmmSim != NULL && pbm == gpfrmmSim->GetDib();
}

bool HdWantsRedraw() {
    return HdIsActive() && s_fMoving;
}

void HdPaintStart() {
    s_fPaintPending = true;
}

void HdBeginFrame() {
    s_fPaintPending = false;
    s_vitem.clear();
    s_vbSnap.clear();
    s_nVarLast = 0;
    s_fMoving = false;
    s_cFrame++;
    if (gpdisp != NULL) {
        DibBitmap *pbmFront = gpdisp->GetFrontDib();
        if (pbmFront != NULL) {
            Size siz;
            pbmFront->GetSize(&siz);
            if (siz.cx != s_cxHdShadow || siz.cy != s_cyHdShadow) {
                s_cxHdShadow = siz.cx;
                s_cyHdShadow = siz.cy;
                s_vbHdShadow.assign((size_t)siz.cx * siz.cy, 0);
                s_vbHdCovered.assign((size_t)siz.cx * siz.cy, 0);
            } else if (!s_vbHdShadow.empty()) {
                memset(&s_vbHdShadow[0], 0, s_vbHdShadow.size());
                memset(&s_vbHdCovered[0], 0, s_vbHdCovered.size());
            }
        }
    }

    // Forget animations that haven't drawn for a while (their gobs are gone)

    if ((s_cFrame & 255) == 0) {
        for (std::map<Animation *, HdTrack>::iterator it = s_maptrack.begin(); it != s_maptrack.end(); ) {
            if (s_cFrame - it->second.cFrameSeen > 600)
                s_maptrack.erase(it++);
            else
                ++it;
        }
    }
}

void HdNoteCovered(DibBitmap *pbm, int x, int y, int cx, int cy) {
    if (!HdIsActive() || s_vitem.empty() || s_vbHdCovered.empty())
        return;
    // Any part of the back buffer (the playfield, or a form over it such
    // as the minimap's)

    DibBitmap *pbmBack = gpdisp->GetBackDib();
    if (pbmBack == NULL || pbm->GetRowBytes() != pbmBack->GetRowBytes() ||
            pbm->GetBits() < pbmBack->GetBits())
        return;
    int yTop = (int)((pbm->GetBits() - pbmBack->GetBits()) / pbmBack->GetRowBytes());
    Size siz;
    pbm->GetSize(&siz);
    int xL = x < 0 ? 0 : x, xR = x + cx > siz.cx ? siz.cx : x + cx;
    int yT = y < 0 ? 0 : y, yB = y + cy > siz.cy ? siz.cy : y + cy;
    for (int yP = yT; yP < yB; yP++) {
        int yF = yP + yTop;
        if (yF < 0 || yF >= s_cyHdShadow || xL >= xR || xR > s_cxHdShadow)
            continue;
        memset(&s_vbHdCovered[(size_t)yF * s_cxHdShadow + xL], 1, xR - xL);
    }
}

// The game holds this animation still (a building without power), or lets
// it go again: pictures looping on their own stop and go with it

void HdHoldAnimation(Animation *pani, bool fHold) {
    if (fHold)
        s_setHeld.insert(pani);
    else
        s_setHeld.erase(pani);
}

// Game time in updates (with the fraction since the last update)

static double SimTime() {
    return gsim.GetUpdateCount() + gsim.GetInterpolationAlpha() / 256.0;
}

static int TargetPic(HdStrip *pstrip, int nFrame, int cFrames, int nDir) {
    if (nDir < 0 && nFrame < (int)pstrip->abmap.size())
        return pstrip->abmap[nFrame];
    if (nFrame < (int)pstrip->amap.size())
        return pstrip->amap[nFrame];
    if (cFrames <= 0)
        return 0;
    return nFrame * pstrip->n / cFrames;
}

// Updates the classic frame is shown for

static int FrameUpdates(AnimationData *panid, int nStrip, int nFrame) {
    return panid->GetStripDelay(nStrip) + panid->GetFrameDelay(nStrip, nFrame) + 1;
}

// Updates one step of the motion takes: the strip's shortest frame. Longer
// frames are pauses (the HQ's hatch staying shut): the picture holds, then
// moves at the same speed as the rest of the animation.

static int MotionUpdates(HdStrip *pstrip, AnimationData *panid, int nStrip) {
    if (pstrip->cupdMotion < 0) {
        int cFrames = panid->GetFrameCount(nStrip);
        int cupdMin = 0;
        for (int f = 0; f < cFrames; f++) {
            int cupd = FrameUpdates(panid, nStrip, f);
            if (cupdMin == 0 || cupd < cupdMin)
                cupdMin = cupd;
        }
        pstrip->cupdMotion = cupdMin > 0 ? cupdMin : 1;
    }
    return pstrip->cupdMotion;
}

// Which picture. Every picture the artist drew plays, in game time (so it
// follows the game speed and stops when the game is paused):
//
// - A looping animation (generator, dishes, HQ, R&D panel) runs through the
//   pictures between one classic frame's and the next's while the classic
//   frame is shown, reaching the next one's picture just as the classic
//   frame changes: a continuous motion that keeps step with the classic
//   frames (which the game logic still uses).
// - An animation the game steps itself (the Processor's door, forward to
//   open and back to close) plays the pictures between the old frame's and
//   the new one's after each step, at the same speed.
// - Strips whose art has far more pictures than the classic frames (the
//   generator's fan, the Surveillance Center's dish, the R&D Center's
//   panel, also when damaged, where the classic art doesn't move) loop on
//   their own at a natural pace in game time (cycle=), stopping when the
//   game holds the animation (HdHoldAnimation: no power).

static int PickPic(HdStrip *pstrip, AnimationData *panid, int nStrip, int nFrame, Animation *pani) {
    int cFrames = panid->GetFrameCount(nStrip);
    int n = pstrip->n;
    if (!pstrip->fBlend || pani == NULL || n <= 1)
        return TargetPic(pstrip, nFrame, cFrames, 0) % n;

    double t = SimTime();
    std::map<Animation *, HdTrack>::iterator it = s_maptrack.find(pani);
    if (it == s_maptrack.end() || it->second.nStrip != nStrip) {
        HdTrack trk;
        trk.nStrip = nStrip;
        trk.nFrame = nFrame;
        trk.tCreate = t;
        trk.tChange = t;
        trk.tDur = 1;
        trk.picFrom = trk.picTo = TargetPic(pstrip, nFrame, cFrames, 0);
        trk.nDir = 0;
        trk.nLastDir = 0;
        trk.fRunning = false;
        trk.tLast = t;
        trk.tCycle = 0;
        trk.cFrameSeen = s_cFrame;
        it = s_maptrack.insert(std::make_pair(pani, trk)).first;
        it->second = trk;
    }
    HdTrack *ptrk = &it->second;
    ptrk->cFrameSeen = s_cFrame;

    // Looping on its own (cycle=): the art has many more pictures than the
    // classic frames, at a pace of its own. Each building starts at its own
    // point of the loop, so neighbours don't turn in step.

    if (pstrip->cycle > 0) {
        double dt = t - ptrk->tLast;
        ptrk->tLast = t;
        if (dt > 0 && dt < 4 && s_setHeld.find(pani) == s_setHeld.end()) {
            ptrk->tCycle += dt;
            s_fMoving = true;
        }
        double p = ptrk->tCycle / pstrip->cycle + (((uintptr_t)pani >> 4) % 97) / 97.0;
        p -= floor(p);
        return ((int)(p * n)) % n;
    }

    int cupdMotion = MotionUpdates(pstrip, panid, nStrip);
    word wf = pani->GetFlags();
    bool fLoop = (wf & kfAniLoop) && !(wf & kfAniDone) && pstrip->abmap.empty();

    if (nFrame != ptrk->nFrame) {
        int nPrev = ptrk->nFrame;
        int nDir = 0;
        if (nFrame == nPrev + 1 || (nFrame == 0 && nPrev == cFrames - 1 && pstrip->amap.empty()))
            nDir = 1;
        else if (nFrame == nPrev - 1)
            nDir = -1;

        // Where the picture is now

        int picNow = ptrk->picTo;
        if (t - ptrk->tChange < ptrk->tDur && ptrk->tDur > 0) {
            int cSteps = (ptrk->picTo - ptrk->picFrom + n) % n;
            picNow = (ptrk->picFrom + (int)(cSteps * (t - ptrk->tChange) / ptrk->tDur)) % n;
        }

        // Turning back (Processor door): continue from the matching picture
        // of the other direction's sequence

        if (nDir != 0 && ptrk->nLastDir != 0 && nDir != ptrk->nLastDir &&
                !pstrip->abmap.empty() && picNow == ptrk->picTo)
            picNow = TargetPic(pstrip, nPrev, cFrames, nDir);

        ptrk->picTo = TargetPic(pstrip, nFrame, cFrames, nDir);
        ptrk->picFrom = nDir != 0 ? picNow : ptrk->picTo;
        ptrk->tDur = cupdMotion;

        // The frame changed in the latest update

        ptrk->tChange = gsim.GetUpdateCount();
        if (ptrk->tChange > t)
            ptrk->tChange = t;
        ptrk->nFrame = nFrame;
        if (nDir != 0)
            ptrk->nLastDir = nDir;
        ptrk->fRunning = nDir == 1;
    }

    // Only while the animation is seen running: a building that has stopped
    // (no power) keeps its classic frame's picture

    if (fLoop && ptrk->fRunning) {
        // Towards the next classic frame's picture, over the last
        // cupdMotion updates of this frame

        int nNext = (nFrame + 1) % cFrames;
        int picKey = TargetPic(pstrip, nFrame, cFrames, 1) % n;
        int picNext = TargetPic(pstrip, nNext, cFrames, 1) % n;
        int cSteps = (picNext - picKey + n) % n;
        int cupdFrame = FrameUpdates(panid, nStrip, nFrame);
        double p = (t - ptrk->tChange - (cupdFrame - cupdMotion)) / cupdMotion;
        if (p <= 0 || cSteps == 0)
            return picKey;
        if (p >= 1)
            return picNext;
        s_fMoving = true;
        return (picKey + (int)(cSteps * p)) % n;
    }

    if (t - ptrk->tChange >= ptrk->tDur || ptrk->picFrom == ptrk->picTo)
        return ptrk->picTo % n;
    s_fMoving = true;
    int cSteps = (ptrk->picTo - ptrk->picFrom + n) % n;
    double p = (t - ptrk->tChange) / ptrk->tDur;
    if (p < 0)
        p = 0;
    return (ptrk->picFrom + (int)(cSteps * p)) % n;
}

void HdRecord(HdBinding *pbind, AnimationData *panid, int nStrip, int nFrame, DibBitmap *pbm,
        int x, int y, Side side, Animation *pani, const Rect *prcShadow) {
    HdStrip *pstrip = pbind->apstrip[nStrip];
    if (pstrip->var >= 0)
        s_nVarLast = pstrip->var;

    // Pixels darkened by this building's shadow pass: another HD picture
    // under them stays visible there

    // The playfield is part of the back buffer, which is copied whole to
    // the front buffer (shown) each frame: its place there

    DibBitmap *pbmBack = gpdisp->GetBackDib();
    if (pbmBack == NULL)
        return;
    int cbRow = pbmBack->GetRowBytes();
    if (pbm->GetRowBytes() != cbRow)
        return;
    int yTop = (int)((pbm->GetBits() - pbmBack->GetBits()) / cbRow);
    if (yTop < 0 || yTop >= s_cyHdShadow)
        return;
    Size sizDib;
    pbm->GetSize(&sizDib);
    if (prcShadow != NULL && !s_vbHdShadow.empty()) {
        for (int yT = prcShadow->top; yT < prcShadow->bottom; yT++) {
            if (yT + yTop < 0 || yT + yTop >= s_cyHdShadow)
                continue;
            for (int xT = prcShadow->left; xT < prcShadow->right; xT++)
                s_vbHdShadow[(yT + yTop) * s_cxHdShadow + xT] = 1;
        }
    }

    if (pstrip->fNone)
        return;
    int v = 0;
    if (pstrip->fVariantPics) {
        if (s_nVarLast >= 2)
            return;     // destroyed: the wreck picture is the whole thing
        v = s_nVarLast;
    }
    int ipic = PickPic(pstrip, panid, nStrip, nFrame, pani);
    if (ipic < 0 || ipic >= (int)pstrip->apic[v].size())
        return;
    HdPic *ppic = pstrip->apic[v][ipic];
    if (ppic == NULL)
        return;

    HdItem itm;
    itm.ppic = ppic;
    itm.side = side == (Side)-1 ? kiHdFlash : (side >= 0 && side < kcSides ? side : ksideNeutral);
    itm.x = x + ppic->x;
    itm.y = y + ppic->y + yTop;
    Rect rc;
    rc.left = (int)floorf(x + ppic->x);
    rc.top = (int)floorf(y + ppic->y);
    rc.right = (int)ceilf(x + ppic->x + ppic->cx / ppic->scale);
    rc.bottom = (int)ceilf(y + ppic->y + ppic->cy / ppic->scale);
    itm.fClipped = rc.left < 0 || rc.top < 0 || rc.right > sizDib.cx || rc.bottom > sizDib.cy;
    if (rc.left < 0) rc.left = 0;
    if (rc.top < 0) rc.top = 0;
    if (rc.right > sizDib.cx) rc.right = sizDib.cx;
    if (rc.bottom > sizDib.cy) rc.bottom = sizDib.cy;
    if (rc.IsEmpty())
        return;

    // Snapshot of the 8-bit pixels under it

    itm.ibSnap = (int)s_vbSnap.size();
    s_vbSnap.resize(s_vbSnap.size() + (size_t)rc.Width() * rc.Height());
    byte *pbSnap = &s_vbSnap[itm.ibSnap];
    byte *pbDib = pbm->GetBits();
    for (int yT = rc.top; yT < rc.bottom; yT++) {
        memcpy(pbSnap, pbDib + yT * cbRow + rc.left, rc.Width());
        pbSnap += rc.Width();
    }
    rc.top += yTop;
    rc.bottom += yTop;
    itm.rc = rc;
    s_vitem.push_back(itm);
}

// --------------------------------------------------------------------------
// Display side

// Team colour, as the classic art does it: the classic sprites paint team
// areas with 5 shades (brightest to darkest), drawn from each side's own
// 5-shade ramp in the palette. The artist painted those areas steel blue
// (the mask marks them); each painted pixel's brightness is placed on the
// classic blue ramp's brightnesses and given the side's colour there, so
// every side, blue included, gets exactly its classic colours, with the
// painted shading. Brighter than the brightest shade (highlights) or darker
// than the darkest, the end shade is scaled. White for a hit flash.

static const int s_aiclrRampFirst[kcSides] = {
    kiclrNeutralSideFirst, kiclrBlueSideFirst, kiclrRedSideFirst,
    kiclrYellowSideFirst, kiclrSpringGreenSideFirst, kiclrCrimsonSideFirst,
    kiclrSkyBlueSideFirst
};

static float Luma(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

static void TeamColor(byte *pbRgba, const byte *pbTeam, int cp, int side, const dword *palette) {
    if (pbTeam == NULL)
        return;

    // The side's ramp, and the brightness of each shade of the blue one

    float aclr[5][3];
    float al[5];
    for (int i = 0; i < 5; i++) {
        dword clrBlue = palette[GetColor(kiclrBlueSideFirst + i)];
        al[i] = Luma((float)((clrBlue >> 16) & 255), (float)((clrBlue >> 8) & 255), (float)(clrBlue & 255));
        dword clr = side == kiHdFlash ? 0xffffff : palette[GetColor(s_aiclrRampFirst[side] + i)];
        aclr[i][0] = (float)((clr >> 16) & 255);
        aclr[i][1] = (float)((clr >> 8) & 255);
        aclr[i][2] = (float)(clr & 255);
    }

    for (int i = 0; i < cp; i++) {
        int m = pbTeam[i * 4];
        if (m == 0)
            continue;
        byte *pb = &pbRgba[i * 4];
        float l = Luma(pb[0], pb[1], pb[2]);
        float rgb[3];
        if (l >= al[0]) {
            float k = al[0] > 0 ? l / al[0] : 1;
            for (int c = 0; c < 3; c++)
                rgb[c] = aclr[0][c] * k;
        } else if (l <= al[4]) {
            float k = al[4] > 0 ? l / al[4] : 0;
            for (int c = 0; c < 3; c++)
                rgb[c] = aclr[4][c] * k;
        } else {
            int s = 0;
            while (s < 3 && l < al[s + 1])
                s++;
            float d = al[s] - al[s + 1];
            float f = d > 0 ? (l - al[s + 1]) / d : 0;
            for (int c = 0; c < 3; c++)
                rgb[c] = aclr[s + 1][c] + (aclr[s][c] - aclr[s + 1][c]) * f;
        }
        float w = m / 255.0f;
        for (int c = 0; c < 3; c++) {
            float v = rgb[c] > 255 ? 255 : rgb[c];
            pb[c] = (byte)(pb[c] + (v - pb[c]) * w + 0.5f);
        }
    }
}

// Area-average resample of premultiplied RGBA

static void Resample(const byte *pbSrc, int cxSrc, int cySrc, byte *pbDst, int cxDst, int cyDst) {
    float flX = (float)cxSrc / cxDst, flY = (float)cySrc / cyDst;
    std::vector<float> vfRow((size_t)cxDst * 4);
    for (int yD = 0; yD < cyDst; yD++) {
        float y0 = yD * flY, y1 = y0 + flY;
        std::fill(vfRow.begin(), vfRow.end(), 0.0f);
        float wy = 0;
        for (int yS = (int)y0; yS < cySrc && yS < y1; yS++) {
            float fy = (yS + 1 < y1 ? yS + 1 : y1) - (yS > y0 ? yS : y0);
            if (fy <= 0)
                continue;
            wy += fy;
            const byte *pbRow = pbSrc + (size_t)yS * cxSrc * 4;
            for (int xD = 0; xD < cxDst; xD++) {
                float x0 = xD * flX, x1 = x0 + flX;
                float s[4] = { 0, 0, 0, 0 }, wx = 0;
                for (int xS = (int)x0; xS < cxSrc && xS < x1; xS++) {
                    float fx = (xS + 1 < x1 ? xS + 1 : x1) - (xS > x0 ? xS : x0);
                    if (fx <= 0)
                        continue;
                    wx += fx;
                    const byte *pb = pbRow + xS * 4;
                    s[0] += pb[0] * fx; s[1] += pb[1] * fx; s[2] += pb[2] * fx; s[3] += pb[3] * fx;
                }
                if (wx > 0) {
                    float *pf = &vfRow[xD * 4];
                    pf[0] += s[0] / wx * fy; pf[1] += s[1] / wx * fy;
                    pf[2] += s[2] / wx * fy; pf[3] += s[3] / wx * fy;
                }
            }
        }
        for (int xD = 0; xD < cxDst * 4; xD++) {
            float f = wy > 0 ? vfRow[xD] / wy : 0;
            pbDst[(size_t)yD * cxDst * 4 + xD] = (byte)(f > 255 ? 255 : f + 0.5f);
        }
    }
}

static HdTex *GetTexture(SDL_Renderer *prend, HdPic *ppic, int side, float flScale, const dword *palette) {
    HdTex *ptex = &ppic->atex[side];

    // Made for this window size (pictures are scaled down on the CPU, area
    // averaged, so they don't shimmer; larger than the painting they are
    // stretched by the GPU)

    float flTarget = flScale / ppic->scale;
    if (flTarget > 1)
        flTarget = 1;
    if (ptex->ptex != NULL && fabsf(ptex->flScale - flScale) < 0.001f)
        return ptex;
    if (ptex->ptex != NULL) {
        SDL_DestroyTexture(ptex->ptex);
        ptex->ptex = NULL;
    }
    std::vector<byte> vb, vbTeam;
    int cx, cy;
    if (!QoiDecode(ppic->pbQoi, ppic->cbQoi, &vb, &cx, &cy))
        return NULL;
    int cxT, cyT;
    if (ppic->pbTeam != NULL && QoiDecode(ppic->pbTeam, ppic->cbTeam, &vbTeam, &cxT, &cyT) &&
            cxT == cx && cyT == cy)
        TeamColor(&vb[0], &vbTeam[0], cx * cy, side, palette);

    // Premultiply

    for (int i = 0; i < cx * cy; i++) {
        byte *pb = &vb[i * 4];
        int a = pb[3];
        pb[0] = (byte)((pb[0] * a + 127) / 255);
        pb[1] = (byte)((pb[1] * a + 127) / 255);
        pb[2] = (byte)((pb[2] * a + 127) / 255);
    }

    int cxD = (int)ceilf(cx * flTarget), cyD = (int)ceilf(cy * flTarget);
    if (cxD < 1) cxD = 1;
    if (cyD < 1) cyD = 1;
    std::vector<byte> vbD;
    const byte *pbUpload = &vb[0];
    if (cxD < cx || cyD < cy) {
        vbD.resize((size_t)cxD * cyD * 4);
        Resample(&vb[0], cx, cy, &vbD[0], cxD, cyD);
        pbUpload = &vbD[0];
    } else {
        cxD = cx;
        cyD = cy;
    }
    ptex->ptex = SDL_CreateTexture(prend, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, cxD, cyD);
    if (ptex->ptex == NULL)
        return NULL;
    SDL_UpdateTexture(ptex->ptex, NULL, pbUpload, cxD * 4);
    SDL_SetTextureBlendMode(ptex->ptex, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
    SDL_SetTextureScaleMode(ptex->ptex, SDL_SCALEMODE_LINEAR);
    ptex->cx = cxD;
    ptex->cy = cyD;
    ptex->flScale = flScale;
    return ptex;
}

static SDL_Texture *s_ptexMask;         // game-pixel mask (visible, shadow, fog)
static int s_cxMask, s_cyMask;
static SDL_Texture *s_ptexTarget;       // window-sized scratch target
static int s_cxTarget, s_cyTarget;
static std::vector<dword> s_vdwMask;

bool HdHasItems() {
    // A paint without the playfield (a menu over the whole screen): nothing
    // of the last playfield frame may show

    if (s_fPaintPending) {
        s_fPaintPending = false;
        s_vitem.clear();
    }
    return HdIsActive() && !s_vitem.empty();
}

void HdComposite(SDL_Renderer *prend, float flScale, float xOff, float yOff, const byte *pbFront,
        int cxFront, int cyFront, const dword *palette, const dword *pdwTC, const byte *pbUnder) {
    if (!HdHasItems() || flScale <= 0)
        return;

    // Scratch textures

    if (s_ptexMask == NULL || s_cxMask != cxFront || s_cyMask != cyFront) {
        if (s_ptexMask != NULL)
            SDL_DestroyTexture(s_ptexMask);
        s_ptexMask = SDL_CreateTexture(prend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, cxFront, cyFront);
        if (s_ptexMask == NULL)
            return;
        s_cxMask = cxFront;
        s_cyMask = cyFront;
        SDL_SetTextureScaleMode(s_ptexMask, SDL_SCALEMODE_NEAREST);

        // Multiplies what's under it: colour by the mask's colour, alpha by
        // its alpha

        SDL_BlendMode bm = SDL_ComposeCustomBlendMode(
                SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDOPERATION_ADD,
                SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
        SDL_SetTextureBlendMode(s_ptexMask, bm);
    }
    int cxWin, cyWin;
    SDL_GetCurrentRenderOutputSize(prend, &cxWin, &cyWin);
    if (s_ptexTarget == NULL || s_cxTarget < cxWin || s_cyTarget < cyWin) {
        if (s_ptexTarget != NULL)
            SDL_DestroyTexture(s_ptexTarget);
        s_cxTarget = cxWin;
        s_cyTarget = cyWin;
        s_ptexTarget = SDL_CreateTexture(prend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, cxWin, cyWin);
        if (s_ptexTarget == NULL)
            return;
        SDL_SetTextureBlendMode(s_ptexTarget, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        SDL_SetTextureScaleMode(s_ptexTarget, SDL_SCALEMODE_NEAREST);
    }

    // The forms over the playfield (the minimap, menus, dialogs) cover the
    // pictures under them, whether or not they were repainted this frame.
    // (A form that didn't repaint leaves its old pixels, which an HD
    // picture's snapshot of that spot would match.)

    if (gpmfrmm != NULL && !s_vbHdCovered.empty()) {
        for (int n = 0; n < gpmfrmm->GetFormCount(); n++) {
            Form *pfrm = gpmfrmm->GetFormAt(n);
            word wf = pfrm->GetFlags();
            if (!(wf & kfFrmVisible) || (wf & kfFrmTranslucent))
                continue;
            Rect rcF;
            pfrm->GetRect(&rcF);
            int xL = rcF.left < 0 ? 0 : rcF.left;
            int xR = rcF.right > s_cxHdShadow ? s_cxHdShadow : rcF.right;
            int yT = rcF.top < 0 ? 0 : rcF.top;
            int yB = rcF.bottom > s_cyHdShadow ? s_cyHdShadow : rcF.bottom;
            for (int yF = yT; yF < yB && xL < xR; yF++)
                memset(&s_vbHdCovered[(size_t)yF * s_cxHdShadow + xL], 1, xR - xL);
        }
    }

    SDL_Texture *ptexScreen = SDL_GetRenderTarget(prend);
    for (size_t n = 0; n < s_vitem.size(); n++) {
        HdItem *pitm = &s_vitem[n];
        Rect rc = pitm->rc;
        if (rc.right > cxFront) rc.right = cxFront;
        if (rc.bottom > cyFront) rc.bottom = cyFront;
        if (rc.IsEmpty() || rc.left < 0 || rc.top < 0)
            continue;
        int cx = rc.Width(), cy = rc.Height();

        // The mask: visible where the 8-bit pixel is still the snapshot, or
        // was only darkened by an HD building's shadow (then darkened);
        // darkened again by the fog's soft edge

        s_vdwMask.resize((size_t)cx * cy);
        const byte *pbSnap = &s_vbSnap[pitm->ibSnap];
        int cVisible = 0;
        bool fWhole = true;     // nothing covers it: no mask needed
        for (int y = 0; y < cy; y++) {
            int i = (rc.top + y) * cxFront + rc.left;
            const byte *pbS = pbSnap + (pitm->rc.Width()) * y;
            dword *pdw = &s_vdwMask[(size_t)y * cx];
            for (int x = 0; x < cx; x++, i++) {
                byte b = pbFront[i];
                byte s = pbS[x];
                int nLight;
                if (i < (int)s_vbHdCovered.size() && s_vbHdCovered[i] != 0) {
                    pdw[x] = 0;
                    fWhole = false;
                    continue;
                }
                if (b == s) {
                    nLight = 255;
                } else if (gmpiclriclrShadow != NULL && b == gmpiclriclrShadow[s] &&
                        i < (int)s_vbHdShadow.size() && s_vbHdShadow[i] != 0) {
                    nLight = 160;
                } else {
                    pdw[x] = 0;
                    fWhole = false;
                    continue;
                }
                if (pdwTC != NULL && (pdwTC[i] >> 24) != 0 && pbUnder[i] == b)
                    nLight = nLight * (255 - (int)(pdwTC[i] >> 24)) / 255;
                if (nLight != 255)
                    fWhole = false;
                pdw[x] = 0xff000000 | (nLight << 16) | (nLight << 8) | nLight;
                cVisible++;
            }
        }
        if (cVisible == 0)
            continue;
        HdTex *ptex = GetTexture(prend, pitm->ppic, pitm->side, flScale, palette);
        if (ptex == NULL)
            continue;
        if (fWhole && !pitm->fClipped && pitm->rc.left == rc.left && pitm->rc.top == rc.top &&
                pitm->rc.right == rc.right && pitm->rc.bottom == rc.bottom) {
            SDL_FRect rcPic = { xOff + pitm->x * flScale, yOff + pitm->y * flScale,
                    pitm->ppic->cx / pitm->ppic->scale * flScale, pitm->ppic->cy / pitm->ppic->scale * flScale };
            SDL_RenderTexture(prend, ptex->ptex, NULL, &rcPic);
            continue;
        }
        SDL_Rect rcUpd = { 0, 0, cx, cy };
        SDL_UpdateTexture(s_ptexMask, &rcUpd, &s_vdwMask[0], cx * 4);

        // Into the scratch target: the picture, then the mask over it

        int xW0 = (int)floorf(xOff + rc.left * flScale);
        int yW0 = (int)floorf(yOff + rc.top * flScale);
        int xW1 = (int)ceilf(xOff + rc.right * flScale);
        int yW1 = (int)ceilf(yOff + rc.bottom * flScale);
        int cxW = xW1 - xW0, cyW = yW1 - yW0;
        if (cxW > s_cxTarget) cxW = s_cxTarget;
        if (cyW > s_cyTarget) cyW = s_cyTarget;
        SDL_SetRenderTarget(prend, s_ptexTarget);
        SDL_FRect rcClear = { 0, 0, (float)cxW, (float)cyW };
        SDL_SetRenderDrawBlendMode(prend, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(prend, 0, 0, 0, 0);
        SDL_RenderFillRect(prend, &rcClear);
        SDL_FRect rcPic = { xOff + pitm->x * flScale - xW0, yOff + pitm->y * flScale - yW0,
                pitm->ppic->cx / pitm->ppic->scale * flScale, pitm->ppic->cy / pitm->ppic->scale * flScale };
        SDL_RenderTexture(prend, ptex->ptex, NULL, &rcPic);
        SDL_FRect rcMaskSrc = { 0, 0, (float)cx, (float)cy };
        SDL_FRect rcMaskDst = { xOff + rc.left * flScale - xW0, yOff + rc.top * flScale - yW0,
                cx * flScale, cy * flScale };
        SDL_RenderTexture(prend, s_ptexMask, &rcMaskSrc, &rcMaskDst);

        // Onto the screen

        SDL_SetRenderTarget(prend, ptexScreen);
        SDL_FRect rcSrc = { 0, 0, (float)cxW, (float)cyW };
        SDL_FRect rcDst = { (float)xW0, (float)yW0, (float)cxW, (float)cyW };
        SDL_RenderTexture(prend, s_ptexTarget, &rcSrc, &rcDst);
    }
}

} // namespace wi
