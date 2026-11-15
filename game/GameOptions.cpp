#include "ht.h"

namespace wi {

// Handy dandy global flags used for enabling various test features

#ifdef DRAW_PATHS
bool gfDrawPaths = false;
#endif

#ifdef DRAW_LINES
bool gfDrawLines = false;
#endif

#ifdef STATS_DISPLAY
bool gfShowStats = false;
#endif

bool gfOvermindEnabled = true;
bool gfSoundEnabled = true;

#ifdef DEV_BUILD
bool gfShowFPS = true;
#else
bool gfShowFPS = false;
#endif

bool gfSuspendUpdates = false;
bool gfSingleStep = false;
bool gfGodMode = false;
bool gfAutosave = false;
bool gfStylusUI = false;

#define SetControlChecked(id, f) ((CheckBoxControl *)GetControlPtr(id))->SetChecked(f)
#define GetControlChecked(id) ((CheckBoxControl *)GetControlPtr(id))->IsChecked()

// GameOptions

class GameOptionsForm : public ShellForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);
};

// SoundOptions

class SoundOptionsForm : public ShellForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	void InitResettableControls();
	void UpdateLabels();

	int m_nVolume;
	bool m_fEnabled;
};

// ColorOptions

class ColorOptionsForm : public Form
{
public:
	ColorOptionsForm(Palette *ppal);
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual void OnControlSelected(word idc);

private:
	void InitResettableControls();
	void UpdateLabels();

	Palette *m_ppal;
	int m_nHueOffset;
	int m_nSatMultiplier;
	int m_nLumOffset;
};

// DisplayOptions

class DisplayOptionsForm : public ShellForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	void InitResettableControls();
};

// PerformanceOptions

class PerformanceOptionsForm : public ShellForm
{
public:
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);
	virtual void OnControlSelected(word idc);

private:
	void InitResettableControls();

	word m_wfPerfOptions;
};

// GobCountForm

class GobCountForm : public Form
{
public:
	virtual void OnPaint(DibBitmap *pbm);
	virtual void OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque);
	virtual bool OnPenEvent(Event *pevt);

private:
	void OutputString(DibBitmap *pbm, char *psz, int *px, int *py);
	void FormatSideString(char *pszIn, int *acPerSide, char *pszOut);
};

// +++

bool DoModalGameOptionsForm(Palette *ppal, bool fInGame)
{
	ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms, kidfGameOptions, new GameOptionsForm());
	if (pfrm == NULL)
		return false;
	int nResult;
	pfrm->DoModal(&nResult);

	if (!fInGame)
		pfrm->Show(true);

	if (nResult == kidcColorOptions) {
		gpmfrmm->InvalidateRect(NULL);
		Form *pfrmT = gpmfrmm->LoadForm(gpiniForms, kidfColorOptions, new ColorOptionsForm(ppal));
		if (pfrmT != NULL) {
			pfrmT->DoModal();
			delete pfrmT;
		}
	}

	pfrm->Show(false);
	delete pfrm;
	return true;
}

//
// Format option buttons sequentially, based on visibility
//

void FormatButtons(Form *pfrm, word *aidc, int cidc, int idcRef, int idcRefNext)
{
	Control *pctlRef = pfrm->GetControlPtr(idcRef);
	Rect rcRef;
	pctlRef->GetRect(&rcRef);

	Control *pctlRefNext = pfrm->GetControlPtr(idcRefNext);
	Rect rcRefNext;
	pctlRefNext->GetRect(&rcRefNext);

	int x = rcRef.left;
	int y = rcRef.top;
	int cySpacing = rcRefNext.top - rcRef.top;

	for (int n = 0; n < cidc; n++) {
		Control *pctl = pfrm->GetControlPtr(aidc[n]);
		if (!(pctl->GetFlags() & kfCtlVisible))
			continue;
		Rect rc;
		pctl->GetRect(&rc);
		rc.left = x;
		rc.bottom = y + rc.bottom - rc.top;
		rc.top = y;
		pctl->SetRect(&rc);
		y += cySpacing;
	}
}

//
// GameOptions implementation
//

bool GameOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;

	// No color options when screen depth is 4

	ModeInfo mode;
	gpdisp->GetMode(&mode);
	if (mode.nDepth == 4)
		GetControlPtr(kidcColorOptions)->Show(false);

	// No display options when there is only one choice

	if (ggame.GetModeMatchCount() == 1)
		GetControlPtr(kidcDisplayOptions)->Show(false);

	// Format the remaining visible buttons

	word aidcFormat[] = { kidcInGameOptions, kidcSoundOptions, kidcPerformanceOptions, kidcColorOptions, kidcDisplayOptions };
	FormatButtons(this, aidcFormat, ARRAYSIZE(aidcFormat), kidcInGameOptions, kidcSoundOptions);

	return true;
}

void GameOptionsForm::OnControlSelected(word idc)
{
	// Remember what button was pressed - the caller will want this

	switch (idc) {
	case kidcInGameOptions:
		{
			ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms, kidfInGameOptions, new InGameOptionsForm());
			if (pfrm != NULL) {
				pfrm->DoModal();
				delete pfrm;
				return;
			}
		}
		break;

	case kidcSoundOptions:
		{
			ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms, kidfSoundOptions, new SoundOptionsForm());
			if (pfrm != NULL) {
				pfrm->DoModal();
				delete pfrm;
				return;
			}
		}
		break;

	case kidcColorOptions:
		// This'll get executed after return
		break;

	case kidcDisplayOptions:
		{
			ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms, kidfDisplayOptions, new DisplayOptionsForm());
			if (pfrm != NULL) {
				pfrm->DoModal();
				delete pfrm;
				return;
			}
		}
		break;

	case kidcPerformanceOptions:
		{
			ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms, kidfPerformanceOptions, new PerformanceOptionsForm());
			if (pfrm != NULL) {
				pfrm->DoModal();
				delete pfrm;
				return;
			}
		}
		break;
	}

	Form::OnControlSelected(idc);
}

