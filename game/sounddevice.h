#ifndef __SOUNDDEVICE_H__
#define __SOUNDDEVICE_H__

#include "basictypes.h"

namespace wi {

class SoundDevice // sndd
{
public:
    virtual ~SoundDevice() = 0;
    virtual void Enable(bool fEnable) = 0;
    virtual bool IsEnabled() = 0;
    // nPan is -256 (left) to 256 (right), nVolume 0 to 256 (full)
    virtual void PlayAdpcm(int ichnl, byte *pb, word cb, int nPan = 0,
            int nVolume = 256) = 0;
    virtual int GetChannelCount() = 0;
    virtual bool IsChannelFree(int ichnl) = 0;
    // Bit n set if channel n is still playing (one lock for all channels)
    virtual dword GetBusyChannels() = 0;
    virtual void ServiceProc() = 0;
    virtual bool IsSilent() = 0;
    virtual void SetVolume(int nVolume) = 0;
    virtual int GetVolume() = 0;
};

} // namespace wi

#endif // __SOUNDDEVICE_H__
