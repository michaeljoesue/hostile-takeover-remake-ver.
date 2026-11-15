#include "game/ht.h"
#include "game/wistrings.h"

namespace wi {

// Global representing the contiguous rect opaquing the map, in map relative tile coords

TRect *gptrcMapOpaque;
TRect gtrcMapOpaque;

// Handy dandy global flags used for enabling various test features

#ifdef STATS_DISPLAY
int gcBitmapsDrawn;
extern bool gfShowStats;
int gcbCommandsQueued;
#endif
int gcMessagesPerUpdate;

extern bool gfShowFPS;
extern bool gfSuspendUpdates;
extern bool gfSingleStep;

short gcPaint = 0;
#ifdef STATS_DISPLAY
static short s_cUpdateRects = 0;
#endif
static fix s_cfxFPS = 0;
static long s_tLastFPSUpdate = 0;

// Misc SimUI management variables

bool gfDragSelecting;
WPoint s_wptSelect1, s_wptSelect2;

// Double-click and double key press interval
const long kctDoubleClick = 50; // TUNE: half a second

// Where the last "under attack" alert happened (Space centres on it)

WPoint gwptLastAlert;
bool gfLastAlertValid = false;

void NoteAlertLocation(Gob *pgob)
{
    pgob->GetCenter(&gwptLastAlert);
    gfLastAlertValid = true;
}
WRect gwrcSelection;
#ifdef STATS_DISPLAY
long gcPathScoresCalced = 0;
#endif

//
// SimUIForm implementation
//

bool SimUIForm::s_fReadyForPaint;
    
SimUIForm::SimUIForm()
{
	s_fReadyForPaint = false;
	gfDragSelecting = false;
    
	m_fTimerAdded = false;

	m_nStateMoveTarget = 0;

    m_ppenh = new StylusHandler(this);
    m_fPanning = false;
    m_xPanLast = 0;
    m_yPanLast = 0;
    m_fAttackMoveArmed = false;
    memset(m_acgidGroup, 0, sizeof(m_acgidGroup));
    m_nGroupLast = -1;
    m_tGroupLast = 0;
}

SimUIForm::~SimUIForm()
{
	if (m_fTimerAdded)
		gtimm.RemoveTimer(this);
    delete m_ppenh;
}

bool SimUIForm::Init(FormMgr *pfrmm, IniReader *pini, word idf)
{
	// Init move target

	m_nStateMoveTarget = 0;
	m_aniMoveTarget.Init(g_panidMoveTarget);

    gtimm.AddTimer(this, gtGameSpeed);
    m_fTimerAdded = true;

	// Let the base Form initialize the controls

	bool fSuccess = Form::Init(pfrmm, pini, idf);

	// Size the rect to the dib

	DibBitmap *pbmSim = m_pfrmm->GetDib();
	Size sizSim;
	pbmSim->GetSize(&sizSim);
	Rect rcSim;
	rcSim.Set(0, 0, sizSim.cx, sizSim.cy);
	SetRect(&rcSim);

	// Set control positions
	
	Control *pctlT;
	Rect rcT;

	// Objective is at upper-left

	pctlT = GetControlPtr(kidcObjective);
	Assert(pctlT != NULL);
	pctlT->SetPosition(0, 0);

	// Countdown timer is at upper-right

	pctlT = GetControlPtr(kidcCountdown);
	Assert(pctlT != NULL);
	pctlT->GetRect(&rcT);
	pctlT->SetPosition(sizSim.cx - rcT.Width(), 0);

	// FPS is at upper right, down 1 line to leave room for countdown timer

	pctlT = GetControlPtr(kidcFps);
	Assert(pctlT != NULL);
	pctlT->GetRect(&rcT);
	pctlT->SetPosition(sizSim.cx - rcT.Width(), rcT.Height());

	// Alert is at bottom left

	pctlT = GetControlPtr(kidcAlert);
	Assert(pctlT != NULL);
	pctlT->GetRect(&rcT);
	pctlT->SetPosition(0, sizSim.cy - rcT.Height());

	// Show / hide these controls

	GetControlPtr(kidcFps)->Show(gfShowFPS);
	GetControlPtr(kidcAlert)->Show(false);
	Assert(gsim.GetLevel() != NULL);
	Assert(gsim.GetLevel()->GetTriggerMgr() != NULL);
	GetControlPtr(kidcCountdown)->Show(gsim.GetLevel()->GetTriggerMgr()->GetCountdownTimer()->GetFlags() & kfCtVisibleAtStart);

    // This form wants pen2 events
    m_wf |= kfFrmDemandPen2;

	return fSuccess;
}

void SimUIForm::OnTimer(long tCurrent) {

	if (gfSuspendUpdates && !gfSingleStep) {
		gevm.SetRedrawFlags(kfRedrawDirty);
		return;
	}
	gfSingleStep = false;
	if (gsim.IsPaused()) {
		return;
    }

    // Update (i.e., 'step') the Simulation

    gsim.Update(&gcmdq);

    // Update UI

	Update();
}

void SimUIForm::Update()
{
#ifdef STRESS
	if (gfStress) {
		if (gsim.GetTickCount() > gtStressTimeout) {
            Event evt;
			memset(&evt, 0, sizeof(evt));
			evt.eType = gameOverEvent;
			evt.dw = knGoAbortLevel;
			gevm.PostEvent(&evt);
		}
	}
#endif

	// Advance the move target while it's visible

	if (m_nStateMoveTarget == 1) {
		m_aniMoveTarget.Advance(1);
		if (m_aniMoveTarget.GetFlags() & kfAniDone)
			m_nStateMoveTarget = 2;
	}

	// Have the InputUI update its displays (e.g., credits, power)

	InputUIForm *pfrmInputUI = ggame.GetInputUIForm();
	pfrmInputUI->Update();

	// Update the objective display

	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcObjective);
	plbl->SetText(gpplrLocal->GetObjective());

	// Calc stats

	long tCurrent = HostGetTickCount();
	if (s_tLastFPSUpdate == 0)
		s_tLastFPSUpdate = tCurrent;
	if (tCurrent - s_tLastFPSUpdate >= 100) {
		fix32 fx1 = itofx32(gcPaint * 100);
		fix32 fx2 = itofx32(tCurrent - s_tLastFPSUpdate);
		if (fx2 == 0)
			s_cfxFPS = 0;
		else
			s_cfxFPS = (fix)divfx32(fx1, fx2);
		int cFpsWhole = fx32toi(s_cfxFPS);
		int cFpsFraction = (int)fxtoi(mulfx(fracfx(s_cfxFPS), itofx(10)));

		if (gfShowFPS) {
			char szFPS[80];
			sprintf(szFPS, "%d.%d", cFpsWhole, cFpsFraction);
			((LabelControl *)GetControlPtr(kidcFps))->SetText(szFPS);
		}

		s_tLastFPSUpdate = tCurrent;
		gcPaint = 0;

		// Set the # of pen events to accept per second. We don't need anything faster
		// than the frame rate.

		static long s_tLastPaint;
		if (s_tLastPaint == 0)
			s_tLastPaint = tCurrent;
		gevm.SetPenEventInterval((word)(tCurrent - s_tLastPaint));
		s_tLastPaint = tCurrent;
	}

	// Let the minimap decide if it should invalidate.

	gpmm->Update();

	// Everything has been updated for (at least) the first time
	// so we're ready to display it.

	if (!s_fReadyForPaint) {
		s_fReadyForPaint = true;
		InvalidateRect(NULL);
	}

	// Set redraw

	gevm.SetRedrawFlags(kfRedrawDirty);
}