//
// InGameOptionsForm implementation
//

bool InGameOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;

	m_fLassoSelection = gfLassoSelection;
	m_tGameSpeed = gtGameSpeed;
	m_wfHandicap = gwfHandicap;
    m_nScrollSpeed = gnScrollSpeed;

    GetControlPtr(kidcLassoSelection)->Show(false);

	InitResettableControls();
	return true;
}

// Transplate game update rate into a 'multiplier from normal' string

void GetSpeedMultiplierString(char *psz, long tGameSpeed)
{
	int n = 0;
    if (tGameSpeed != 0) {
        n = (8 * 100) / tGameSpeed;
    }
	int cWhole = n / 100;
	int cFrac = n % 100;
	sprintf(psz, "%d.%dx", cWhole, cFrac);
}

void InGameOptionsForm::InitResettableControls()
{
	// Lasso

	SetControlChecked(kidcLassoSelection, m_fLassoSelection);

	// Game Speed

	SliderControl *psldr = (SliderControl *)GetControlPtr(kidcGameSpeed);
	psldr->SetRange(0, ARRAYSIZE(gatGameSpeeds) - 1);
	psldr->SetValue(8);
	for (int i = 0; i < ARRAYSIZE(gatGameSpeeds); i++) {
		if (gatGameSpeeds[i] == m_tGameSpeed) {
			psldr->SetValue(i);
			break;
		}
	}

    // Scroll Speed 1x to 5x, in .25 increments

#define knScrollSpeedMax 5

	psldr = (SliderControl *)GetControlPtr(kidcScrollSpeed);
	psldr->SetRange(0, (knScrollSpeedMax - 1) * 4);
	psldr->SetValue((m_nScrollSpeed - 1.0) / 0.25);

	// Difficulty

	SetControlChecked(kidcEasy, m_wfHandicap == kfHcapEasy);
	SetControlChecked(kidcNormal, m_wfHandicap == kfHcapNormal);
	SetControlChecked(kidcHard, m_wfHandicap == kfHcapHard);

	UpdateLabels();
}

void InGameOptionsForm::UpdateLabels()
{
	char szT[80];
	GetSpeedMultiplierString(szT, m_tGameSpeed);
	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcGameSpeedLabel);
	plbl->SetText(szT);

	plbl = (LabelControl *)GetControlPtr(kidcScrollSpeedLabel);
    int cWhole = (int)m_nScrollSpeed;
    int cFrac = (m_nScrollSpeed - (int)m_nScrollSpeed) * 100;
    sprintf(szT, "%d.%dx", cWhole, cFrac);
    plbl->SetText(szT);
}

void InGameOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcGameSpeed:
		{
			SliderControl *psldr =
                    (SliderControl *)GetControlPtr(kidcGameSpeed);
			m_tGameSpeed = gatGameSpeeds[psldr->GetValue()];
			UpdateLabels();
		}
		break;

    case kidcScrollSpeed:
        {
			SliderControl *psldr =
                    (SliderControl *)GetControlPtr(kidcScrollSpeed);
            m_nScrollSpeed = 1.0 + psldr->GetValue() * 0.25;
			UpdateLabels();
        }
        break;

	case kidcEasy:
	case kidcNormal:
	case kidcHard:
		{
			SetControlChecked(kidcEasy, idc == kidcEasy);
			SetControlChecked(kidcNormal, idc == kidcNormal);
			SetControlChecked(kidcHard, idc == kidcHard);
		}
		break;

	case kidcOk:
		{
			// Lasso and game speed, other

			gfLassoSelection = GetControlChecked(kidcLassoSelection);
			SliderControl *psldr = (SliderControl *)GetControlPtr(kidcGameSpeed);
			ggame.SetGameSpeed(gatGameSpeeds[psldr->GetValue()]);
			psldr = (SliderControl *)GetControlPtr(kidcScrollSpeed);
            gnScrollSpeed = 1.0 + psldr->GetValue() * 0.25;

			// Difficulty

			if (GetControlChecked(kidcEasy))
				gwfHandicap = kfHcapEasy;
			else if (GetControlChecked(kidcNormal))
				gwfHandicap = kfHcapNormal;
			else if (GetControlChecked(kidcHard))
				gwfHandicap = kfHcapHard;
			if (gpplrLocal != NULL)
				gpplrLocal->SetHandicap(gwfHandicap);

			// Save prefs now in case the game crashes before exiting!

			ggame.SavePreferences();
		}
		EndForm(idc);
		break;

	case kidcCancel:
		EndForm(idc);
		break;

	case kidcDefault:
		m_fLassoSelection = false;
		m_tGameSpeed = kcmsUpdate / 20;
        m_nScrollSpeed = 1.0;
		m_wfHandicap = kfHcapDefault;
		InitResettableControls();
		break;
	}
}

//
// SoundOptionsForm
//

bool SoundOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;

	// Save away values to restore in case of cancel

	m_nVolume = gsndm.GetVolume();
	m_fEnabled = gsndm.IsEnabled();

	InitResettableControls();
	return true;
}

void SoundOptionsForm::InitResettableControls()
{
	SetControlChecked(kidcMute, !gsndm.IsEnabled());

	SliderControl *psldr = (SliderControl *)GetControlPtr(kidcVol);
	psldr->SetRange(0, 255);

	int nVolume = gsndm.GetVolume();
	if (nVolume < 0) {
		psldr->Show(false);
		GetControlPtr(kidcVolLabel)->Show(false);
		GetControlPtr(kidcVolumeString)->Show(false);
	} else {
		psldr->SetValue(nVolume);
	}

	UpdateLabels();
}

