#ifndef __MIXER_H__
#define __MIXER_H__

#include "basictypes.h"

namespace wi {
    
struct Channel // chnl
{
    byte *pb;
    byte *pbEnd;
    byte bSampleLast;
    int nStepIndex;
};


// Mixes into signed 16 bit samples (csamples must be even). Sounds are summed
// with headroom and soft clipped rather than averaged, so each effect keeps
// its loudness when several play at once.
void MixChannels16(Channel *achnl, int cchnl, short *ps, int csamples);

} // namespace wi

#endif // __MIXER_H__