void SimUIForm::OnUpdateMapInvalidate(UpdateMap *pupd, Rect *prcOpaque)
{
#ifdef DRAW_LINES
	if (gfDrawLines)
		pupd->InvalidateRect();
#endif

#ifdef DRAW_PATHS
	if (gfDrawPaths)
		pupd->InvalidateRect();
#endif

#if defined(DRAW_OCCUPIED_TILE_INDICATOR) || defined(MARK_TILE_BOUNDARIES) || defined(MARK_OCCUPIED_TILES)
	pupd->InvalidateRect();
#endif

	// Invalidate move target

	if (m_nStateMoveTarget != 0) {
		pupd->InvalidateTile(TcFromWc(m_wxMoveTarget), TcFromWc(m_wyMoveTarget));
		if (m_nStateMoveTarget == 2)
			m_nStateMoveTarget = 0;
	}

	// Handle selection invalidation

	InvalidateDragSelection();

	// Calculate the current opaquing rect for the map.
	// HACK: Store in a global since various places need it, such as when a gob gets removed or when
	// galaxite gets invalidated.

	Rect rcT;
	bool fEmpty = prcOpaque->IsEmpty();
	if (fEmpty) {
// Probably not worth it
	} else {
		// Use the opaque rect passed

		rcT = *prcOpaque;
	}

	// Map it map relative in tile coords

	if (!fEmpty) {
		WCoord wxView, wyView;
		gsim.GetViewPos(&wxView, &wyView);
		Rect rcMapRelative;
		rcMapRelative = rcT;
		rcMapRelative.Offset(PcFromWc(wxView), PcFromWc(wyView));
		gtrcMapOpaque.left = TcFromPc(rcMapRelative.left + gcxTile - 1);
		gtrcMapOpaque.top = TcFromPc(rcMapRelative.top + gcyTile - 1);
		gtrcMapOpaque.right = TcFromPc(rcMapRelative.right);
		gtrcMapOpaque.bottom = TcFromPc(rcMapRelative.bottom);

		// Expand the tile rect if the opaque rect is on the screen
		// edge. This way gobs will get opaqued when on screen edge even

		Size sizDib;
		m_pfrmm->GetDib()->GetSize(&sizDib);
		if (rcT.left <= 0)
			gtrcMapOpaque.left = 0;
		if (rcT.top <= 0)
			gtrcMapOpaque.top = 0;
		if (rcT.right >= sizDib.cx)
			gtrcMapOpaque.right = ktcMax;
		if (rcT.bottom >= sizDib.cy)
			gtrcMapOpaque.bottom = ktcMax;

		// Turn this opaque rect on by setting this global

		gptrcMapOpaque = &gtrcMapOpaque;
	} else {
		gptrcMapOpaque = NULL;
	}

	// Get visible gobs

	Gob **ppgobVisible;
	int cgobVisible;
	gsim.FindVisibleGobs(&ppgobVisible, &cgobVisible);

	// Add known invalid gobs to the update map

	Gob **ppgobStop = &ppgobVisible[cgobVisible];
	Gob **ppgobT;
	for (ppgobT = ppgobVisible; ppgobT < ppgobStop; ppgobT++) {
		Gob *pgob = *ppgobT;

		// Invalidate if asked to redraw

		if (pgob->GetFlags() & kfGobRedraw)
			pgob->Invalidate();
	}

	// Mark gobs not already marked for redraw that are touching invalid areas
	// Add this "overflow" to the damage map; this is what is used to create
	// a valid back buffer when scrolling

 	bool fNewInvalid = false;
 	do {
 		fNewInvalid = false;
 		for (ppgobT = ppgobVisible; ppgobT < ppgobStop; ppgobT++) {
 			Gob *pgob = *ppgobT;
			dword ff = pgob->GetFlags();
			if (!(ff & kfGobRedraw)) {
 				if (pupd->IsMapTileRectInvalidAndTrackDamage(&pgob->m_trcBoundingLast, &fNewInvalid))
 					pgob->SetFlags(ff | kfGobRedraw);
 			}
 		}
 	} while (fNewInvalid);

	// Let the base invalidate controls as necessary

	Form::OnUpdateMapInvalidate(pupd, prcOpaque);
}

void SimUIForm::InvalidateDragSelection()
{
	if (gfDragSelecting) {
		WCoord wxView, wyView;
		gsim.GetViewPos(&wxView, &wyView);

		// PERF: there is a way to make selection drawing faster by only invalidating edges
		// that move, and redrawing dirty cells only on the control layer. Can do if this isn't fast enough.

		int xLeft = PcFromWc(gwrcSelection.left - wxView);
		int yTop = PcFromWc(gwrcSelection.top - wyView);
		int xRight = PcFromWc(gwrcSelection.right - wxView);
		int yBottom = PcFromWc(gwrcSelection.bottom - wyView);

		Rect rcT;
		rcT.Set(xLeft, yTop, xRight, yTop + 1);
		gpupdSim->InvalidateRect(&rcT);
		rcT.Set(xRight - 1, yTop, xRight, yBottom);
		gpupdSim->InvalidateRect(&rcT);
		rcT.Set(xLeft, yBottom - 1, xRight, yBottom);
		gpupdSim->InvalidateRect(&rcT);
		rcT.Set(xLeft, yTop, xLeft + 1, yBottom);
		gpupdSim->InvalidateRect(&rcT);
		
	}
}

void SimUIForm::ScrollInvalidate(UpdateMap *pupd)
{
	Form::ScrollInvalidate(pupd);
	InvalidateDragSelection();
	gpmm->Invalidate();
}

void SimUIForm::OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd)
{
	if (!s_fReadyForPaint)
		return;

	gsim.DrawBackground(pupd, pbm);
}

void SimUIForm::OnPaint(DibBitmap *pbm)
{
	// We're not ready to paint until the first Update has executed because
	// Update initializes things like the Credits and Power controls.

	if (!s_fReadyForPaint)
		return;

	// Draw everything managed by the Simulation (map, Gobs, Gob UI)

	gsim.Draw(m_pfrmm->GetUpdateMap(), pbm);

	// Draw the selection rectangle or lasso lines, if any

	if (gfDragSelecting) {
		WCoord wxView, wyView;
		gsim.GetViewPos(&wxView, &wyView);

		Rect rcT;
		rcT.left = PcFromWc(gwrcSelection.left - wxView);
		rcT.top = PcFromWc(gwrcSelection.top - wyView);
		rcT.right = PcFromWc(gwrcSelection.right - wxView);
		rcT.bottom = PcFromWc(gwrcSelection.bottom - wyView);
		DrawBorder(pbm, &rcT, 1, GetColor(kiclrWhite));
		
	}

	// Handle placement form

	gpfrmPlace->OnPaintSimUI(pbm);

    // Let the input handler draw what it needs

    m_ppenh->OnPaint(pbm);

	// Call the default Form handling

	Form::OnPaint(pbm);
}

void SimUIForm::OnPaintControls(DibBitmap *pbm, UpdateMap *pupd)
{
	// We're not ready to paint until the first Update has executed because
	// Update initializes things like the Credits and Power controls.

	if (!s_fReadyForPaint)
		return;


	// Draw fog

	gsim.DrawFog(pupd, pbm);

	// Draw move indicator

	if (m_nStateMoveTarget == 1) {
		WCoord wxView, wyView;
		gsim.GetViewPos(&wxView, &wyView);
		m_aniMoveTarget.Draw(pbm, PcFromUwc(m_wxMoveTarget) - (PcFromWc(wxView) & 0xfffe), PcFromUwc(m_wyMoveTarget) - (PcFromWc(wyView) & 0xfffe));
	}


	// Draw controls

	Form::OnPaintControls(pbm, pupd);

	// Handle Structure Placement controls here

	gpfrmPlace->OnPaintControlsSimUI(pbm, pupd); 

	// Draw stats

#ifdef STATS_DISPLAY
	if (gfShowStats) {
		Font *pfnt = gapfnt[kifntDefault];
		int cy = pfnt->GetHeight();
		int y = cy;
		char szT[80];

		// Display useful info

		// Count of active Gobs

		int cpgob = 0;
		for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT), cpgob++);
		sprintf(szT, "active Gobs: %d", cpgob);
		pfnt->DrawText(pbm, szT, 1, y);
		y += cy;

		// Count of visible sprites

		sprintf(szT, "bitmaps drawn: %d", gcBitmapsDrawn);
		pfnt->DrawText(pbm, szT, 1, y);
		y += cy;

		// Count of messages per update

		sprintf(szT, "msgs/update: %d", gcMessagesPerUpdate);
		pfnt->DrawText(pbm, szT, 1, y);
		y += cy;

		// Count of updates

		sprintf(szT, "update count: %ld", gsim.GetUpdateCount());
		pfnt->DrawText(pbm, szT, 1, y);
		y += cy;

		// Invalidate

		Rect rcT = m_rc;
		rcT.top = cy;
		rcT.bottom = y;
		InvalidateRect(&rcT);
	}
#endif

#ifdef STATS_DISPLAY
	s_cUpdateRects += gpmfrmm->GetUpdateRectCount();
#endif
}