void SoundOptionsForm::UpdateLabels()
{
	char szT[80];
	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcVolLabel);
	itoa((gsndm.GetVolume() * 100) / 255, szT, 10);
	plbl->SetText(szT);
}

void SoundOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcVol:
		{
			SliderControl *psldr = (SliderControl *)GetControlPtr(kidcVol);
			gsndm.SetVolume(psldr->GetValue());
			UpdateLabels();
		}
		break;

	case kidcMute:
		{
			gsndm.Enable(!GetControlChecked(kidcMute));
			InitResettableControls();
		}
		break;

	case kidcCancel:
		gsndm.Enable(m_fEnabled);
		gsndm.SetVolume(m_nVolume);
		EndForm(idc);
		break;

	case kidcOk:
		ggame.SavePreferences();
		EndForm(idc);
		break;
	}
}

//
// Color Options Form
//

ColorOptionsForm::ColorOptionsForm(Palette *ppal)
{
	m_ppal = ppal;
}

bool ColorOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!Form::Init(pfrmm, pini, idf))
		return false;

	// Save away values to restore in case of cancel

	m_nHueOffset = gnHueOffset;
	m_nSatMultiplier = gnSatMultiplier;
	m_nLumOffset = gnLumOffset;

	InitResettableControls();

	// Position the form middle of screen, bottom

	Size sizDib;
	m_pfrmm->GetDib()->GetSize(&sizDib);
	Rect rc;
	rc.left = ((sizDib.cx - m_rc.Width()) / 2) & ~1;
	rc.top = sizDib.cy - m_rc.Height();
	rc.right = rc.left + m_rc.Width();
	rc.bottom = rc.top + m_rc.Height();
	SetRect(&rc);

	return true;
}

void ColorOptionsForm::InitResettableControls()
{
	SliderControl *psldr = (SliderControl *)GetControlPtr(kidcLum);
	psldr->SetRange(-100, 100);
	psldr->SetValue(gnLumOffset);

	psldr = (SliderControl *)GetControlPtr(kidcSat);
	psldr->SetRange(-100, 100);
	psldr->SetValue(gnSatMultiplier);

	psldr = (SliderControl *)GetControlPtr(kidcHue);
	psldr->SetRange(-100, 100);
	psldr->SetValue(gnHueOffset);

	UpdateLabels();
}

void ColorOptionsForm::UpdateLabels()
{
	char szT[80];
	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcHueLabel);
	itoa(gnHueOffset, szT, 10);
	plbl->SetText(szT);

	plbl = (LabelControl *)GetControlPtr(kidcSatLabel);
	itoa(gnSatMultiplier, szT, 10);
	plbl->SetText(szT);

	plbl = (LabelControl *)GetControlPtr(kidcLumLabel);
	itoa(gnLumOffset, szT, 10);
	plbl->SetText(szT);
}

void ColorOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcHue:
	case kidcSat:
	case kidcLum:
		{
			SliderControl *psldr = (SliderControl *)GetControlPtr(kidcHue);
			gnHueOffset = (short)psldr->GetValue();

			psldr = (SliderControl *)GetControlPtr(kidcSat);
			gnSatMultiplier = (short)psldr->GetValue();

			psldr = (SliderControl *)GetControlPtr(kidcLum);
			gnLumOffset = (short)psldr->GetValue();
			
			UpdateLabels();

			SetHslAdjustedPalette(m_ppal, gnHueOffset, gnSatMultiplier, gnLumOffset);

			// Some devices such as the PocketPC need the form redrawn because setting the palette
			// is only setting an 8->16 bit translation table, and only the controls are redrawing.

		}
		break;

	case kidcCancel:
		if (m_nHueOffset != gnHueOffset || m_nSatMultiplier != gnSatMultiplier || m_nLumOffset != gnLumOffset) {
			gnHueOffset = m_nHueOffset;
			gnSatMultiplier = m_nSatMultiplier;
			gnLumOffset = m_nLumOffset;
			SetHslAdjustedPalette(m_ppal, gnHueOffset, gnSatMultiplier, gnLumOffset);
		}

		EndForm(idc);
		break;

	case kidcOk:
		// Save prefs now in case the game crashes before exiting!

		ggame.SavePreferences();
		EndForm(idc);
		break;

	case kidcDefault:
		if (gnHueOffset != 0 || gnSatMultiplier != 0|| gnLumOffset != 0) {
			gnHueOffset = 0;
			gnSatMultiplier = 0;
			gnLumOffset = 0;
			SetHslAdjustedPalette(m_ppal, gnHueOffset, gnSatMultiplier, gnLumOffset);
		}

		InitResettableControls();

		// Some devices such as the PocketPC need the form redrawn because setting the palette
		// is only setting an 8->16 bit translation table, and only the controls are redrawing.

		break;
	}
}

void ColorOptionsForm::OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd)
{
	// Draw the fancy background bitmap

	RawBitmap *prbm = LoadRawBitmap("titlescreenbkgd.rbm");
	BltHelper(pbm, prbm, pupd, m_rc.left, m_rc.top);
	delete prbm;
}

//
// Display options form
//

bool DisplayOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;

	// Initialize display mode list

	ListControl *plstc = (ListControl *)GetControlPtr(kidcModesList);
	plstc->SetFlags(plstc->GetFlags() | kfLstcBorder);

	int immCur = ggame.GetModeMatchCurrent();
	int immBest = ggame.GetModeMatchBest();
	int cmm = ggame.GetModeMatchCount();

	for (int imm = 0; imm < cmm; imm++) {
		ModeMatch mm;
		ggame.GetModeMatch(imm, &mm);
		ModeInfo mode;
		gpdisp->GetModeInfo(mm.imode, &mode);
		char szT[64];
		sprintf(szT, "%d x %d%s", mode.cx, mode.cy,
				imm == immBest ? "  (recommended)" : "");
		plstc->Add(szT, (void *)(pword)imm);
	}
	plstc->Select(immCur);

	InitResettableControls();
	return true;
}

