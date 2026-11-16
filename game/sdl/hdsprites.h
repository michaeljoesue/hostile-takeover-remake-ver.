#ifndef __HDSPRITES_H__
#define __HDSPRITES_H__

#include "inc/basictypes.h"
#include <SDL3/SDL.h>

// HD structures (see hdsprites.cpp). The game-side calls are declared in ht.h.

namespace wi {

void HdToggle();
bool HdHasItems();
void HdComposite(SDL_Renderer *prend, float flScale, float xOff, float yOff, const byte *pbFront,
        int cxFront, int cyFront, const dword *palette, const dword *pdwTC, const byte *pbUnder);

} // namespace wi

#endif // __HDSPRITES_H__
