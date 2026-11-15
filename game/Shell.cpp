#include "game/ht.h"
#include "base/misc.h"

namespace wi {

Shell gshl;
Shell::Shell()
{
	m_ppal = NULL;
	m_mpiclriclrShadow = NULL;
}

bool Shell::Init()
{
	m_ppal = (Palette *)gpakr.MapFile("shell.palbin", &m_fmapPalette);
	Assert(m_ppal != NULL);
	m_mpiclriclrShadow = (byte *)gpakr.MapFile("shell.palbin.shadowmap", &m_fmapShadowMap);
	Assert(m_mpiclriclrShadow != NULL);
	return true;
}

void Shell::Exit()
{
	if (m_ppal != NULL)
		gpakr.UnmapFile(&m_fmapPalette);
	if (m_mpiclriclrShadow != NULL)
		gpakr.UnmapFile(&m_fmapShadowMap);
}

void Shell::SetPalette()
{
	gmpiclriclrShadow = m_mpiclriclrShadow;
	SetHslAdjustedPalette(m_ppal, gnHueOffset, gnSatMultiplier, gnLumOffset);
}

int Shell::PlayGame(PlayMode pm, MissionIdentifier *pmiid, Stream *pstm,
        int nRank)
{
	// UNDONE: free up space-taking Shell resources

	int nGo;

	do {
		nGo = knGoSuccess;

		switch (pm) {
		case kpmNormal:
			nGo = ggame.PlayLevel(pmiid, NULL, nRank);
			break;

		case kpmSavedGame:
			nGo = ggame.PlaySavedGame(pstm);
			break;
		}

		if (nGo == knGoLoadSavedGame) {
			pm = kpmSavedGame;
			pstm = gpstmSavedGame;
 		}
	} while (nGo == knGoLoadSavedGame);

	// UNDONE: reload space-taking Shell resources

	gmpiclriclrShadow = m_mpiclriclrShadow;

	return nGo;
}

void Shell::Launch(bool fLoadReinitializeSave, MissionIdentifier *pmiid)
{
#ifdef STRESS
	static char *s_aszStressLevels[] = {
		"S_01.lvl", "S_02.lvl", "S_03.lvl", "S_04.lvl", "S_05.lvl", "S_06.lvl", "S_07.lvl", 
		"S_08.lvl", "S_09.lvl", "S_10.lvl", "S_11.lvl", "S_12.lvl", "S_13.lvl", "S_14.lvl"
	};

	if (gfStress) {
		while (true) {
			gtStressTimeout = 30 * 60 * 100L;		// 30 minutes
			int ilvl = GetAsyncRandom() % ARRAYSIZE(s_aszStressLevels);
            strncpyz(pmiid->szLvlFilename, s_aszStressLevels[ilvl],
                    sizeof(pmiid->szLvlFilename));
			int nGo = PlayGame(kpmNormal, pmiid, NULL, 0);
			if (nGo == knGoAppStop)
				return;
		}
	}
#endif // def STRESS



	if (gevm.IsAppStopping())
		return;

	// Are we launching immediately into a saved game? A level that was asked
	// for (e.g. from the map editor) comes first.

	Stream *pstm = NULL;
	if (pmiid == NULL)
		pstm = HostOpenSaveGameStream(knGameReinitializeSave, true);

	if (pstm != NULL) {

		// Stream is closed/deleted upon return
		
		int nGo = PlayGame(kpmSavedGame, NULL, pstm, 0);
		if (nGo == knGoAppStop)
			return;

		//if (nGo == knGoInitFailure)
		// silently fall into a normal game open

	} else if (pmiid != NULL) {
		int nGo = PlayGame(kpmNormal, pmiid, NULL, 0);
		if (nGo == knGoAppStop)
			return;

		if (nGo == knGoInitFailure)
			HtMessageBox(kfMbWhiteBorder, "Load Game", "Error launch level!");
	}

	while (true) {
        if (gevm.IsAppStopping()) {
            return;
        }

        // Show the main menu (play, load saved game, options, help)

		ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms,
                kidfStartup, new ShellForm());
		Assert(pfrm != NULL);
		if (pfrm == NULL)
			return;

		// Make Shell palette and shadow map active

		gshl.SetPalette();

		int idc;
		pfrm->DoModal(&idc);
		delete pfrm;

		// While the dialog was up the player might have exited the app

		if (gevm.IsAppStopping())
			return;

		switch (idc) {
		case kidcPlay:
			if (PlaySinglePlayer())
				return;
			break;

		case kidcSetupGame:
			DoModalGameOptionsForm(m_ppal, false);
			break;

		case kidcHelp:
			Help();
			break;

		case kidcCredits:
			Help("credits");
			break;

		case kidcLoadSavedGame:
			{
				Stream *pstm = PickLoadGameStream();
				if (pstm == NULL)
					break;

				// Stream is closed/deleted upon return

				int nGo = PlayGame(kpmSavedGame, NULL, pstm, 0);

				if (nGo == knGoAppStop)
					return;

				if (nGo == knGoInitFailure)
					HtMessageBox(kfMbWhiteBorder | kfMbClearDib, "Load Game", "Error loading saved game!");
			}
			break;

		case kidcExitGame:
			return;
		}
	}
}

