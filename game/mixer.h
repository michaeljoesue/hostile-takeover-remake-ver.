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


// Mixes into interleaved stereo frames (cframes must be even), each channel
// placed by its left and right gains, as unclipped sums. Sounds are summed
// with headroom rather than averaged, so each effect keeps its loudness when
// several play at once; SoftClip16 then soft clips a sum to 16 bits.
void MixChannelsStereo32(Channel *achnl, int cchnl, int *pn, int cframes);
short SoftClip16(int n);

} // namespace wi

#endif // __MIXER_H__