void SimUIForm::FrameComplete()
{
    Form::FrameComplete();
}

// SimUI handles input events which may effect the whole playfield and/or
// multiple Gobs. It does as much as it can independent of the Gobs but 
// passes context-specific messages to the Gobs when specialized responses 
// are desired.
//
// SimUI's OnPenEvent handler is responsible for
//
// - individual selection/multiple selection
// - input routing, i.e., cooking and passing pertinent events to the 
//   appropriate Gobs. For example, passing selected Gobs SetTarget and
//   invoking Gob::PopupMenu.
//

// Right/middle buttons and the wheel (see MultiFormMgr::CookBattlefieldEvent)

bool SimUIForm::EventProc(Event *pevt)
{
    switch (pevt->eType) {
    case rightDownEvent:
        OnRightClick(pevt->x, pevt->y);
        return true;

    case middleDownEvent:
        m_fPanning = true;
        m_xPanLast = pevt->x;
        m_yPanLast = pevt->y;
        return true;

    case middleMoveEvent:
        if (m_fPanning) {
            // Drag the map: it follows the mouse
            ScrollViewBy(WcFromPc(m_xPanLast - pevt->x),
                    WcFromPc(m_yPanLast - pevt->y));
            m_xPanLast = pevt->x;
            m_yPanLast = pevt->y;
        }
        return true;

    case middleUpEvent:
        m_fPanning = false;
        return true;

    case wheelEvent:
        {
            // The wheel scrolls the map, three tiles a notch; Shift+wheel
            // scrolls sideways
            WCoord dwc = -(int)pevt->dw * WcFromTile16ths(16 * 3);
            if (pevt->ffMods & kfModShift) {
                ScrollViewBy(dwc, 0);
            } else {
                ScrollViewBy(0, dwc);
            }
        }
        return true;
    }
    return Form::EventProc(pevt);
}

// Hotkeys on the battlefield:
//   1-9          select control group (twice quickly: also centre on it)
//   Ctrl+1-9     make the selection control group 1-9
//   Shift+1-9    add the selection to control group 1-9
//   A            attack-move: the next left-click is the destination
//   S            stop
//   Space        centre on the last "under attack" alert
//   Esc          cancel attack-move / building placement, else deselect

bool SimUIForm::OnBattlefieldKey(Event *pevt)
{
    dword ffMods = pevt->ffMods;
    int chr = pevt->chr;
    if (chr >= 'A' && chr <= 'Z')
        chr += 'a' - 'A';

    if (chr >= '1' && chr <= '9') {
        int nGroup = chr - '0';
        if (ffMods & kfModCtrl) {
            SetGroup(nGroup, false);
        } else if (ffMods & kfModShift) {
            SetGroup(nGroup, true);
        } else {
            SelectGroup(nGroup);
        }
        return true;
    }

    if (ffMods & (kfModCtrl | kfModAlt))
        return false;

    switch (chr) {
    case 'a':
        {
            Gid agid[1];
            if (GetOwnSelection(agid, 1, true) != 0)
                SetAttackMoveArmed(true);
        }
        return true;

    case 's':
        {
            Gid *pgid = new Gid[kcpgobMax];
            int cgid = GetOwnSelection(pgid, kcpgobMax, true);
            for (int i = 0; i < cgid; i++)
                gcmdq.Enqueue(kmidStopCommand, pgid[i]);
            delete[] pgid;
        }
        return true;

    case ' ':
        if (gfLastAlertValid)
            CenterViewOn(gwptLastAlert.wx, gwptLastAlert.wy);
        return true;

    case 0x1b: // Esc
        if (m_fAttackMoveArmed) {
            SetAttackMoveArmed(false);
        } else if (gpfrmPlace != NULL && gpfrmPlace->IsBusy()) {
            gpfrmPlace->OnControlSelected(kidcCancel);
        } else {
            gsim.ClearGobSelection();
        }
        return true;
    }
    return false;
}

void SimUIForm::SetAttackMoveArmed(bool fArmed)
{
    m_fAttackMoveArmed = fArmed;
    HostSetTargetCursor(fArmed);
}

// A + left-click: the selected units move there, attacking enemies they meet
// on the way. Clicking an enemy attacks it.

void SimUIForm::AttackMoveTo(int x, int y)
{
    SetAttackMoveArmed(false);
    WCoord wxTarget, wyTarget;
    Gob *pgobHit = HitTestGob(x, y, &wxTarget, &wyTarget);
    if (pgobHit != NULL && pgobHit->GetSide() != gpplrLocal->GetSide()) {
        MoveOrAttackOrSelect(pgobHit, wxTarget, wyTarget, kfMasAttack);
        return;
    }
    gfAttackMoveOrder = true;
    MoveOrAttackOrSelect(NULL, wxTarget, wyTarget, kfMasMove);
    gfAttackMoveOrder = false;
}

// The player's selected units (and towers), optionally mobile units only

int SimUIForm::GetOwnSelection(Gid *agid, int cgidMax, bool fMobileOnly)
{
    int cgid = 0;
    for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL && cgid < cgidMax;
            pgobT = ggobm.GetNextGob(pgobT)) {
        dword ff = pgobT->GetFlags();
        if ((ff & (kfGobUnit | kfGobSelected | kfGobActive)) !=
                (kfGobUnit | kfGobSelected | kfGobActive))
            continue;
        if (fMobileOnly && (ff & kfGobMobileUnit) == 0)
            continue;
        if (((UnitGob *)pgobT)->GetOwner() != gpplrLocal)
            continue;
        agid[cgid++] = pgobT->GetId();
    }
    return cgid;
}

void SimUIForm::SetGroup(int nGroup, bool fAdd)
{
    Gid *pgid = new Gid[kcgidGroupMax];
    int cgid = GetOwnSelection(pgid, kcgidGroupMax, false);
    if (cgid == 0) {
        delete[] pgid;
        return;
    }
    if (!fAdd)
        m_acgidGroup[nGroup] = 0;
    for (int i = 0; i < cgid; i++) {
        bool fHave = false;
        for (int j = 0; j < m_acgidGroup[nGroup]; j++) {
            if (m_aagidGroup[nGroup][j] == pgid[i])
                fHave = true;
        }
        if (!fHave && m_acgidGroup[nGroup] < kcgidGroupMax)
            m_aagidGroup[nGroup][m_acgidGroup[nGroup]++] = pgid[i];
    }
    delete[] pgid;
}

void SimUIForm::SelectGroup(int nGroup)
{
    // Select the members still alive

    bool fAny = false;
    long tNow = HostGetTickCount();
    WCoord wxSum = 0, wySum = 0;
    int cSum = 0;
    int cgidLive = 0;
    for (int i = 0; i < m_acgidGroup[nGroup]; i++) {
        Gob *pgob = ggobm.GetGob(m_aagidGroup[nGroup][i]);
        if (pgob == NULL || (pgob->GetFlags() & kfGobUnit) == 0 ||
                ((UnitGob *)pgob)->GetOwner() != gpplrLocal)
            continue;
        m_aagidGroup[nGroup][cgidLive++] = m_aagidGroup[nGroup][i];
        if (!fAny) {
            gsim.ClearGobSelection();
            fAny = true;
        }
        ((UnitGob *)pgob)->Select(true);
        WPoint wpt;
        pgob->GetCenter(&wpt);
        wxSum += wpt.wx;
        wySum += wpt.wy;
        cSum++;
    }
    m_acgidGroup[nGroup] = cgidLive;
    if (!fAny)
        return;

    // Pressed twice quickly: centre on the group

    if (m_nGroupLast == nGroup && tNow - m_tGroupLast < kctDoubleClick)
        CenterViewOn(wxSum / cSum, wySum / cSum);
    m_nGroupLast = nGroup;
    m_tGroupLast = tNow;
}

void SimUIForm::CenterViewOn(WCoord wx, WCoord wy)
{
    Size sizPlayfield;
    ggame.GetPlayfieldSize(&sizPlayfield);
    gsim.SetViewPos(wx - WcFromPc(sizPlayfield.cx) / 2,
            wy - WcFromPc(sizPlayfield.cy) / 2);
    gevm.SetRedrawFlags(kfRedrawDirty | kfRedrawBeforeTimer);
}