void DisplayOptionsForm::InitResettableControls()
{
	SetControlChecked(kidcFullscreen, gpdisp->IsFullscreen());
	SetControlChecked(kidcSmoothMotion, gfSmoothMotion);
}

void DisplayOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcOk:
		{
			gpdisp->SetFullscreen(GetControlChecked(kidcFullscreen));
			gfSmoothMotion = GetControlChecked(kidcSmoothMotion);

			ListControl *plstc = (ListControl *)GetControlPtr(kidcModesList);
			if (plstc->GetFlags() & kfCtlVisible) {
				int immNew = (int)(pword)plstc->GetSelectedItemData();
				ggame.RequestModeChange(immNew);
			}

			// Save prefs now in case the game crashes before exiting!

			ggame.SavePreferences();
		}
		EndForm(idc);
		break;

	case kidcCancel:
		EndForm(idc);
		break;

	case kidcDefault:
		{
			ListControl *plstc = (ListControl *)GetControlPtr(kidcModesList);
			plstc->Select(ggame.GetModeMatchBest(), true);
			SetControlChecked(kidcFullscreen, true);
			SetControlChecked(kidcSmoothMotion, true);
		}
		break;
	}
}

//
// PerformanceOptionsForm implementation
//

bool PerformanceOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;
	m_wfPerfOptions = gwfPerfOptions;
	InitResettableControls();
	return true;
}

void PerformanceOptionsForm::InitResettableControls()
{
	SetControlChecked(kidcRocketShots, (m_wfPerfOptions & kfPerfRocketShots) != 0);
	SetControlChecked(kidcRocketTrails, (m_wfPerfOptions & kfPerfRocketTrails) != 0);
	SetControlChecked(kidcRocketImpacts, (m_wfPerfOptions & kfPerfRocketImpacts) != 0);
	SetControlChecked(kidcShots, (m_wfPerfOptions & kfPerfShots) != 0);
	SetControlChecked(kidcShotImpacts, (m_wfPerfOptions & kfPerfShotImpacts) != 0);
	SetControlChecked(kidcSelectionBrackets, (m_wfPerfOptions & kfPerfSelectionBrackets) != 0);
	SetControlChecked(kidcSmoke, (m_wfPerfOptions & kfPerfSmoke) != 0);
	SetControlChecked(kidcEnemyDamageIndicator, (m_wfPerfOptions & kfPerfEnemyDamageIndicator) != 0);
	SetControlChecked(kidcScorchMarks, (m_wfPerfOptions & kfPerfScorchMarks) != 0);
	SetControlChecked(kidcSymbolFlashing, (m_wfPerfOptions & kfPerfSymbolFlashing) != 0);
}

void PerformanceOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcCancel:
		EndForm(idc);
		break;

	case kidcOk:
		{
			word wfT = 0;
			if (GetControlChecked(kidcRocketShots))
				wfT |= kfPerfRocketShots;
			if (GetControlChecked(kidcRocketTrails))
				wfT |= kfPerfRocketTrails;
			if (GetControlChecked(kidcRocketImpacts))
				wfT |= kfPerfRocketImpacts;
			if (GetControlChecked(kidcShots))
				wfT |= kfPerfShots;
			if (GetControlChecked(kidcShotImpacts))
				wfT |= kfPerfShotImpacts;
			if (GetControlChecked(kidcSelectionBrackets))
				wfT |= kfPerfSelectionBrackets;
			if (GetControlChecked(kidcSmoke))
				wfT |= kfPerfSmoke;
			if (GetControlChecked(kidcEnemyDamageIndicator))
				wfT |= kfPerfEnemyDamageIndicator;
			if (GetControlChecked(kidcScorchMarks))
				wfT |= kfPerfScorchMarks;
			if (GetControlChecked(kidcSymbolFlashing))
				wfT |= kfPerfSymbolFlashing;
			gwfPerfOptions = wfT;
			ggame.SavePreferences();
			EndForm(idc);
		}
		break;

	case kidcDefault:
		m_wfPerfOptions = kfPerfAll;
		InitResettableControls();
		break;
	}
}

//
// TestOptionsForm implementation
//

bool TestOptionsForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!Form::Init(pfrmm, pini, idf))
		return false;

	CheckBoxControl *pcbox = (CheckBoxControl *)GetControlPtr(kidcDrawLines);
#ifdef DRAW_LINES
	pcbox->SetChecked(gfDrawLines);
#else
	pcbox->Show(false);
#endif

	pcbox = (CheckBoxControl *)GetControlPtr(kidcDrawPaths);
#ifdef DRAW_PATHS
	pcbox->SetChecked(gfDrawPaths);
#else
	pcbox->Show(false);
#endif

	pcbox = (CheckBoxControl *)GetControlPtr(kidcShowStats);
#ifdef STATS_DISPLAY
	pcbox->SetChecked(gfShowStats);
#else
	pcbox->Show(false);
#endif

	SetControlChecked(kidcOvermind, gfOvermindEnabled);
	SetControlChecked(kidcShowFPS, gfShowFPS);
	SetControlChecked(kidcSuspendUpdates, gfSuspendUpdates);
	SetControlChecked(kidcMaxRepaint, (gevm.GetRedrawFlags() & kfRedrawMax) != 0);
#ifdef DRAW_UPDATERECTS
	SetControlChecked(kidcDrawUpdateRects, gfDrawUpdateRects);
