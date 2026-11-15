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
    int nGainLeft;  // 0 to 256 (full)
    int nGainRight;
};


// Mixes into signed 16 bit samples (csamples must be even). Sounds are summed
// with headroom and soft clipped rather than averaged, so each effect keeps
// its loudness when several play at once.
void MixChannels16(Channel *achnl, int cchnl, short *ps, int csamples);

// The same into interleaved stereo frames (cframes must be even), each
// channel placed by its left and right gains.
void MixChannelsStereo16(Channel *achnl, int cchnl, short *ps, int cframes);

} // namespace wi

#endif // __MIXER_H__
