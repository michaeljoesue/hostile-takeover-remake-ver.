#include "game/sdl/sdlselectionsprite.h"

#define GRABBIE_SIZE 10
#define GRABBIE_SIZE_HALF GRABBIE_SIZE/2

namespace wi {

SdlSelectionSprite::SdlSelectionSprite(SpriteManager *psprm) {
    fVisible_ = false;
    psprm_ = psprm;
    psprm_->Add(this);
}

SdlSelectionSprite::~SdlSelectionSprite() {
    psprm_->Remove(this);
}

void SdlSelectionSprite::SetDragRect(const DragRect& drc) {
    crit_.Enter();
    drc_ = drc;
    crit_.Leave();
    psprm_->Update(this);
}

const DragRect& SdlSelectionSprite::GetDragRect() {
    return drc_;
}

void SdlSelectionSprite::Show(bool fShow) {
    if (fVisible_ == fShow) {
        return;
    }
    fVisible_ = fShow;
    psprm_->Update(this);
}

void SdlSelectionSprite::Draw(void *pv, Size *psiz) {
    if (!fVisible_) {
        return;
    }

    crit_.Enter();

    DPoint apt[4];
    drc_.GetPoints(apt);

    #define renderer (SDL_Renderer *)pv
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < ARRAYSIZE(apt); i++) {
        int next = i + 1 == ARRAYSIZE(apt) ? 0 : i + 1;

        // x/y are swapped
        Point pt1 = {(int)apt[i].y, (int)apt[i].x};
        Point pt2 = {(int)apt[next].y, (int)apt[next].x};

        // draw the selection sprite line
        SDL_RenderLine(renderer, (float)pt1.x, (float)pt1.y, (float)pt2.x, (float)pt2.y);

        // create and draw a grabbie rect
        SDL_FRect rect = {(float)(pt1.x - GRABBIE_SIZE_HALF),
            (float)(pt1.y - GRABBIE_SIZE_HALF), GRABBIE_SIZE, GRABBIE_SIZE};
        SDL_RenderFillRect(renderer, &rect);
    }

    crit_.Leave();
}

} // namespace wi