#endif
	SetControlChecked(kidcGodMode, gfGodMode);
	SetControlChecked(kidcAutosave, gfAutosave);
    SetControlChecked(kidcStylusUI, gfStylusUI);

    dword ff = ggame.GetSimUIForm()->GetPenHandler()->GetFlags();

	return true;
}

void TestOptionsForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcBreak:
		break;

	case kidcGobCount:
		{
			Form *pfrm = gpmfrmm->LoadForm(gpiniForms, kidfGobCount, new GobCountForm());
			pfrm->DoModal();
			delete pfrm;
		}
		break;

	case kidcMemoryUse:
		{
			Form *pfrm = gpmfrmm->LoadForm(gpiniForms, kidfMemoryUse, new MemoryUseForm());
			pfrm->DoModal();
			delete pfrm;
		}
		break;

#ifdef DRAW_PATHS
	case kidcDrawPaths:
		gfDrawPaths = !gfDrawPaths;
		break;
#endif

#ifdef DRAW_LINES
	case kidcDrawLines:
		gfDrawLines = !gfDrawLines;
		break;
#endif

    case kidcStylusUI:
        gfStylusUI = !gfStylusUI;
		ggame.GetSimUIForm()->SetUIType(gfStylusUI == true ?
                kuitStylus : kuitFinger);
        break;

	case kidcOvermind:
		{
			for (Gob *pgob = ggobm.GetFirstGob(); pgob != NULL; pgob = ggobm.GetNextGob(pgob)) {
				if (pgob->GetType() == kgtOvermind)
					((OvermindGob *)pgob)->Toggle();
			}
	 		gfOvermindEnabled = !gfOvermindEnabled;
		}
		break;

	case kidcClearFog:
		gsim.GetLevel()->GetFogMap()->RevealAll(gpupdSim);
		break;

#ifdef STATS_DISPLAY
	case kidcShowStats:
		gfShowStats = !gfShowStats;
		break;
#endif

	case kidcShowFPS:
		gfShowFPS = !gfShowFPS;
		ggame.GetSimUIForm()->GetControlPtr(kidcFps)->Show(gfShowFPS);
		break;

	case kidcMaxRepaint:
		if (gevm.GetRedrawFlags() & kfRedrawMax) {
			gevm.ClearRedrawFlags(kfRedrawMax);
		} else {
			gevm.SetRedrawFlags(kfRedrawMax);
		}
		break;

	case kidcSuspendUpdates:
		gfSuspendUpdates = !gfSuspendUpdates;
		break;

	case kidcDrawUpdateRects:
#ifdef DRAW_UPDATERECTS
		gfDrawUpdateRects = !gfDrawUpdateRects;
#endif
		break;

	case kidcGodMode:
		gfGodMode = !gfGodMode;
		break;

	case kidcAutosave:
		gfAutosave = !gfAutosave;
		break;

	case kidcHelp:
		Help();
		break;

	default:
		EndForm(idc);
	}
}

//
// MemoryUseForm implementation
//

bool MemoryUseForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	if (!Form::Init(pfrmm, pini, idf))
		return false;

	SetControlChecked(kidcLimitCache, gcam.GetLimit() != 0);
	UpdateLabels();

	SetBackgroundColorIndex(kiclrBlack);

	return true;
}

void MemoryUseForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcClearCache:
		gcam.MakeSpace((dword)-1);
		UpdateLabels();
		break;

	case kidcLimitCache:
		if (gcam.GetLimit() == 0) {
			gcam.SetLimit(10240);
		} else {
			gcam.SetLimit(0);
		}
		UpdateLabels();
		break;

	case kidcAdd10KCache:
		gcam.SetLimit(gcam.GetLimit() + 10240);
		UpdateLabels();
		break;

	case kidcSub10KCache:
		gcam.SetLimit(gcam.GetLimit() - 10240);
		UpdateLabels();
		break;

	default:
		EndForm(idc);
	}
}

extern dword gcbDynMemAtStart;
extern dword gcbDbMemAtStart;

void MemoryUseForm::UpdateLabels()
{
	dword cbDyn, cbDb;
	dword cbTotal = gmmgr.GetTotalSize(&cbDyn, &cbDb);

	dword cbFree = 0;

	char szT[128];

	sprintf(szT, "start dyn, db free: %u, %u", gcbDynMemAtStart, gcbDbMemAtStart);
	((LabelControl *)GetControlPtr(kidcDynDbInitial))->SetText(szT);
	sprintf(szT, "mmgr dyn, db reserve: %u, %u", cbDyn, cbDb);
	((LabelControl *)GetControlPtr(kidcMmgrDynDbReserve))->SetText(szT);
	sprintf(szT, "dyn use: %u / %u", gcbDynMemAtStart - cbFree, gcbDynMemAtStart);
	((LabelControl *)GetControlPtr(kidcDynUse))->SetText(szT);
	sprintf(szT, "mmgr use: %u / %u", cbTotal - gmmgr.GetFreeSize(), cbTotal);
	((LabelControl *)GetControlPtr(kidcMmgrUse))->SetText(szT);
	sprintf(szT, "cache use: %u", gcam.GetTotalSize());
	((LabelControl *)GetControlPtr(kidcCacheUse))->SetText(szT);

	bool fCacheLimit = (gcam.GetLimit() != 0);
	GetControlPtr(kidcAdd10KCache)->Show(fCacheLimit);
	GetControlPtr(kidcSub10KCache)->Show(fCacheLimit);
	GetControlPtr(kidcCacheLimit)->Show(fCacheLimit);

	sprintf(szT, "Cache Limit: %u", gcam.GetLimit());
	((LabelControl *)GetControlPtr(kidcCacheLimit))->SetText(szT);
}

//
// GobCountForm
//

// Note: matches gob types (kgt*)