bool Shell::PlaySinglePlayer()
{
    MissionIdentifier miidFind;
    memset(&miidFind, 0, sizeof(miidFind));

    while (true) {
        // First, create a mission list, which is an enumerator for the
        // missions we want to show in this form.

        MissionList *pml = CreateMissionList();
        if (pml == NULL) {
            return true;
        }
		SelectMissionForm *pfrm = (SelectMissionForm *)gpmfrmm->LoadForm(
                gpiniForms, kidfSelectMissionWide,
                new SelectMissionForm(pml, &miidFind));
		if (pfrm == NULL) {
            delete pml;
			return true;
        }

		int idc;
		pfrm->DoModal(&idc);

        MissionIdentifier miid;
        bool fMissionSelected = false;
        if (idc == kidcOk) {
            if (pfrm->GetSelectedMission(&miid)) {
                miidFind = miid;
                fMissionSelected = true;
            }
        }
            
		delete pfrm;
        delete pml;

		// While the dialog was up the player might have exited the app

		if (gevm.IsAppStopping())
			return true;

		if (!fMissionSelected)
			return false;

		int nGo = PlayGame(kpmNormal, &miid, NULL, 0);

        // The next time the SelectMission form shows, highlight the last
        // mission the user was playing.

        miidFind = ggame.GetLastMissionIdentifier();

		if (nGo == knGoAppStop) {
			return true;
        }

		if (nGo == knGoInitFailure) {
			HtMessageBox(kfMbWhiteBorder, "Error", "Unable to load and initialize the mission.");
			continue;
		}
	}

    return false;
}

//
// ShellForm implementation
//

ShellForm::ShellForm()
{
	m_fCached = false;
	m_fAnimate = true;
	m_fTimerEnabled = false;
}

ShellForm::~ShellForm()
{
	if (m_fTimerEnabled)
		gtimm.RemoveTimer(this);
}

bool ShellForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!Form::Init(pfrmm, pini, idf))
		return false;

	// Keep invisible until the controls are at the right place

	Show(false);

	// Shell forms draw over the whole screen, so size the form to be full screen.
	// This way it is a full screen opaquing form when it comes up, which means things
	// won't try to draw behind it, slowing the game down

	DibBitmap *pbm = m_pfrmm->GetDib();
	Size siz;
	pbm->GetSize(&siz);

	int xNew = (siz.cx - m_rc.Width()) / 2;
	int yNew = (siz.cy - m_rc.Height()) / 2;

	// Reposition the controls

	for (int n = 0; n < m_cctl; n++) {
		Control *pctl = m_apctl[n];
		Rect rcCtl;
		pctl->GetRect(&rcCtl);
		int xNewCtl = rcCtl.left + xNew;
		int yNewCtl = rcCtl.top + yNew;
		pctl->SetPosition(xNewCtl, yNewCtl);
	}
	Rect rcNew;
	rcNew.Set(0, 0, siz.cx, siz.cy);
	SetRect(&rcNew);

	// Set version string - hack

	if (m_idf == kidfStartup) {
		LabelControl *pctl = (LabelControl *)GetControlPtr(kidcVersion);
		if (pctl != NULL) {
			// If no version string, exe+data are unmarked. Show date/time
			// Use the version string verbatim so we show extra text at the end

			char szT[64];
			if (!ggame.GetFormattedVersionString(gszVersion, szT)) {
				szT[0] = 0;
#ifdef DEV_BUILD
				strcat(szT, "DEV BUILD ");
#endif
				strcat(szT, __DATE__);
				strcat(szT, ", ");
				strcat(szT, __TIME__);
			} else {
				szT[0] = 'v';
				strncpyz(&szT[1], gszVersion, sizeof(szT));
			}
			pctl->SetText(szT);
		}
	}

	return true;
}

