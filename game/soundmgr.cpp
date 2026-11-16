#include "ht.h"

namespace wi {

SoundMgr gsndm;

// The same sound isn't started again within this long of itself
#define kmsSameSoundMin 40

SoundMgr::SoundMgr()
{
	m_apcmh = NULL;
	m_cpcmh = 0;
	m_asfxe = NULL;
	m_csfxe = 0;
	m_afmap = NULL;
	m_psndd = NULL;
	m_cChannels = 0;
	memset(m_anPriorityChannel, 0, sizeof(m_anPriorityChannel));
	memset(m_anSoundChannel, 0xff, sizeof(m_anSoundChannel));
	memset(m_anVolumeChannel, 0, sizeof(m_anVolumeChannel));
	memset(m_amsStartChannel, 0, sizeof(m_amsStartChannel));
}

SoundMgr::~SoundMgr()
{
	Assert(m_psndd == NULL);
}

bool SoundMgr::Init()
{
	// Open the sound device

	m_psndd = HostOpenSoundDevice();
	if (m_psndd == NULL)
		return false;

	FileMap fmap;
	byte *pbFiles = (byte *)gpakr.MapFile("soundfiles", &fmap);
	if (pbFiles == NULL)
		return false;

	// Load sounds - get count and load arrays

	bool fSuccess = true;
	m_cpcmh = BigWord(*(word *)pbFiles);
	pbFiles += 2;
	m_apcmh = new PcmHeader[m_cpcmh];
	if (m_apcmh == NULL) {
		gpakr.UnmapFile(&fmap);
		return false;
	}
	memset(m_apcmh, 0, sizeof(PcmHeader) * m_cpcmh);
	m_afmap = new FileMap[m_cpcmh];
	if (m_afmap == NULL) {
		gpakr.UnmapFile(&fmap);
		return false;
	}
	memset(m_afmap, 0, sizeof(FileMap) * m_cpcmh);

	// Map the sound files

	char *psz = (char *)pbFiles;
	for (int i = 0; i < m_cpcmh; i++) {
		dword cb;
		m_apcmh[i].pb = (byte *)gpakr.MapFile(psz, &m_afmap[i], &cb);
		if (m_apcmh[i].pb == NULL) {
			fSuccess = false;
			break;
		}
		m_apcmh[i].cb = (word)cb;
		if (i < m_cpcmh - 1)
			psz += strlen(psz) + 1;
	}
	gpakr.UnmapFile(&fmap);
	if (!fSuccess)
		return false;

	// Map SfxEntries

	dword cb;
	m_asfxe = (SfxEntry *)gpakr.MapFile("SfxEntries", &m_fmapSfxEntries, &cb);
	if (m_asfxe == NULL)
		return false;
	m_csfxe = cb / kcbSfxEntry;

	// Turn on sound device

	m_cChannels = m_psndd->GetChannelCount();

	return true;
}

void SoundMgr::Exit()
{
	delete m_psndd;
	m_psndd = NULL;

	for (int n = 0; n < m_cpcmh; n++) {
		if (m_apcmh[n].pb != NULL) {
			gpakr.UnmapFile(&m_afmap[n]);
			m_apcmh[n].pb = NULL;
		}
	}
	delete[] m_apcmh;
	m_apcmh = NULL;
	delete[] m_afmap;
	m_afmap = NULL;
	m_cpcmh = 0;

	if (m_asfxe != NULL) {
		gpakr.UnmapFile(&m_fmapSfxEntries);
		m_asfxe = NULL;
	}
}

void SoundMgr::Enable(bool fEnable)
{
	if (m_psndd != NULL)
		m_psndd->Enable(fEnable);
}

bool SoundMgr::IsEnabled()
{
	if (m_psndd != NULL)
		return m_psndd->IsEnabled();
	return false;
}

// Battle sounds come from where they happen: panned left or right of the
// centre of the view, and quieter the further off screen they are (but
// never silent, so the player can tell where a fight is).

int gnVolumeEffects = 256;
int gnVolumeBattle = 256;
int gnVolumeResponses = 256;
int gnVolumeMenu = 256;

void SoundMgr::PlayResponseSfx(Sfx sfx)
{
	if (gnVolumeResponses > 0)
		PlaySfxRaw(sfx, 0, gnVolumeResponses);
}

// Interface clicks are effects; other sounds played this way (build menus,
// building select / build / ready sounds, alerts, credits, win / lose) are
// menus & alerts

void SoundMgr::PlaySfx(Sfx sfx, int nPan, int nVolume)
{
	bool fEffect = sfx >= ksfxGuiBuildMenuHide && sfx <= ksfxGuiScrollingListSelectItem;
	int nGroup = fEffect ? gnVolumeEffects : gnVolumeMenu;
	if (nGroup <= 0)
		return;
	PlaySfxRaw(sfx, nPan, nVolume * nGroup / 256);
}

void SoundMgr::PlaySfxAt(Sfx sfx, WCoord wx, WCoord wy)
{
	if (gnVolumeBattle <= 0)
		return;
	Size sizPlayfield;
	ggame.GetPlayfieldSize(&sizPlayfield);
	WCoord wxView, wyView;
	gsim.GetViewPos(&wxView, &wyView);
	int cxHalf = WcFromPc(sizPlayfield.cx) / 2;
	int cyHalf = WcFromPc(sizPlayfield.cy) / 2;
	if (cxHalf <= 0 || cyHalf <= 0) {
		PlaySfxRaw(sfx, 0, gnVolumeBattle);
		return;
	}
	int dx = (int)wx - ((int)wxView + cxHalf);
	int dy = (int)wy - ((int)wyView + cyHalf);

	// Pan: strongly to the side the sound is on - the edges of the view
	// are nearly all the way (7/8), further off screen fully

	int nPan = dx * 224 / cxHalf;
	if (nPan > 256)
		nPan = 256;
	if (nPan < -256)
		nPan = -256;

	// Volume: full on screen, falling to a quarter by 3 screens away

	int nOutX = abs(dx) - cxHalf;
	int nOutY = abs(dy) - cyHalf;
	int nOut = 0;
	if (nOutX > 0)
		nOut = nOutX * 256 / cxHalf;
	if (nOutY > 0)
		nOut = _max(nOut, nOutY * 256 / cyHalf);
	int nVolume = 256 - nOut * 192 / (256 * 4);
	if (nVolume < 64)
		nVolume = 64;
	PlaySfxRaw(sfx, nPan, nVolume * gnVolumeBattle / 256);
}

void SoundMgr::PlaySfxRaw(Sfx sfx, int nPan, int nVolume)
{
	// Get the sfx entry and channel

	if (m_psndd == NULL)
		return;
	if (!m_psndd->IsEnabled())
		return;
	int nSfx = (int)sfx;
	if (nSfx < 0 || nSfx > m_csfxe)
		return;
	SfxEntry *psfxe = (SfxEntry *)((byte *)m_asfxe + sfx * kcbSfxEntry);
	if (psfxe->nSound == 0xff)
		return;

	// Choose a channel. In a big battle there are more sounds than
	// channels, so:
	// - the same sound starting again within a moment of itself isn't
	//   started twice (a dozen identical shots sound like one, and would
	//   otherwise fill every channel)
	// - a free channel is used if there is one
	// - otherwise the least important sound is replaced: a lower priority
	//   one, or one of the same priority that is quieter (further away) or,
	//   at the same volume, has been playing longest
	// Important sounds (alerts, speech) are never cut off by battle noise.

	dword dwBusy = m_psndd->GetBusyChannels();
	long msNow = HostGetMillisecondCount();
	int ichnlUse = -1;
	for (int ichnl = 0; ichnl < m_cChannels; ichnl++) {
		if (!(dwBusy & ((dword)1 << ichnl)))
			continue;
		if (m_anSoundChannel[ichnl] == psfxe->nSound &&
				msNow - m_amsStartChannel[ichnl] < kmsSameSoundMin &&
				m_anVolumeChannel[ichnl] >= nVolume)
			return;
	}
	for (int ichnl = 0; ichnl < m_cChannels; ichnl++) {
		if (!(dwBusy & ((dword)1 << ichnl))) {
			ichnlUse = ichnl;
			break;
		}
	}
	if (ichnlUse == -1) {
		for (int ichnl = 0; ichnl < m_cChannels; ichnl++) {
			// Higher numbers are lower priority
			if (m_anPriorityChannel[ichnl] < psfxe->nPriority)
				continue;
			if (ichnlUse == -1) {
				ichnlUse = ichnl;
				continue;
			}
			if (m_anPriorityChannel[ichnl] != m_anPriorityChannel[ichnlUse]) {
				if (m_anPriorityChannel[ichnl] > m_anPriorityChannel[ichnlUse])
					ichnlUse = ichnl;
				continue;
			}
			if (m_anVolumeChannel[ichnl] != m_anVolumeChannel[ichnlUse]) {
				if (m_anVolumeChannel[ichnl] < m_anVolumeChannel[ichnlUse])
					ichnlUse = ichnl;
				continue;
			}
			if (m_amsStartChannel[ichnl] - m_amsStartChannel[ichnlUse] < 0)
				ichnlUse = ichnl;
		}
		if (ichnlUse == -1)
			return;

		// A quieter sound of the same priority doesn't replace a louder one

		if (m_anPriorityChannel[ichnlUse] == psfxe->nPriority &&
				m_anVolumeChannel[ichnlUse] > nVolume)
			return;
	}
	m_anPriorityChannel[ichnlUse] = psfxe->nPriority;
	m_anSoundChannel[ichnlUse] = psfxe->nSound;
	m_anVolumeChannel[ichnlUse] = (short)nVolume;
	m_amsStartChannel[ichnlUse] = msNow;

	// Play the sound

	PcmHeader *pcmh = &m_apcmh[psfxe->nSound];
	m_psndd->PlayAdpcm(ichnlUse, pcmh->pb, pcmh->cb, nPan, nVolume);
}

void SoundMgr::WaitSilence()
{
	if (m_psndd == NULL)
		return;
	while (!m_psndd->IsSilent()) {
		HostSleep(1);
	}
}

long SoundMgr::FilterSleepTicks(long ct)
{
	// HACK:
    // On PocketPC sound is processed on an OS callback so we don't need to
    // limit sleeping Further, if we DO limit sleeping we will cause problems
    // for the background Bluetooth communications threads.

	return ct;
}

void SoundMgr::SetVolume(int nVolume)
{
	if (m_psndd != NULL)
		m_psndd->SetVolume(nVolume);
}

int SoundMgr::GetVolume()
{
	if (m_psndd != NULL)
		return m_psndd->GetVolume();
	return -1;
}

} // namespace wi