static char *s_aszGobs[] = {
	"none", "guard", "trooper", "hrc", "decal", "scenery", "anim", "pwr gen", 
	"proc", "struc", "unit", "bullpup", "hq", "randc", "vts", "srv ctr", 
	"broadsw", "libertr", "eagle", "hydra",
	"raider", "wareh", "domin", "overm", "tshot", "rockt", "gtower", "rtower", "scorch",
	"smoke", "puff", "bullet", "cyclops", "cy shot", "andy", "repl", "actvtr", "fox"
};

void GobCountForm::OnPaint(DibBitmap *pbm)
{
	int acbGob[ARRAYSIZE(s_aszGobs)];
	memset(acbGob, 0, sizeof(acbGob));
	int acGobs[ARRAYSIZE(s_aszGobs)];
	memset(acGobs, 0, sizeof(acGobs));
	int acGobsSide[ARRAYSIZE(s_aszGobs)][kcSides];
	memset(acGobsSide, 0, sizeof(acGobsSide));
	bool afUnit[ARRAYSIZE(s_aszGobs)];
	memset(afUnit, 0, sizeof(afUnit));
	bool afStructure[ARRAYSIZE(s_aszGobs)];
	memset(afStructure, 0, sizeof(afStructure));
	bool afMobileUnit[ARRAYSIZE(s_aszGobs)];
	memset(afMobileUnit, 0, sizeof(afMobileUnit));
	for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
		GobType gt = pgobT->GetType();
		acGobs[gt]++;
		acbGob[gt] = 0;
		if (pgobT->GetFlags() & kfGobUnit) {
			afUnit[gt] = true;
			UnitGob *punt = (UnitGob *)pgobT;
			acGobsSide[gt][punt->GetSide()]++;
		}
		if (pgobT->GetFlags() & kfGobStructure)
			afStructure[gt] = true;
		if (pgobT->GetFlags() & kfGobMobileUnit)
			afMobileUnit[gt] = true;
	}

	int y = m_rc.top;
	int x = m_rc.left;
	char szT[64];

	int gt;
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++) {
		if (acGobs[gt] == 0)
			continue;

		// Unit?

		if (afUnit[gt]) {
			char szT2[64];
			sprintf(szT2, "%s=%d", s_aszGobs[gt], acbGob[gt]);
			FormatSideString(szT2, acGobsSide[gt], szT);
		} else {
			sprintf(szT, "%s=%d:%d", s_aszGobs[gt], acbGob[gt], acGobs[gt]);
		}

		OutputString(pbm, szT, &x, &y);
	}

	// Units per side

	int acUnitsPerSide[kcSides];
	memset(acUnitsPerSide, 0, sizeof(acUnitsPerSide));
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++) {
		if (!afUnit[gt])
			continue;
		for (Side side = ksideNeutral; side < kcSides; side++)
			acUnitsPerSide[side] += acGobsSide[gt][side];
	}
	FormatSideString("Units", acUnitsPerSide, szT);
	OutputString(pbm, szT, &x, &y);

	// Structures per side

	int acStructuresPerSide[kcSides];
	memset(acStructuresPerSide, 0, sizeof(acStructuresPerSide));
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++) {
		if (!afStructure[gt])
			continue;
		for (Side side = ksideNeutral; side < kcSides; side++)
			acStructuresPerSide[side] += acGobsSide[gt][side];
	}
	FormatSideString("Structs", acStructuresPerSide, szT);
	OutputString(pbm, szT, &x, &y);

	// Mobile units per side

	int acMobileUnitsPerSide[kcSides];
	memset(acMobileUnitsPerSide, 0, sizeof(acMobileUnitsPerSide));
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++) {
		if (!afMobileUnit[gt])
			continue;
		for (Side side = ksideNeutral; side < kcSides; side++)
			acMobileUnitsPerSide[side] += acGobsSide[gt][side];
	}
	FormatSideString("Munts", acMobileUnitsPerSide, szT);
	OutputString(pbm, szT, &x, &y);

	// Total gobs

	int cGobs = 0;
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++)
		cGobs += acGobs[gt];
	sprintf(szT, "total:%d", cGobs);
	OutputString(pbm, szT, &x, &y);

	// Total memory consumed

	long cbGobs = 0;
	for (gt = 0; gt < ARRAYSIZE(s_aszGobs); gt++)
		cbGobs += (long)(acbGob[gt] + 8) * acGobs[gt];
	sprintf(szT, "gob mem:%ld", cbGobs);
	OutputString(pbm, szT, &x, &y);

}

void GobCountForm::OutputString(DibBitmap *pbm, char *psz, int *px, int *py)
{
	Font *pfnt = gapfnt[kifntDefault];

	char *pszNext = ",";
	int cxNext = pfnt->GetTextExtent(pszNext);

	int x = *px;
	int y = *py;

	int cx = pfnt->GetTextExtent(psz);
	if (x + cxNext + cx > m_rc.right) {
		y += pfnt->GetHeight();
		x = m_rc.left;
	} else {
		if (x != m_rc.left) {
			pfnt->DrawText(pbm, pszNext, x, y);
			x += cxNext;
		}
	}
	pfnt->DrawText(pbm, psz, x, y);
	x += cx;

	*px = x;
	*py = y;
}

void GobCountForm::FormatSideString(char *pszIn, int *acPerSide, char *pszOut)
{
	int cTotal = 0;
	Side sideLast = (Side)-1;
	for (Side side = ksideNeutral; side < kcSides; side++) {
		cTotal += acPerSide[side];
		if (acPerSide[side] != 0)
			sideLast = side;
	}

	sprintf(pszOut, "%s:", pszIn);
	char szT[32];
	for (Side sideT = ksideNeutral; sideLast != (Side)-1 && sideT <= sideLast; sideT++) {
		sprintf(szT, sideT != sideLast ? "%d/" : "%d", acPerSide[sideT]);
		strcat(pszOut, szT);
	}
	sprintf(szT, "(%d)", cTotal);
	strcat(pszOut, szT);
}