void SimUIForm::ScrollViewBy(WCoord dwx, WCoord dwy)
{
    if (dwx == 0 && dwy == 0)
        return;
    WCoord wxView, wyView;
    gsim.GetViewPos(&wxView, &wyView);
    gsim.SetViewPos(wxView + dwx, wyView + dwy);
    gevm.SetRedrawFlags(kfRedrawDirty | kfRedrawBeforeTimer);
}

// Right-click: on the ground, move the selected units there; on an enemy,
// attack it; on one of the player's own units or buildings, open its menu.

void SimUIForm::OnRightClick(int x, int y)
{
    if (gpfrmPlace != NULL && gpfrmPlace->IsBusy())
        return;

    // Right-click cancels a pending attack-move

    if (m_fAttackMoveArmed) {
        SetAttackMoveArmed(false);
        return;
    }

    WCoord wxTarget, wyTarget;
    Gob *pgobHit = HitTestGob(x, y, &wxTarget, &wyTarget);
    if (pgobHit != NULL && pgobHit->GetType() != kgtReplicator &&
            (pgobHit->GetFlags() & kfGobUnit) != 0 &&
            (gfGodMode || pgobHit->GetSide() == gpplrLocal->GetSide())) {
        pgobHit->PopupMenu();
        return;
    }
    MoveOrAttackOrSelect(pgobHit, wxTarget, wyTarget, kfMasMove | kfMasAttack);
}

bool SimUIForm::OnPenEvent(Event *pevt)
{
    // Give the form controls first crack at handling the event.  Form
    // controls include: soft menu button, mode cancel button, status
    // label)

    if (Form::OnPenEvent(pevt)) {
        return true;
    }

    // Debug gob identification

    if (pevt->eType == penHoverEvent) {
        return true;
    }

    bool fScrollOnly = false;
    if (gpfrmPlace != NULL && gpfrmPlace->IsBusy()) {
        fScrollOnly = true;
    }

    return m_ppenh->OnPenEvent(pevt, fScrollOnly);
}

/*
var ar = new Array(64)
for (r = 0; r < 64; r++) {
	c = 0;
	for (y = 50; y >= -50; y--) {
		for (x = 50; x >= -50; x--) {
			d = Math.sqrt(x * x + y * y);
			if (d <= r)
				c++;
		}
	}

	ar[r] = c;
}

for (r = 0; r < 64; r++) {
	WScript.Echo("Range " + r + " fits " + ar[r]);
}

var str = "";
n = 0;
for (r = 0; r < 64; r++) {
	while (n <= ar[r]) {
		if (n % 16 == 0) {
			WScript.Echo(str);
			str = "";
		}
		str += r + ", ";
		n++;
	}
}
*/

// Map unit count to target range, i.e. what circle radius will fit a given count of units

byte gmpcUnitsToRadius[] = 
{
	0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10,
};

int RadiusFromUnitCount(int cUnits)
{
	TCoord tcRadius;
	if (cUnits >= ARRAYSIZE(gmpcUnitsToRadius)) {
		tcRadius = gmpcUnitsToRadius[ARRAYSIZE(gmpcUnitsToRadius) - 1];
	} else {
		tcRadius = gmpcUnitsToRadius[cUnits];
	}
	return tcRadius;
}