#define kctRate 2
#define kcFcZip 18
bool ShellForm::DoModal(int *pnResult, bool fAnimate, bool fShowSound)
{
	m_fAnimate = fAnimate;
	if (!m_fAnimate) {
		if (fShowSound)
			gsndm.PlaySfx(ksfxGuiFormShow);
		return Form::DoModal(pnResult, (Sfx)-1, (Sfx)-1);
	}

	// Take over form show sound playing

	Size siz;
	ggame.GetPlayfieldSize(&siz);

	// Reposition all the form's controls of the form so they can be zipped in

	for (int i = 0; i < m_cctl; i++) {
		Control *pctl = m_apctl[i];
		
		Rect rcCtl;
		pctl->GetRect(&rcCtl);

		// Remember where the control is supposed to end up

		m_axDst[i] = (rcCtl.left & ~1);

		// Odd index controls fly in from the left, even from the right

		if (i & 1) {
			pctl->SetPosition(rcCtl.left - siz.cx - (i * PcFromFc(kcFcZip)), rcCtl.top);
		} else {
			pctl->SetPosition(rcCtl.left + siz.cx + (i * PcFromFc(kcFcZip)), rcCtl.top);
		}
	}

	// Now that the controls are positioned right make the form visible

	Show(true);

	if (fShowSound)
		m_wf |= kfFrmShowSound;

	// Start timer

	gtimm.AddTimer(this, kctRate);
	m_fTimerEnabled = true;
	m_fCached = false;
	m_tLast = 0;

	// Some controls might be added on the fly after ShellForm::Init. Since we 
	// won't have their destination stashed away we'll just ignore them.

	m_cctlToZip = m_cctl;

	return Form::DoModal(pnResult, (Sfx)-1, (Sfx)-1);
}

void ShellForm::OnTimer(long tCurrent)
{
	Assert(m_fAnimate);

	int ct = (int)(tCurrent - m_tLast);
	m_tLast = tCurrent;

	// Waiting until all the pieces have been cached before doing absolute
	// timing. This gives smoother movement.

	int pcMove = PcFromFc(kcFcZip);
	if (m_fCached)
		pcMove = ((ct + kctRate / 2) / kctRate) * PcFromFc(kcFcZip);

	bool fZipping = false;

	for (int i = 0; i < m_cctlToZip; i++) {
		Control *pctl = m_apctl[i];

		Rect rcCtl;
		pctl->GetRect(&rcCtl);

		int xDst = m_axDst[i];
		if (rcCtl.left == xDst)
			continue;

		fZipping = true;

		int x;
		if (xDst > rcCtl.left)
			x = _min(rcCtl.left + pcMove, xDst);
		else
			x = _max(rcCtl.left - pcMove, xDst);

		pctl->SetPosition(x & ~1, rcCtl.top);
	}

	gevm.SetRedrawFlags(kfRedrawDirty | kfRedrawBeforeTimer);

	if (!fZipping) {
		gtimm.RemoveTimer(this);
		m_fTimerEnabled = false;
		if (m_wf & kfFrmShowSound)
			gsndm.PlaySfx(ksfxGuiFormShow);
        OnZipDone();
	}
}

void ShellForm::OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd)
{
	// Draw the fancy background bitmap

	Size sizDib;
	pbm->GetSize(&sizDib);
	RawBitmap *prbm = LoadRawBitmap("titlescreenbkgd.rbm");
	Size sizBmp;
	prbm->GetSize(&sizBmp);
    
    // Tile the bitmap over the whole screen, with one copy centered. Larger
    // screens than the original devices get more of the pattern instead of
    // black borders.

	int xCenter = ((sizDib.cx - sizBmp.cx) / 2) & ~1;
	int yCenter = (sizDib.cy - sizBmp.cy) / 2;
	int xStart = xCenter;
	while (xStart > 0)
		xStart -= sizBmp.cx;
	int yStart = yCenter;
	while (yStart > 0)
		yStart -= sizBmp.cy;
	for (int y = yStart; y < sizDib.cy; y += sizBmp.cy) {
		for (int x = xStart; x < sizDib.cx; x += sizBmp.cx) {
			if (pupd != NULL) {
				// Update rects are always on screen, so this clips
				BltHelper(pbm, prbm, pupd, x, y);
				continue;
			}
			Rect rcDst;
			rcDst.Set(_max(x, 0), _max(y, 0), _min(x + sizBmp.cx, sizDib.cx),
					_min(y + sizBmp.cy, sizDib.cy));
			if (rcDst.IsEmpty())
				continue;
			Rect rcSrc;
			rcSrc.Set(rcDst.left - x, rcDst.top - y, rcDst.right - x,
					rcDst.bottom - y);
			prbm->BltTo(pbm, rcDst.left, rcDst.top, &rcSrc);
		}
	}

	delete prbm;

	// Don't start timing for absolute positioned animation until we've
	// loaded the cache at least once. Gives smoother animation

	if (!m_fCached) {
		m_fCached = true;
		m_tLast = gtimm.GetTickCount();
	}
}

} // namespace wi