void GobCountForm::OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque)
{
	pupd->InvalidateRect();
	Form::OnUpdateMapInvalidate(pupd, prcOpaque);
}

bool GobCountForm::OnPenEvent(Event *pevt)
{
	if (pevt->eType == penDownEvent) {
		EndForm(kidcOk);
		return true;
	}
	return false;
}

//
// InputPanel form
//

class InputPanelForm : public ShellForm
{
public:
	virtual void OnControlSelected(word idc);
	virtual void OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque);
	virtual void OnPaint(DibBitmap *pbm);
	virtual bool OnPenEvent(Event *pevt);
	virtual bool EventProc(Event *pevt);
	virtual bool Init(FormMgr *pfrmm, IniReader *pini, word idf);

	void SetLabel(const char *pszLabel);
	void SetEdit(const char *pszEdit);
	void GetEdit(char *psz, int cb);
	void SetChars(char **ppsz, int cRows);
	void SetValidateProc(bool (*pfnValidateInput)(const char *psz)) {
		m_pfnValidateInput = pfnValidateInput;
	}

private:
	char TrackPen(int x, int y, bool fDown);
	void OnChar(char ch);
	void OnBackspace();
	int GetCharRect(int iCol, int iRow, char *pch, Rect *prcChar);

	char **m_ppszChars;
	int m_cRows;
	int m_iColLast;
	int m_iRowLast;
	bool m_fDown;
	int m_xForm;
	int m_yForm;
	Font *m_pfnt;
	bool (*m_pfnValidateInput)(const char *psz);
};

bool DoInputPanelForm(char **ppszChars, int cRows, const char *pszLabel,
        const char *pszEdit, char *pszOut, int cbOut,
        bool (*pfnValidateInput)(const char *psz))
{
	InputPanelForm *pfrm = (InputPanelForm *)gpmfrmm->LoadForm(gpiniForms, kidfInputPanel, new InputPanelForm());
	if (pfrm != NULL) {
		pfrm->SetValidateProc(pfnValidateInput);
		pfrm->SetLabel(pszLabel);
		pfrm->SetEdit(pszEdit);
		if (ppszChars != NULL)
			pfrm->SetChars(ppszChars, cRows);
		if (pfrm->DoModal()) {
			pfrm->GetEdit(pszOut, cbOut);
			delete pfrm;
			return true;
		} else {
			strncpyz(pszOut, pszEdit, cbOut);
			delete pfrm;
			return false;
		}
	}

	return false;
}

void DoInputPanelForm(Form *pfrm, word idcLabel, word idcEdit)
{
	LabelControl *plbl = (LabelControl *)pfrm->GetControlPtr(idcLabel);
	const char *pszLabel = plbl->GetText();
	char szEdit[128];
	EditControl *pedt = (EditControl *)pfrm->GetControlPtr(idcEdit);
	pedt->GetText(szEdit, sizeof(szEdit));
	DoInputPanelForm(NULL, 0, pszLabel, szEdit, szEdit, sizeof(szEdit));
	pedt->SetText(szEdit);
}

bool InputPanelForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	// Get initial form dimensions before passing to ShellForm since it reformats

	char szForm[32];
	itoa(idf, szForm, 10);
	int x, y, cx, cy;
	pini->GetPropertyValue(szForm, "FORM", "(%d %d %d %d)", &x, &y, &cx, &cy);
	Size siz;
	pfrmm->GetDib()->GetSize(&siz);
	m_xForm = (siz.cx - PcFromFc(cx)) / 2;
	m_yForm = (siz.cy - PcFromFc(cy)) / 2;
	m_pfnt = gapfnt[kifntShadow];
	m_iColLast = -1;
	m_iRowLast = -1;
	m_fDown = false;

	static char *s_apszChars[] = {
		"ABCDEFGHIJKLM",
		"NOPQRSTUVWXYZ",
		"abcdefghijklm",
		"nopqrstuvwxyz",
		"1234567890?!*"
	};

	m_ppszChars = s_apszChars;
	m_cRows = ARRAYSIZE(s_apszChars);
	m_pfnValidateInput = NULL;

	return ShellForm::Init(pfrmm, pini, idf);
}

const int kxBox = 5;
const int kyBox = 35;
const int kcxBox = (160 - (5 + 5));
const int kcyBox = (118 - kyBox - 5);

int InputPanelForm::GetCharRect(int iCol, int iRow, char *pch, Rect *prcChar)
{
	int cRows = m_cRows;
	int cCols = (int)strlen(m_ppszChars[0]);
	prcChar->SetEmpty();
	if (iRow < 0 || iRow >= cRows)
		return 0;
	if (iCol < 0 || iCol >= (int)strlen(m_ppszChars[iRow]))
		return 0;

	prcChar->left = PcFromFc(kxBox) + PcFromFc(kcxBox) * iCol / cCols;
	prcChar->right = PcFromFc(kxBox) + PcFromFc(kcxBox) * (iCol + 1) / cCols;
	prcChar->top = PcFromFc(kyBox) + PcFromFc(kcyBox) * iRow / cRows;
	prcChar->bottom = PcFromFc(kyBox) + PcFromFc(kcyBox) * (iRow + 1) / cRows;
	prcChar->Offset(m_xForm, m_yForm);
	*pch = m_ppszChars[iRow][iCol];
	return m_pfnt->GetTextExtent(pch, 1);
}