MobileUnitGob *SimUIForm::SetSelectionTargets(Gid gid, WCoord wxTarget, WCoord wyTarget)
{
	// Find a target that is free and close

	WPoint wptT;
	FindNearestFreeTile(TcFromWc(wxTarget), TcFromWc(wyTarget), &wptT, 0);
	wxTarget = wptT.wx;
	wyTarget = wptT.wy;

	// Find targets for all MobileUnits. Targets are a "closely packed" version
	// of the current selection, centered around the midpoint of the selection,
	// placed at the target.

    // First figure out which units are mobile and attempt to retain their formation   

    WCoord wxMin = kwcMax;   
    WCoord wxMax = 0;   
    WCoord wyMin = kwcMax;   
    WCoord wyMax = 0;   

	WCoord wcMoveDistPerUpdate = (WCoord)kwcMax;
	int cgobsSel = 0;
    Gob *pgobT;   
    for (pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {   
        if ((pgobT->GetFlags() & (kfGobSelected | kfGobMobileUnit)) == (kfGobSelected | kfGobMobileUnit) &&    
		        (gfGodMode || pgobT->GetSide() == gpplrLocal->GetSide())) {   

			// Get selection dimensions

            WPoint wpt;
			pgobT->GetCenter(&wpt);
            if (wpt.wx < wxMin)   
                    wxMin = wpt.wx;   
            if (wpt.wx > wxMax)   
                    wxMax = wpt.wx;   
            if (wpt.wy < wyMin)   
                    wyMin = wpt.wy;   
            if (wpt.wy > wyMax)   
                    wyMax = wpt.wy;   
			cgobsSel++;

			// Get min speed per update

			if (pgobT->GetFlags() & kfGobMobileUnit) {
				MobileUnitGob *pmunt = (MobileUnitGob *)pgobT;
				MobileUnitConsts *pmuntc = (MobileUnitConsts *)pmunt->GetConsts();
				if (pmuntc->GetMoveDistPerUpdate() < wcMoveDistPerUpdate)
					wcMoveDistPerUpdate = pmuntc->GetMoveDistPerUpdate();
			}
        }   
    }
	WCoord cwxSel = wxMax - wxMin;
	if (cwxSel <= 0)
		cwxSel = 1;
	WCoord cwySel = wyMax - wyMin;
	if (cwySel <= 0)
		cwySel = 1;

	// The dest scaling rect is different if moving vs. attacking.

	int ctxSel = TcFromWc(wxMax) - TcFromWc(wxMin) + 1;
	int ctySel = TcFromWc(wyMax) - TcFromWc(wyMin) + 1;
	int ctxTarg;
	int ctyTarg;
	if (gid == kgidNull) {
		// Pick a destination scaling rect for movement

		ctxTarg = ctxSel;
		ctyTarg = ctySel;

		// Pack the rect if it is <= 50% full

		if (cgobsSel <= (ctxSel * ctySel + 1) / 2) {
			int ctxT = ctxSel;
			int ctyT = ctySel;
			while (true) {
				// Remember last 

				ctxTarg = ctxT;
				ctyTarg = ctyT;

				// Try smaller size, decrease whichever size is greater

				if (ctxT > ctyT) {
					ctxT--;
				} else {
					ctyT--;
				}

				if (ctxT * ctyT < cgobsSel + cgobsSel / 3)
					break;
			}
		}

	} else {
		// Pick a destination scaling rect for attacking

		Gob *pgob = ggobm.GetGob(gid);
		ctxTarg = 1;
		ctyTarg = 1;
		if (pgob != NULL) {
			TRect trc;
			pgob->GetTileRect(&trc);
			ctxTarg = trc.right - trc.left;
			ctyTarg = trc.bottom - trc.top;
		}

		// Make the outside edge away from the gob; this hardwires some accounting
		// for some firing distance, gives a larger edge for gobs to move to

		ctxTarg += 2;
		ctyTarg += 2;
	}

	WCoord cwxTarg = WcFromTc(ctxTarg - 1);
	if (cwxTarg == 0)
		cwxTarg = 1;
	WCoord cwyTarg = WcFromTc(ctyTarg - 1);
	if (cwyTarg == 0)
		cwyTarg = 1;

	// Figure out the range of this target

	TCoord tcTargetRadius = RadiusFromUnitCount(cgobsSel);

    // Find src center   

    WCoord wxCenterSrc = wxMin + (wxMax - wxMin) / 2;   
    WCoord wyCenterSrc = wyMin + (wyMax - wyMin) / 2;   

	// Find map size

    Size sizMapTiles; 
    gsim.GetLevel()->GetTileMap()->GetTCoordMapSize(&sizMapTiles); 
	WCoord cwxMap = WcFromTc(sizMapTiles.cx);
	WCoord cwyMap = WcFromTc(sizMapTiles.cy);

	// Move & keep formation, or target destination if not a mobile unit

	Gob *pgobTarget = ggobm.GetGob(gid);

	MobileUnitGob *pmuntSetTarget = NULL;
	for (pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
		dword ff = pgobT->GetFlags();
		if ((ff & kfGobSelected) && 
				(gfGodMode || pgobT->GetSide() == gpplrLocal->GetSide() || (pgobTarget != NULL && pgobTarget->GetType() == kgtReplicator))) {

			// If it's a mobile gob, give it a move command to a scaled coordinate

			if (ff & kfGobMobileUnit) {
				// Calc dst

 				MobileUnitGob *pmunt = (MobileUnitGob *)pgobT;
 				WPoint wptSrc;
				pmunt->GetCenter(&wptSrc);
				WCoord wxDst;
				if (wptSrc.wx - wxCenterSrc >= 0) {
 					wxDst = (WCoord)(((long)(wptSrc.wx - wxCenterSrc) * cwxTarg * 8 + (cwxSel * 4)) / (cwxSel * 8) + wxTarget);
				} else {
 					wxDst = (WCoord)(((long)(wptSrc.wx - wxCenterSrc) * cwxTarg * 8 - (cwxSel * 4)) / (cwxSel * 8) + wxTarget);
				}
 				if (wxDst < 0)
 					wxDst = 0;
 				if (wxDst >= cwxMap)
 					wxDst = cwxMap - 1;
 				WCoord wyDst;
				if (wptSrc.wy - wyCenterSrc >= 0) {
					wyDst = (WCoord)(((long)(wptSrc.wy - wyCenterSrc) * cwyTarg * 8 + (cwySel * 4)) / (cwySel * 8) + wyTarget);
				} else {
					wyDst = (WCoord)(((long)(wptSrc.wy - wyCenterSrc) * cwyTarg * 8 - (cwySel * 4)) / (cwySel * 8) + wyTarget);
				}
 				if (wyDst < 0)
 					wyDst = 0;
 				if (wyDst >= cwyMap)
 					wyDst = cwyMap - 1;
				if (gfGodMode)
					pmunt->ClearAction();

				// Need to clean up the targets.
				// 1. Targets can be either in open terrain or on the unit being targetted,
				//    not on other structures (other munts is ok)
				// 2. For any targets on the munt being targetted, if it's a structure it needs
				//    to be checked for accessibility
				// 3. Adjust the target radius to be on the same side of any blockage

				// Clean up dest if attacking

				if (pgobTarget != NULL) {
					// Find the closest tile to this that is unblocked by terrain
					// Ok if on a structure... which structure is important, that is checked next

					WPoint wptT;
					FindNearestFreeTile(TcFromWc(wxDst), TcFromWc(wyDst), &wptT, 0);
					wxDst = wptT.wx;
					wyDst = wptT.wy;

					// Calc these now as wxDst, wyDst may have changed

					TCoord txT = TcFromWc(wxDst);
					TCoord tyT = TcFromWc(wyDst);

					// If dest is on top of a structure make sure that is the target and dest
					// is accessible

					Assert(pgobTarget->GetFlags() & kfGobUnit);
					UnitGob *puntTarget = (UnitGob *)pgobTarget;
					Gob *pgobT = ggobm.GetShadowGob(txT, tyT);
					if (pgobT != NULL) {
                        // The destination is on top of a structure. Need to
                        // check if this is ok If it isn't ok, get a new point
                        // that is accessible. If it is ok, make sure the
                        // position is accessible.

						Assert(pgobT->GetFlags() & kfGobStructure);
						StructGob *pstruT = (StructGob *)pgobT;
						if (pgobT != pgobTarget) {
                            // Targetting something but wxDst, wyDst on a
                            // structure we're not targetting.  Get a valid
                            // attack point from the real target

							WPoint wptT;
							puntTarget->GetAttackPoint(&wptT);
							wxDst = wptT.wx;
							wyDst = wptT.wy;
						} else {
                            // wxDst, wyDst is on top of the structure we are
                            // targetting which is fine. Get an accessible
                            // position if the one we have isn't accessible

							if (!pstruT->IsAccessible(txT, tyT)) {
								WPoint wptT;
								puntTarget->GetAttackPoint(&wptT);
								wxDst = wptT.wx;
								wyDst = wptT.wy;
							}
						}
					}
				} else {
					// Not attacking, just moving.
                    // Find a clear terrain spot that isn't terrain blocked and
                    // not on a structure

					WPoint wptT;
					FindNearestFreeTile(TcFromWc(wxDst), TcFromWc(wyDst), &wptT, kbfStructure);
					wxDst = wptT.wx;
					wyDst = wptT.wy;
				}

                // Adjust the target radius so we end up on the same side of
                // any blockage

				TCoord tcTargetRadiusT = tcTargetRadius;
				TerrainMap *ptrmap = gsim.GetLevel()->GetTerrainMap();
				TCoord txTo = TcFromWc(wxTarget);
				TCoord tyTo = TcFromWc(wyTarget);
				TCoord txFrom = TcFromWc(wxDst);
				TCoord tyFrom = TcFromWc(wyDst);
				TCoord txFree, tyFree;
				if (ptrmap->FindLastUnoccupied(txTo, tyTo, txFrom, tyFrom, &txFree, &tyFree)) {
					if (txFree != txFrom || tyFree != tyFrom) {
						wxDst = WcFromTc(txFree) + kwcTileHalf;
						wyDst = WcFromTc(tyFree) + kwcTileHalf;
						int dtx = abs(txFree - txTo);
						if (dtx < ARRAYSIZE(gmpDistFromDxy)) {
							int dty = abs(tyFree - tyTo);
							if (dty < ARRAYSIZE(gmpDistFromDxy)) {
								tcTargetRadiusT = gmpDistFromDxy[dtx][dty];
							}
						}
					}
				}

				// Send command

 				pmunt->SetTarget(gid, wxDst, wyDst, wxTarget, wyTarget, tcTargetRadiusT, wcMoveDistPerUpdate);
				pmuntSetTarget = pmunt;
			}
			
            // Else give it a set target command with the original target
            // coordinate

			else if (ff & kfGobUnit) {
				UnitGob *punt = (UnitGob *)pgobT;
				punt->SetTarget(gid, wxTarget, wyTarget, 0, wcMoveDistPerUpdate);
			}
		}
	}

	return pmuntSetTarget;
}

Gob *SimUIForm::HitTestGob(int x, int y, WCoord *pwx, WCoord *pwy)
{
	WCoord wxTarget, wyTarget;
	gsim.GetViewPos(&wxTarget, &wyTarget);
	wxTarget += WcFromPc(x);
	wyTarget += WcFromPc(y);

	// Pin to edges. Gobs created off the map will cause corruption

	TCoord ctx, cty;
	ggobm.GetMapSize(&ctx, &cty);
	if (TcFromWc(wxTarget) >= ctx)
		wxTarget = WcFromTc(ctx - 1);
	if (TcFromWc(wyTarget) >= cty)
		wyTarget = WcFromTc(cty - 1);

    *pwx = wxTarget;
    *pwy = wyTarget;

    Gob *pgobHit = NULL;
    Enum enm;
    gsim.HitTest(&enm, wxTarget, wyTarget, kfGobActive | kfGobUnit, &pgobHit);
    return pgobHit;
}

void SimUIForm::MoveOrAttackOrSelect(int x, int y, dword ff)
{
	WCoord wxTarget, wyTarget;
    Gob *pgobHit = HitTestGob(x, y, &wxTarget, &wyTarget);
    MoveOrAttackOrSelect(pgobHit, wxTarget, wyTarget, ff);
}

// This routine is called in two situations.
// 1. On pen down to select the unit tapped on (fSelectOnly == true)
// 2. On pen up to attack the unit tapped on or to move to the location tapped on (fSelectOnly == false)

const int kctDoubleTapDelay = 100; // TUNE:

void SimUIForm::MoveOrAttackOrSelect(Gob *pgobHit, WCoord wxTarget,
        WCoord wyTarget, dword ff)
{
    // TODO: break up this giant routine

    if (pgobHit == NULL) {
		if (ff & kfMasMove) {
			MobileUnitGob *pmunt = SetSelectionTargets(kgidNull, wxTarget, wyTarget);
			if (pmunt != NULL) {
				// BUGBUG: fix this multiplayer problem (gob lists out of sync)
				// Single and multi-player friendly move target

				if (m_nStateMoveTarget != 0)
					m_pfrmm->GetUpdateMap()->InvalidateTile(TcFromWc(m_wxMoveTarget), TcFromWc(m_wyMoveTarget));
				m_aniMoveTarget.Start(0, 0, 0);
				m_wxMoveTarget = WcTrunc(wxTarget) + kwcTileHalf;
				m_wyMoveTarget = WcTrunc(wyTarget) + kwcTileHalf;
				m_nStateMoveTarget = 1;
				gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmunt->GetUnitType()])->sfxcMove));
			}

