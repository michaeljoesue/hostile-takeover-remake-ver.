#include "../ht.h"
#include <SDL3/SDL.h>

#include "../sounddevice.h"
#include "../mixer.h"

namespace wi {

// The game's sound effects are 8 kHz mono ADPCM. They're decoded and mixed
// into 16 bit stereo at 8 kHz, each placed left or right by where it happens
// on the battlefield, and SDL's audio stream resamples the mix to whatever
// the output device uses (typically 48 kHz).

#define kcChannels kcChannelsMax
#define knSourceRate 8000

extern long HostGetTickCount();
static void SDLCALL AudioCallback(void *pvUser, SDL_AudioStream *pstm,
        int cbAdditional, int cbTotal);

class SdlSoundDevice : public SoundDevice // sndd
{
public:
    SdlSoundDevice();
    virtual ~SdlSoundDevice();

    bool Init();
    virtual void Enable(bool fEnable);
    virtual bool IsEnabled();
    virtual void PlayAdpcm(int ichnl, byte *pb, word cb, int nPan, int nVolume);
    virtual int GetChannelCount();
    virtual bool IsChannelFree(int ichnl);
    virtual void ServiceProc();
    virtual bool IsSilent();
    virtual void SetVolume(int nVolume);
    virtual int GetVolume();

private:
    void Fill(SDL_AudioStream *pstm, int cbNeeded);

    bool m_fEnable;
    long m_tSilence;
    int m_nVolume;
    Channel m_achnl[kcChannels];
    SDL_AudioStream *m_pstm;
    short m_asBuffer[1024];

    friend void SDLCALL AudioCallback(void *pvUser, SDL_AudioStream *pstm,
            int cbAdditional, int cbTotal);
};

SoundDevice *CreateSdlSoundDevice()
{
    SdlSoundDevice *psndd = new SdlSoundDevice;
    if (psndd == NULL)
        return NULL;
    if (!psndd->Init()) {
        delete psndd;
        return NULL;
    }
    return (SoundDevice *)psndd;
}

SdlSoundDevice::SdlSoundDevice()
{
    m_fEnable = false;
    m_tSilence = 0;
    m_nVolume = 255;
    m_pstm = NULL;
    memset(m_achnl, 0, sizeof(m_achnl));
}

SdlSoundDevice::~SdlSoundDevice()
{
    if (m_pstm != NULL) {
        SDL_DestroyAudioStream(m_pstm);
        m_pstm = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool SdlSoundDevice::Init()
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        LOG() << "Couldn't initialize SDL audio: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = knSourceRate;
    m_pstm = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &spec, AudioCallback, this);
    if (m_pstm == NULL) {
        LOG() << "Couldn't open audio device: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec specDevice;
    if (SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(m_pstm),
            &specDevice, NULL)) {
        LOG() << "Audio output " << specDevice.freq << " Hz, "
                << specDevice.channels << " channel(s)";
    }
    return true;
}

bool SdlSoundDevice::IsEnabled()
{
    return m_fEnable;
}

void SdlSoundDevice::Enable(bool fEnable)
{
    SDL_LockAudioStream(m_pstm);
    if (fEnable != m_fEnable) {
        memset(m_achnl, 0, sizeof(m_achnl));
        m_tSilence = 0;
        m_fEnable = fEnable;
    }
    SDL_UnlockAudioStream(m_pstm);

    if (fEnable) {
        SDL_ResumeAudioStreamDevice(m_pstm);
    } else {
        SDL_PauseAudioStreamDevice(m_pstm);
    }
}

int SdlSoundDevice::GetChannelCount()
{
    return kcChannels;
}

bool SdlSoundDevice::IsChannelFree(int ichnl)
{
    if (ichnl < 0 || ichnl >= kcChannels)
        return false;
    SDL_LockAudioStream(m_pstm);
    Channel *pchnl = &m_achnl[ichnl];
    bool fFree = (pchnl->pb >= pchnl->pbEnd);
    SDL_UnlockAudioStream(m_pstm);
    return fFree;
}

bool SdlSoundDevice::IsSilent()
{
    if (!m_fEnable)
        return true;
    if (m_tSilence == 0)
        return true;
    long tCurrent = HostGetTickCount();
    if (tCurrent < 0 && m_tSilence >= 0)
        return true;
    return tCurrent >= m_tSilence;
}

void SdlSoundDevice::ServiceProc()
{
}

void SdlSoundDevice::PlayAdpcm(int ichnl, byte *pb, word cb, int nPan,
        int nVolume)
{
    if (ichnl < 0 || ichnl >= kcChannels)
        return;
    if (!m_fEnable)
        return;

    SDL_LockAudioStream(m_pstm);
	Channel *pchnl = &m_achnl[ichnl];
	pchnl->pb = pb;
	pchnl->pbEnd = pb + cb;
	pchnl->nStepIndex = 0;
	pchnl->bSampleLast = 128;

    // Constant-power-ish pan: centre is full in both ears, a sound far to
    // one side is quiet in the other ear but not silent

    nPan = SDL_clamp(nPan, -256, 256);
    nVolume = SDL_clamp(nVolume, 0, 256);
    int nLeft = nPan <= 0 ? 256 : 256 - nPan * 3 / 4;
    int nRight = nPan >= 0 ? 256 : 256 + nPan * 3 / 4;
    pchnl->nGainLeft = nLeft * nVolume / 256;
    pchnl->nGainRight = nRight * nVolume / 256;

    // Each byte is 2 samples at 8 kHz, i.e. 1/40th of a 10ms tick. Allow a
    // little extra for the output latency.
	m_tSilence = HostGetTickCount() + (cb + 39) / 40 + 5;
    SDL_UnlockAudioStream(m_pstm);
}

void SdlSoundDevice::SetVolume(int nVolume)
{
    if (nVolume < 0)
        nVolume = 0;
    if (nVolume > 255)
        nVolume = 255;
    m_nVolume = nVolume;
    if (m_pstm != NULL)
        SDL_SetAudioStreamGain(m_pstm, nVolume / 255.0f);
}

int SdlSoundDevice::GetVolume()
{
	return m_nVolume;
}

// Called on SDL's audio thread with the stream locked

void SdlSoundDevice::Fill(SDL_AudioStream *pstm, int cbNeeded)
{
    const int cbFrame = 2 * (int)sizeof(short);
    while (cbNeeded > 0) {
        int cframes = (cbNeeded + cbFrame - 1) / cbFrame;
        cframes = (cframes + 1) & ~1;
        if (cframes > ARRAYSIZE(m_asBuffer) / 2)
            cframes = ARRAYSIZE(m_asBuffer) / 2;
        if (m_fEnable) {
            MixChannelsStereo16(m_achnl, kcChannels, m_asBuffer, cframes);
        } else {
            memset(m_asBuffer, 0, cframes * cbFrame);
        }
        SDL_PutAudioStreamData(pstm, m_asBuffer, cframes * cbFrame);
        cbNeeded -= cframes * cbFrame;
    }
}

static void SDLCALL AudioCallback(void *pvUser, SDL_AudioStream *pstm,
        int cbAdditional, int cbTotal)
{
    if (cbAdditional > 0)
        ((SdlSoundDevice *)pvUser)->Fill(pstm, cbAdditional);
}

} // namespace wi