char InputPanelForm::TrackPen(int x, int y, bool fDown)
{
	// Hittest

	char ch = (char)-1;
	Rect rcChar;
	int iRow, iCol;
	bool fFound = false;
	for (iRow = 0; iRow < m_cRows; iRow++) {
		int cCols = (int)strlen(m_ppszChars[iRow]);
		for (iCol = 0; iCol < cCols; iCol++) {
			GetCharRect(iCol, iRow, &ch, &rcChar);
			if (rcChar.PtIn(x, y)) {
				fFound = true;
				break;
			}
		}
		if (fFound)
			break;
	}

	// Track

	if (iRow != m_iRowLast || iCol != m_iColLast || fDown != m_fDown) {
		char chT;
		Rect rcT;
		GetCharRect(iCol, iRow, &chT, &rcT);
		InvalidateRect(&rcT);
		GetCharRect(m_iColLast, m_iRowLast, &chT, &rcT);
		InvalidateRect(&rcT);
		m_iRowLast = iRow;
		m_iColLast = iCol;
	}
	m_fDown = fDown;
	return fFound ? ch : (char)-1;
}

bool InputPanelForm::OnPenEvent(Event *pevt)
{
	switch (pevt->eType) {
	case penDownEvent:
		TrackPen(pevt->x, pevt->y, true);
		break;

	case penUpEvent:
		{
			char ch = TrackPen(pevt->x, pevt->y, false);
			if (ch != (char)-1) {
				gsndm.PlaySfx(ksfxGuiButtonTap);
				OnChar(ch);
			}
		}
		break;

	case penMoveEvent:
		TrackPen(pevt->x, pevt->y, m_fDown);
		break;
	}

	return ShellForm::OnPenEvent(pevt);
}

bool InputPanelForm::EventProc(Event *pevt)
{
	if (pevt->eType != keyDownEvent)
		return ShellForm::EventProc(pevt);

	// Keyboard interface

	if (pevt->chr == chrBackspace || pevt->chr == chrDelete) {
		OnBackspace();
	} else {
		for (int iRow = 0; iRow < m_cRows; iRow++) {
			int cCols = (int)strlen(m_ppszChars[iRow]);
			for (int iCol = 0; iCol < cCols; iCol++) {
				if (pevt->chr == m_ppszChars[iRow][iCol])
					OnChar(pevt->chr);
			}
		}
	}

	return false;
}

void InputPanelForm::OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque)
{
	ShellForm::OnUpdateMapInvalidate(pupd, prcOpaque);

	Rect rc;
	rc.SetEmpty();
	for (int iRow = 0; iRow < m_cRows; iRow++) {
		int cCols = (int)strlen(m_ppszChars[iRow]);
		for (int iCol = 0; iCol < cCols; iCol++) {
			Rect rcChar;
			char ch;
			GetCharRect(iCol, iRow, &ch, &rcChar);
			rc.Union(&rcChar);
		}
	}
	pupd->InvalidateRect(&rc);
}

void InputPanelForm::OnPaint(DibBitmap *pbm)
{
	ShellForm::OnPaint(pbm);

	int cyChar = m_pfnt->GetHeight();
	for (int iRow = 0; iRow < m_cRows; iRow++) {
		int cCols = (int)strlen(m_ppszChars[iRow]);
		for (int iCol = 0; iCol < cCols; iCol++) {
			Rect rcChar;
			char ch;
			int cxChar = GetCharRect(iCol, iRow, &ch, &rcChar);
			int xChar = rcChar.left + (rcChar.Width() - cxChar) / 2;
			int yChar = rcChar.top + (rcChar.Height() - cyChar) / 2;
			m_pfnt->DrawText(pbm, &ch, xChar, yChar, 1);
			if (iCol == m_iColLast && iRow == m_iRowLast && m_fDown)
				DrawBorder(pbm, &rcChar, 1, GetColor(kiclrWhite), NULL);
		}
	}
}

void InputPanelForm::OnChar(char ch)
{
	char szT[128];
	GetEdit(szT, sizeof(szT));
	int cch = (int)strlen(szT);
	if (cch < sizeof(szT) - 1) {
		szT[cch] = ch;
		szT[cch + 1] = 0;
		SetEdit(szT);
	}
}

void InputPanelForm::OnBackspace()
{
	char szT[128];
	GetEdit(szT, sizeof(szT));
	int cch = (int)strlen(szT);
	if (cch > 0) {
		szT[cch - 1] = 0;
		SetEdit(szT);
	}
}

void InputPanelForm::OnControlSelected(word idc)
{
	if (idc == kidcInputEdit)
		return;

	if (idc == kidcBackspace) {
		OnBackspace();
		return;
	}

	if (idc == kidcOk) {
		if (m_pfnValidateInput != NULL) {
			char szT[255];
			GetEdit(szT, sizeof(szT));
			if (!m_pfnValidateInput(szT))
				return;
		}
	}

	ShellForm::OnControlSelected(idc);
}

void InputPanelForm::SetLabel(const char *pszLabel)
{
	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcInputLabel);
	plbl->SetText(pszLabel);
	Rect rcLabel;
	plbl->GetRect(&rcLabel);
	EditControl *pedt = (EditControl *)GetControlPtr(kidcInputEdit);
	Rect rcEdit;
	pedt->GetRect(&rcEdit);
	rcEdit.left = rcLabel.right + 3;
	pedt->SetRect(&rcEdit);
}

void InputPanelForm::SetChars(char **ppsz, int cRows)
{
	m_ppszChars = ppsz;
	m_cRows = cRows;
}

void InputPanelForm::SetEdit(const char *pszEdit)
{
	EditControl *pedt = (EditControl *)GetControlPtr(kidcInputEdit);
	pedt->SetText(pszEdit);
}

void InputPanelForm::GetEdit(char *psz, int cb)
{
	EditControl *pedt = (EditControl *)GetControlPtr(kidcInputEdit);
	pedt->GetText(psz, cb);
}

} // namespace wi