#ifdef RALLY_POINTS
			// TOTAL HACK:
            // If the player has HRC or VTS selected and they've tapped the
            // ground set that point as the structure's rally point. Of course
            // this should be visualized, etc. Ah, if only we had time for such
            // things.

			for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
				if ((pgobT->GetFlags() & (kfGobStructure | kfGobSelected)) != (kfGobStructure | kfGobSelected))
					continue;
				BuilderGob *pbldr = (BuilderGob *)pgobT;

				// We know that if an enemy structure is selected then a local player's
				// structure can't also be selected.

				if (pgobT->GetOwner() != gpplrLocal)
					break;

				// VTS or HRC?

				if (!((1UL << pbldr->GetUnitType()) & (kumVehicleTransportStation | kumHumanResourceCenter)))
					break;

				pbldr->SetRallyPoint(TcFromWc(wxTarget), TcFromWc(wyTarget));
				WCoord wxT = WcTrunc(wxTarget) + kwcTileHalf;
				WCoord wyT = WcTrunc(wyTarget) + kwcTileHalf;
				CreateAnimGob(wxT, wyT, kfAnmDeleteWhenDone | kfAnmSmokeFireLayer, NULL, g_panidMoveTarget);
				ShowAlert("Rally point set");
				break;
			}
#endif
		}
		return;
	}

    // Replicator is a special case. When it is specified as a target the
    // selected units are directed to move to its input.

	if (pgobHit->GetType() == kgtReplicator) {
		if ((ff & (kfMasMove | kfMasAttack)) == 0) {
			return;
        }

		MobileUnitGob *pmunt = SetSelectionTargets(pgobHit->GetId(), wxTarget, wyTarget);
		if (pmunt != NULL)
			gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmunt->GetUnitType()])->sfxcMove));
		return;
	}


	// God mode is allowed only if gfGodMode is on

	bool fGodMode = false;
	if (gfGodMode)
		fGodMode = true;

	// Selection?

	if (IsSelectionCommand(pgobHit)) {
		static long s_tLastSelected = 0;
		static Gob *s_pgobLastSelected = NULL;

		if (ff & kfMasSelect) {
			bool fAlreadySelected = (pgobHit->GetFlags() & kfGobSelected) != 0;

            // Selection. Cancel selection for units elsewhere, set selection
            // for this unit

            // If the unit is already selected and the last selection occured
            // in within the double-tap window then select all units of the
            // same type on-screen.  UNDONE: this may not be the best place to
            // do this.

			if (fAlreadySelected && HostGetTickCount() - s_tLastSelected < kctDoubleTapDelay && s_pgobLastSelected == pgobHit) {
                SelectSameUnitTypes(pgobHit, false);
			} else {
				gsim.SetGobSelected(pgobHit);
			}
			s_tLastSelected = HostGetTickCount();
			s_pgobLastSelected = pgobHit;

			// Play sfx

			if (!gfGodMode) {
				if (pgobHit->GetFlags() & kfGobStructure) {
					gsndm.PlaySfx(((StructConsts *)gapuntc[((StructGob *)pgobHit)->GetUnitType()])->sfxSelect);
				} else {
					if (gfGodMode || pgobHit->GetSide() == gpplrLocal->GetSide())
						gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[((UnitGob *)pgobHit)->GetUnitType()])->sfxcSelect));
				}
			}
		}

        if (ff & kfMasShowMenu) {
			if (pgobHit->GetFlags() & kfGobStructure) {
                ShowUnitMenu(pgobHit);
			}
		}
	} else if (ff & kfMasAttack) {
        // An attack. Reset the wxTarget, wyTarget to be the middle of the gob
        // and issue targets

		UnitGob *puntHit = (UnitGob *)pgobHit;
		WPoint wptTarget;
		puntHit->GetAttackPoint(&wptTarget);
		wxTarget = wptTarget.wx;
		wyTarget = wptTarget.wy;
		MobileUnitGob *pmunt = SetSelectionTargets(puntHit->GetId(), wxTarget, wyTarget);
		if (pmunt != NULL)
			gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[((UnitGob *)pmunt)->GetUnitType()])->sfxcAttack));
	}
}

bool SimUIForm::IsSelectionCommand(Gob *pgobHit)
{
	// God mode is allowed only if gfGodMode is on

	bool fGodMode = false;
	if (gfGodMode)
		fGodMode = true;

	// Figure out if this is a selection or an attack command:
	//
	// Player can select same side units
	// Player can select enemy units if no friendly units are selected
	// Player can attack enemy units
	// GodMode can select any unit if that doesn't imply an attack (from
    // either side)
	// GodMode can attack any side if there are selected enemy units

	// If there are any selected units that can attack pgobHit, it may be an
    // attack.

	bool fOwnSelection = false;
	bool fSelection = true;
	Side sideHit = pgobHit->GetSide();
	for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
		if ((pgobT->GetFlags() & (kfGobUnit | kfGobSelected)) != (kfGobUnit | kfGobSelected))
			continue;
		UnitGob *puntT = (UnitGob *)pgobT;
		if (puntT->GetOwner() == gpplrLocal)
			fOwnSelection = true;
		if (puntT->IsValidTarget(pgobHit)) {
			fSelection = false;
			break;
		}
	}

    // If not in god mode and the local player does not own any of the already
    // selected units then just select the new unit

	if ((!fGodMode) && (!fOwnSelection)) {
        fSelection = true;
    }

    return fSelection;
}

void SimUIForm::ShowUnitMenu(Gob *pgob) {
    if (pgob != NULL && (pgob->GetFlags() & kfGobUnit) != 0) {
        // Don't popup menu if the player doesn't own the Gob (unless in
        // God mode).
        if (!gfGodMode && 
                pgob->GetSide() != gpplrLocal->GetSide()) {
            return;
        }
        pgob->PopupMenu();
    }
}

// A left-click on a unit or building. Plain click selects it (a double-click
// selects all of the player's units of that type on screen, as does
// Ctrl+click); Shift+click adds the player's own unit to the selection or
// takes it out.

void SimUIForm::ClickSelect(Gob *pgob, dword ffMods)
{
    static long s_tLast = 0;
    static Gid s_gidLast = kgidNull;

    if ((pgob->GetFlags() & kfGobUnit) == 0)
        return;
    bool fOwn = gfGodMode || pgob->GetSide() == gpplrLocal->GetSide();
    long tNow = HostGetTickCount();
    bool fDouble = s_gidLast == pgob->GetId() && tNow - s_tLast < kctDoubleClick;
    s_tLast = tNow;
    s_gidLast = pgob->GetId();

    if ((ffMods & kfModShift) && fOwn) {
        // Can't mix in an enemy unit selected for a look
        for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
            if ((pgobT->GetFlags() & (kfGobUnit | kfGobSelected)) == (kfGobUnit | kfGobSelected) &&
                    pgobT->GetSide() != pgob->GetSide()) {
                ((UnitGob *)pgobT)->Select(false);
            }
        }
        bool fSelected = (pgob->GetFlags() & kfGobSelected) != 0;
        ((UnitGob *)pgob)->Select(!fSelected);
        if (!fSelected)
            PlaySelectSfx(pgob);
        return;
    }

    if ((fDouble || (ffMods & kfModCtrl)) && fOwn &&
            (pgob->GetFlags() & kfGobStructure) == 0) {
        gsim.SetGobSelected(pgob);
        SelectSameUnitTypes(pgob, false);
        return;
    }

    gsim.SetGobSelected(pgob);
    PlaySelectSfx(pgob);
}

void SimUIForm::PlaySelectSfx(Gob *pgob)
{
    if (gfGodMode)
        return;
    if (pgob->GetFlags() & kfGobStructure) {
        gsndm.PlaySfx(((StructConsts *)gapuntc[((StructGob *)pgob)->GetUnitType()])->sfxSelect);
    } else if (pgob->GetSide() == gpplrLocal->GetSide()) {
        gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[((UnitGob *)pgob)->GetUnitType()])->sfxcSelect));
    }
}

void SimUIForm::SelectSameUnitTypes(Gob *pgob, bool fSfx)
{
    if ((pgob->GetFlags() & (kfGobUnit | kfGobStructure)) != kfGobUnit) {
        return;
    }

    WCoord wxViewT, wyViewT;
    gsim.GetViewPos(&wxViewT, &wyViewT);
    TCoord txView = TcFromWc(wxViewT);
    TCoord tyView = TcFromWc(wyViewT);

    Size sizT;
    ggame.GetPlayfieldSize(&sizT);
    TCoord txRight = TcFromWc(wxViewT + WcFromPc(sizT.cx + gcxTile - 1));
    TCoord tyBottom = TcFromWc(wyViewT + WcFromPc(sizT.cy + gcyTile - 1));

    TRect trc;
    trc.Set(txView, tyView, txRight, tyBottom);
    gsim.SelectSameUnitTypes((UnitGob *)pgob, &trc);

    if (gfGodMode || pgob->GetSide() == gpplrLocal->GetSide()) {
        gsndm.PlaySfx(SfxFromCategory(((MobileUnitConsts *)
                gapuntc[((UnitGob *)pgob)->GetUnitType()])->sfxcSelect));
    }
}

void SimUIForm::CalcLevelSpecificConstants()
{
}

//
// MiniMapControl
//

MiniMapControl *gpmm;

MiniMapControl::MiniMapControl()
{
	m_pbm = NULL;
	m_cxyBorder = 0;
	m_wfMm = 0;
	m_xOff = 0;
	m_yOff = 0;
	m_tInvalidateLast = 0;
}

MiniMapControl::~MiniMapControl()
{
	gpmm = NULL;
	delete m_pbm;
}

int MiniMapControl::CalcWidth()
{
	// Figure out scale based on playfield size

	Size siz;
	ggame.GetPlayfieldSize(&siz);
	int nScale = 1;
	if (siz.cx >= 240)
		nScale = 2;

	// Width

	Size sizMap;
	gsim.GetLevel()->GetTileMap()->GetMapSize(&sizMap);
	return sizMap.cx / (gcxTile / nScale) + 1;
}

bool MiniMapControl::Init(Form *pfrm, IniReader *pini, FindProp *pfind)
{
	// Base initialization

	if (!Control::Init(pfrm, pini, pfind))
		return false;

	// idc (x y cx cy) border

	char szBorder[32];
	int cArgs = pini->GetPropertyValue(pfind, "%*d (%*d %*d %*d %*d) %s", szBorder);

	// Remember the size of the input UI area since our border drawing needs it

	Size sizDib;
	ggame.GetInputUIForm()->GetFormMgr()->GetDib()->GetSize(&sizDib);
	m_cyInputUI = sizDib.cy;

	// Our even aligned blts depend on having a border for odd width maps

	m_cxyBorder = 1;

	// Figure out scale based on playfield size

	Size siz;
	ggame.GetPlayfieldSize(&siz);
	m_nScale = 1;
	if (siz.cx >= 240 && siz.cx < 320) {
		if (siz.cy > siz.cx)
			m_nScale = 2;
	}
	if (siz.cx >= 320)
		m_nScale = 2;
	gsim.SetMiniMapScale(m_nScale);

	// Size

	Size sizMap;
	gsim.GetLevel()->GetTileMap()->GetMapSize(&sizMap);
	m_rc.left = m_rc.right - ((sizMap.cx / (gcxTile / m_nScale)) + m_cxyBorder);
	m_rc.bottom = m_rc.top + (sizMap.cy / (gcyTile / m_nScale)) + m_cxyBorder;

	// The map is aligned to an edge. The edge is even, and the dib starts on
	// an even address, so we want the width to be even so our blts are aligned.
	// If the map is odd, draw the border inside the dib to make it even. If the
	// map is even, draw the border outside the dib.

	int cx = m_rc.Width();
	if (cx & 1) {
		m_wfMm &= ~kfMmVertBorderInside;
		cx -= m_cxyBorder;
		m_xOff = 0;
		m_yOff = m_cxyBorder;
	} else {
		m_wfMm |= kfMmVertBorderInside;
		m_xOff = m_cxyBorder;
		m_yOff = m_cxyBorder;
	}

	// Alloc a dib to hold it

	m_pbm = CreateDibBitmap(NULL, cx, m_rc.Height());
	if (m_pbm == NULL)
		return false;

	// Draw internal borders

	int x = 0;
	int y = 0;
	if (m_cxyBorder != 0) {
		int iclr = kiclr0CyanSide;
		Color clrBorder = GetColor(iclr);
		if (m_wfMm & kfMmVertBorderInside) {
			m_pbm->Fill(x + m_cxyBorder, y, m_rc.Width() - m_cxyBorder, m_cxyBorder, clrBorder);
			m_pbm->Fill(x, y, m_cxyBorder, m_rc.Height() - m_cyInputUI + 1, clrBorder);
			m_pbm->Fill(x, y + m_rc.Height() - m_cyInputUI + 1, m_cxyBorder, m_rc.Height(), GetColor(kiclrBlack));
		} else {
			m_pbm->Fill(x, y, m_rc.Width(), m_cxyBorder, clrBorder);
		}
	}

	// Stuff constants

	TileMap *ptmap = gsim.GetLevel()->GetTileMap();
	MiniTileSetHeader *pmtseth = ptmap->GetMiniTileSetHeader(m_nScale);
	m_pbTileData = (byte *)(pmtseth + 1);
	m_pwTileMap = ptmap->m_pwMapData;
	m_pbFogMap = gsim.GetLevel()->GetFogMap()->GetMapPtr();
	ggobm.GetMapSize(&m_ctx, &m_cty);
	m_cbRowBytes = m_pbm->GetRowBytes();
	m_clrBlack = (byte)GetColor(kiclrBlack);
	m_clrWhite = (byte)GetColor(kiclrWhite);
	m_clrGalaxite = (byte)GetColor(kiclrGalaxite);
	for (Side sideT = ksideNeutral; sideT < kcSides; sideT++)
		m_aclrSide[sideT] = (byte)GetSideColor(sideT);

	// Calc powered radar flag

	if (!CalcPoweredRadar())
		Redraw();

	// Remember this object globally

	gpmm = this;
	return true;
}

void MiniMapControl::OnPaint(DibBitmap *pbm)
{
	Rect rcForm;
	m_pfrm->GetRect(&rcForm);
	int x = m_rc.left + rcForm.left;
	int y = m_rc.top + rcForm.top;

	// Lay down bits

	Size siz;
	m_pbm->GetSize(&siz);
	Rect rcSrc;
	rcSrc.Set(0, 0, siz.cx, siz.cy);

	if (m_wfMm & kfMmVertBorderInside) {
		pbm->Blt(m_pbm, &rcSrc, x, y);
	} else {
		pbm->Blt(m_pbm, &rcSrc, x + m_cxyBorder, y);
		int iclr = kiclr0CyanSide;
		pbm->Fill(x, y, m_cxyBorder, m_rc.Height() - m_cyInputUI + 1, GetColor(iclr));
		pbm->Fill(x, y + m_rc.Height() - m_cyInputUI + 1, m_cxyBorder, m_rc.Height(), GetColor(kiclrBlack));
	}

	// Draw box showing the full-screen visible area of the map

	WCoord wxViewT, wyViewT;
	gsim.GetViewPos(&wxViewT, &wyViewT);
	short xView = PcFromUwc(wxViewT) & 0xfffe;
	short yView = PcFromUwc(wyViewT) & 0xfffe;
	WCoord wxView = WcFromUpc(xView);
	WCoord wyView = WcFromUpc(yView);

	int cxTile = gcxTile / m_nScale;
	int cyTile = gcyTile / m_nScale;

	Rect rc;
	rc.left = x + m_cxyBorder + PcFromUwc(wxView) / cxTile;
	rc.top = y + m_cxyBorder + PcFromUwc(wyView) / cyTile;
	ggame.GetPlayfieldSize(&siz);
	rc.right = rc.left + (siz.cx + cxTile - 1) / cxTile;
	rc.bottom = rc.top + (siz.cy + cyTile - 1) / cyTile;
	DrawBorder(pbm, &rc, 1, GetColor(kiclrWhite));
}

//TUNE:
#define kctMiniMapInvalidateRate 25

void MiniMapControl::Update()
{	
    // Gets called at Update time. If the minimap is dirty and it's been at
    // least kctMiniMapUpdateRate ticks since the last invalidate, invalidate
    // it now. This gives us immediate invalidates in many cases, but ensures
    // there is at least kctMiniMapUpdateRate ticks between invalidates.

    long tCurrent = gtimm.GetTickCount();
	if (m_wfMm & kfMmRedraw) {
		if (m_tInvalidateLast == 0 || tCurrent - m_tInvalidateLast >= kctMiniMapInvalidateRate) {
			m_tInvalidateLast = tCurrent;
			m_wfMm &= ~kfMmRedraw;
			Invalidate();
		}
	}
}

void MiniMapControl::Redraw()
{
	TRect trc;
	trc.Set(0, 0, m_ctx, m_cty);
	RedrawTRect(&trc);
}

void MiniMapControl::OnPenEvent(Event *pevt)
{
	if (pevt->eType != penDownEvent && pevt->eType != penMoveEvent) {
		return;
    }

	static int s_xPenDown;
	static int s_yPenDown;
	static WCoord s_wxViewDown;
	static WCoord s_wyViewDown;

	Rect rcForm;
	m_pfrm->GetRect(&rcForm);

	int xPen = pevt->x - (rcForm.left + m_rc.left + m_cxyBorder);
	int yPen = pevt->y - (rcForm.top + m_rc.top + m_cxyBorder);

	WCoord wxView, wyView;
	gsim.GetViewPos(&wxView, &wyView);

	Size sizPlayfield;
	ggame.GetPlayfieldSize(&sizPlayfield);

	if (pevt->eType == penDownEvent) {
		// Calc the rectangle (in minimap pixel coordinates) bounding the map
		// area currently being viewed.

		Rect rcView;
		rcView.left = TcFromWc(wxView) * m_nScale;
		rcView.top = TcFromWc(wyView) * m_nScale;
		rcView.right = rcView.left + (((sizPlayfield.cx + gcxTile - 1) / gcxTile) * m_nScale);
		rcView.bottom = rcView.top + (((sizPlayfield.cy + gcyTile - 1) / gcyTile) * m_nScale);

        
		// Pen down inside the view rect?

		if (!rcView.PtIn(xPen, yPen)) {

			// No, set the view to be centered around the point tapped

			TCoord tx = xPen / m_nScale;
			TCoord ty = yPen / m_nScale;

			wxView = WcFromTc(tx - (sizPlayfield.cx / gcxTile) / 2);
			wyView = WcFromTc(ty - ((sizPlayfield.cy + gcyTile - 1) / gcyTile) / 2);
		}

		s_xPenDown = xPen;
		s_yPenDown = yPen;
		s_wxViewDown = wxView;
		s_wyViewDown = wyView;
	} else {
        int dx = xPen - s_xPenDown;
		wxView = s_wxViewDown + WcFromTc(dx / m_nScale);
        wxView += (dx % m_nScale) * kwcTile / m_nScale;
        int dy = yPen - s_yPenDown;
		wyView = s_wyViewDown + WcFromTc(dy / m_nScale);
        wyView += (dy % m_nScale) * kwcTile / m_nScale;
	}

	gsim.SetViewPos(wxView, wyView);
	Invalidate();
}

bool MiniMapControl::CalcPoweredRadar()
{
	// gpmm uninitialized?

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-undefined-compare"
#endif

	if (this == NULL)
		return false;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

	// Remember if the local player has fully powered Radar. This determines whether
	// enemy units show on the mini-map.

	bool fHasPoweredRadar = gpplrLocal->GetUnitCount(kutRadar) > 0 && gpplrLocal->GetPowerDemand() <= gpplrLocal->GetPowerSupply();
	word wfMm = fHasPoweredRadar ? kfMmHasPoweredRadar : 0;
	if ((wfMm ^ m_wfMm) != 0) {
		m_wfMm &= ~kfMmHasPoweredRadar;
		m_wfMm |= wfMm;
		Redraw();
		return true;
	}
	return false;
}

void MiniMapControl::RedrawTile(TCoord tx, TCoord ty)
{
	TRect trc;
	trc.left = tx;
	trc.top = ty;
	trc.right = tx + 1;
	trc.bottom = ty + 1;
	RedrawTRect(&trc);
}

void MiniMapControl::RedrawTRect(TRect *ptrc)
{
	// gpmm uninitialized?

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-undefined-compare"
#endif

	if (this == NULL)
		return;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

	// Redraw this rect

	byte *pbDst = m_pbm->GetBits() + (long)ptrc->top * m_cbRowBytes * m_nScale +
			ptrc->left * m_nScale + (long)m_yOff * m_cbRowBytes + m_xOff;
	int cbDstReturn = m_cbRowBytes - ptrc->Width() * m_nScale;
	long offset = (long)ptrc->top * m_ctx + ptrc->left;
	byte *pbFogMap = m_pbFogMap + offset;
	word *pwTileMap = m_pwTileMap + offset;
	int ctReturn = m_ctx - ptrc->Width();

	if (m_nScale == 1) {
		for (TCoord ty = ptrc->top; ty < ptrc->bottom; ty++) {
			for (TCoord tx = ptrc->left; tx < ptrc->right; tx++, pbFogMap++, pwTileMap++) {
				// Fogged?

				if (IsFogOpaque(*pbFogMap)) {
					*pbDst++ = m_clrBlack;
					continue;
				}

				// Not fogged; remember to redraw the minimap to the screen next timer

				m_wfMm |= kfMmRedraw;

				// Unit gob?

				UnitGob *punt = ggobm.GetUnitGob(tx, ty);

				// don't show inactive munts

				if (punt != NULL) {
					dword wf = punt->GetFlags();
					if ((wf & (kfGobMobileUnit | kfGobActive)) != (kfGobMobileUnit))  {
						if (wf & kfGobSelected) {
							*pbDst++ = m_clrWhite;
						} else {
							*pbDst++ = m_aclrSide[punt->GetSide()];
						}
						continue;
					}
				}

				// Galaxite?

				if (HasGalaxite(*pbFogMap)) {
					*pbDst++ = m_clrGalaxite;
					continue;
				}

				// Tile

				int nTile = (BigWord(*pwTileMap) & 0x7fc);
				*pbDst++ = m_pbTileData[nTile >> 2];
				continue;
			}
			pwTileMap += ctReturn;
			pbFogMap += ctReturn;
			pbDst += cbDstReturn;
		}
	} else if (m_nScale == 2) {
		for (TCoord ty = ptrc->top; ty < ptrc->bottom; ty++) {
			for (TCoord tx = ptrc->left; tx < ptrc->right; tx++, pbFogMap++, pwTileMap++) {
				// Fogged?

				if (IsFogOpaque(*pbFogMap)) {
					*pbDst++ = m_clrBlack;
					*pbDst++ = m_clrBlack;
					*(pbDst + m_cbRowBytes - 2) = m_clrBlack;
					*(pbDst + m_cbRowBytes - 1) = m_clrBlack;
					continue;
				}

				// Not fogged; remember to redraw the minimap to the screen next timer

				m_wfMm |= kfMmRedraw;

				// Unit gob?

				UnitGob *punt = ggobm.GetUnitGob(tx, ty);
				if (punt != NULL) {
					dword wf = punt->GetFlags();
					if ((wf & (kfGobMobileUnit | kfGobActive)) != (kfGobMobileUnit))  {
						byte clr;
						if (wf & kfGobSelected) {
							clr = m_clrWhite;
						} else {
							clr = m_aclrSide[punt->GetSide()];
						}

						*pbDst++ = clr;
						*pbDst++ = clr;
						*(pbDst + m_cbRowBytes - 2) = clr;
						*(pbDst + m_cbRowBytes - 1) = clr;
						continue;
					}
				}

				// Galaxite?

				if (HasGalaxite(*pbFogMap)) {
					*pbDst++ = m_clrGalaxite;
					*pbDst++ = m_clrGalaxite;
					*(pbDst + m_cbRowBytes - 2) = m_clrGalaxite;
					*(pbDst + m_cbRowBytes - 1) = m_clrGalaxite;
					continue;
				}

				// Tile

				int nTile = (BigWord(*pwTileMap) & 0x7fc);
				byte *pbSrc = &m_pbTileData[nTile];
				*pbDst++ = *pbSrc++;
				*pbDst++ = *pbSrc++;
				*(pbDst + m_cbRowBytes - 2) = *pbSrc++;
				*(pbDst + m_cbRowBytes - 1) = *pbSrc++;
				continue;
			}
			pwTileMap += ctReturn;
			pbFogMap += ctReturn;
			pbDst += cbDstReturn + m_cbRowBytes;
		}
	}
}

} // namespace wi
