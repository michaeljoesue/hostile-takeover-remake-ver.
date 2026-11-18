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
    
// Set while the miners of the selection are sent to a Processor and the
// rest move

static bool s_fSkipMiners = false;

SimUIForm::SimUIForm()
{
	s_fReadyForPaint = false;
	gfDragSelecting = false;
    
	m_fTimerAdded = false;

	m_nStateMoveTarget = 0;

    m_ppenh = new StylusHandler(this);
    m_fHover = false;
    m_xHover = 0;
    m_yHover = 0;
    m_rcHover.SetEmpty();
    m_pszHover = NULL;
    m_fPanning = false;
    m_xPanLast = 0;
    m_yPanLast = 0;
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

	// The hover label follows its unit

	UpdateHoverLabel(pupd);

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


	// Name of the unit or building under the mouse

	DrawHoverLabel(pbm);

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
        if ((pevt->ffMods & (kfModCtrl | kfModAlt)) == (kfModCtrl | kfModAlt)) {
            OnGuardClick(pevt->x, pevt->y, (pevt->ffMods & kfModShift) != 0);
            return true;
        }
        OnRightClick(pevt->x, pevt->y, (pevt->ffMods & kfModShift) != 0,
                (pevt->ffMods & kfModCtrl) != 0);
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
//   S            stop
//   Space        centre on the last "under attack" alert
//   Esc          cancel building placement, else deselect

bool SimUIForm::OnBattlefieldKey(Event *pevt)
{
    dword ffMods = pevt->ffMods;
    int chr = pevt->chr;
    if (chr >= 'A' && chr <= 'Z')
        chr += 'a' - 'A';

    if (ffMods & (kfModCtrl | kfModAlt))
        return false;

    switch (chr) {
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
        if (gpfrmPlace != NULL && gpfrmPlace->IsBusy()) {
            gpfrmPlace->OnControlSelected(kidcCancel);
        } else {
            gsim.ClearGobSelection();
        }
        return true;
    }
    return false;
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
// attack it; on one of the player's own units or buildings, move there.

// Ctrl+Alt+right-click (Red Alert style): on a friendly unit or building,
// the selected units guard it; on an enemy, attack it; on the ground,
// attack-move there. Shift queues the order. A wall isn't something to
// guard or to attack this way: clicking one attack-moves to it like ground
// (walls are attacked only by a plain or Ctrl+right-click on them).

void SimUIForm::OnGuardClick(int x, int y, bool fQueue)
{
    if (gpfrmPlace != NULL && gpfrmPlace->IsBusy())
        return;

    WCoord wxTarget, wyTarget;
    Gob *pgobHit = HitTestGob(x, y, &wxTarget, &wyTarget);
    gfQueueOrder = fQueue;
    if (pgobHit != NULL && pgobHit->GetType() != kgtReplicator && pgobHit->GetType() != kgtWall &&
            (pgobHit->GetFlags() & (kfGobUnit | kfGobActive)) == (kfGobUnit | kfGobActive)) {
        UnitGob *puntHit = (UnitGob *)pgobHit;
        if (puntHit->IsAlly(gpplrLocal->GetSide())) {
            Gid *pgid = new Gid[kcpgobMax];
            int cgid = GetOwnSelection(pgid, kcpgobMax, true);
            MobileUnitGob *pmuntAck = NULL;
            for (int i = 0; i < cgid; i++) {
                MobileUnitGob *pmunt = (MobileUnitGob *)ggobm.GetGob(pgid[i]);
                if (pmunt != NULL && pmunt != pgobHit) {
                    pmunt->GuardUnit(pgobHit->GetId());
                    if (pmuntAck == NULL)
                        pmuntAck = pmunt;
                }
            }
            delete[] pgid;
            pgobHit->Flash();

            // The units' reply, as for a move

            if (pmuntAck != NULL)
                gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmuntAck->GetUnitType()])->sfxcMove));
        } else {
            MoveOrAttackOrSelect(pgobHit, wxTarget, wyTarget, kfMasAttack);
        }
    } else {
        gfAttackMoveOrder = true;
        MoveOrAttackOrSelect(NULL, wxTarget, wyTarget, kfMasMove);
        gfAttackMoveOrder = false;
    }
    gfQueueOrder = false;
}

void SimUIForm::OnRightClick(int x, int y, bool fQueue, bool fForce)
{
    if (gpfrmPlace != NULL && gpfrmPlace->IsBusy())
        return;

    WCoord wxTarget, wyTarget;
    Gob *pgobHit = HitTestGob(x, y, &wxTarget, &wyTarget);

    // Ctrl+right-click on a unit or building: force attack, even a friend.
    // Ctrl+Shift queues it.

    if (fForce && pgobHit != NULL && pgobHit->GetType() != kgtReplicator &&
            (pgobHit->GetFlags() & (kfGobUnit | kfGobActive)) == (kfGobUnit | kfGobActive)) {
        gfForceAttackOrder = true;
        gfQueueOrder = fQueue;
        MobileUnitGob *pmunt = SetSelectionTargets(pgobHit->GetId(), wxTarget, wyTarget);
        gfQueueOrder = false;
        gfForceAttackOrder = false;
        pgobHit->Flash();
        if (pmunt != NULL)
            gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmunt->GetUnitType()])->sfxcAttack));
        return;
    }

    if (pgobHit != NULL && pgobHit->GetType() != kgtReplicator &&
            (pgobHit->GetFlags() & kfGobUnit) != 0 &&
            (gfGodMode || pgobHit->GetSide() == gpplrLocal->GetSide())) {
        // Right-click on a selected Dominion: pick where its Headquarters
        // goes (the placement follows the mouse, left-click deploys there)

        if (!fQueue && pgobHit->GetType() == kgtMobileHeadquarters &&
                (pgobHit->GetFlags() & kfGobSelected) &&
                (pgobHit->GetFlags() & kfGobActive)) {
            gpfrmPlace->SetDeployer((MobileHqGob *)pgobHit);
            gpfrmmSim->AddForm(gpfrmPlace);
            return;
        }

        // Right-click on one of your Processors: the selected miners unload
        // there; the other selected units move there

        if (pgobHit->GetType() == kgtProcessor &&
                (pgobHit->GetFlags() & kfGobActive) &&
                !(pgobHit->GetFlags() & kfGobBeingBuilt) &&
                DeliverSelectedMiners(pgobHit, fQueue)) {
            gfQueueOrder = fQueue;
            s_fSkipMiners = true;
            SetSelectionTargets(kgidNull, wxTarget, wyTarget);
            s_fSkipMiners = false;
            gfQueueOrder = false;
            return;
        }

        // Otherwise right-click on one of your own units or buildings is a
        // move there (hovering shows its name; left-click opens a building's
        // menu)

        pgobHit = NULL;
    }

    // Shift+right-click queues the order after the ones being carried out

    gfQueueOrder = fQueue;
    MoveOrAttackOrSelect(pgobHit, wxTarget, wyTarget, kfMasMove | kfMasAttack);
    gfQueueOrder = false;
}

bool SimUIForm::OnPenEvent(Event *pevt)
{
    // Give the form controls first crack at handling the event.  Form
    // controls include: soft menu button, mode cancel button, status
    // label)

    if (Form::OnPenEvent(pevt)) {
        return true;
    }

    if (pevt->eType == penHoverEvent) {
        // The mouse resting on the battlefield: name what's under it

        SetHover(true, pevt->x, pevt->y);
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

// The selected miners unload at this Processor. Returns false if none are
// selected.

bool SimUIForm::DeliverSelectedMiners(Gob *pgobProcessor, bool fQueue)
{
    bool fAny = false;
    MinerGob *pmnrFirst = NULL;
    gfQueueOrder = fQueue;
    for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL;
            pgobT = ggobm.GetNextGob(pgobT)) {
        if ((pgobT->GetFlags() & (kfGobSelected | kfGobActive)) !=
                (kfGobSelected | kfGobActive))
            continue;
        if (pgobT->GetType() != kgtGalaxMiner)
            continue;
        if (!gfGodMode && pgobT->GetOwner() != pgobProcessor->GetOwner())
            continue;
        ((MinerGob *)pgobT)->DeliverTo(pgobProcessor->GetId());
        if (pmnrFirst == NULL)
            pmnrFirst = (MinerGob *)pgobT;
        fAny = true;
    }
    gfQueueOrder = false;
    if (!fAny)
        return false;

    pgobProcessor->Flash();
    WPoint wpt;
    pmnrFirst->GetCenter(&wpt);
    gsndm.PlaySfxAt(ksfxGalaxMinerDeliver, wpt.wx, wpt.wy);
    return true;
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

			if ((ff & kfGobMobileUnit) && s_fSkipMiners &&
					pgobT->GetType() == kgtGalaxMiner) {
				continue;
			}
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

    // Nothing under unexplored fog can be pointed at (hovered, selected or
    // targeted): neither where the pointer is, nor a unit standing in fog
    // whose picture reaches out of it (a building partly out of the fog can
    // be pointed at where it shows)

    byte *pbFog = gsim.GetLevel()->GetFogMap()->GetMapPtr();
    if (IsFogOpaque(pbFog[TcFromWc(wyTarget) * ctx + TcFromWc(wxTarget)]))
        return NULL;

    Gob *pgobHit = NULL;
    Enum enm;
    while (gsim.HitTest(&enm, wxTarget, wyTarget, kfGobActive | kfGobUnit, &pgobHit)) {
        WPoint wpt;
        pgobHit->GetCenter(&wpt);
        TCoord tx = TcFromWc(wpt.wx);
        TCoord ty = TcFromWc(wpt.wy);
        if ((pgobHit->GetFlags() & kfGobMobileUnit) &&
                tx >= 0 && tx < ctx && ty >= 0 && ty < cty &&
                IsFogOpaque(pbFog[ty * ctx + tx])) {
            continue;
        }
        return pgobHit;
    }
    return NULL;
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

const int kctDoubleTapDelay = 50; // half a second, like a mouse double-click

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
				gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmunt->GetUnitType()])->sfxcMove));
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
			gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[pmunt->GetUnitType()])->sfxcMove));
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

			PlaySelectSfx(pgobHit);
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
			gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[((UnitGob *)pmunt)->GetUnitType()])->sfxcAttack));
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

// Hover label: the name of the unit or building the mouse rests on. Shown
// for anything visible (own, allied or enemy), not while dragging or when a
// menu or dialog is up.

void SimUIForm::SetHover(bool fHover, int x, int y)
{
    m_fHover = fHover;
    m_xHover = x;
    m_yHover = y;
}

void SimUIForm::CalcHoverLabel(Rect *prc, const char **ppsz)
{
    prc->SetEmpty();
    *ppsz = NULL;
    if (!m_fHover || !s_fReadyForPaint || gfDragSelecting)
        return;
    if (gpfrmPlace != NULL && gpfrmPlace->IsBusy())
        return;

    WCoord wxT, wyT;
    Gob *pgob = HitTestGob(m_xHover, m_yHover, &wxT, &wyT);
    if (pgob == NULL || (pgob->GetFlags() & kfGobUnit) == 0)
        return;
    UnitConsts *puntc = ((UnitGob *)pgob)->GetConsts();
    if (puntc == NULL || puntc->szName == NULL || puntc->szName[0] == 0)
        return;

    Font *pfnt = gapfnt[kifntShadow];
    int cx = pfnt->GetTextExtent(puntc->szName) + 6;
    int cy = pfnt->GetHeight() + 4;

    // Below and right of the pointer, kept on the battlefield

    int x = m_xHover + 10;
    int y = m_yHover + 14;
    if (x + cx > m_rc.Width())
        x = m_xHover - cx - 2;
    if (y + cy > m_rc.Height())
        y = m_yHover - cy - 2;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // Keep it off the minimap: above the pointer, or failing that to its
    // left

    Form *pfrmMiniMap = gpmfrmm->GetFormPtr(kidfMiniMap);
    if (pfrmMiniMap != NULL) {
        Rect rcMM, rcT;
        pfrmMiniMap->GetRect(&rcMM);
        rcT.Set(x, y, x + cx, y + cy);
        if (rcT.Intersect(&rcT, &rcMM)) {
            y = m_yHover - cy - 4;
            if (y < 0)
                y = 0;
            rcT.Set(x, y, x + cx, y + cy);
            if (rcT.Intersect(&rcT, &rcMM)) {
                x = m_xHover - cx - 4;
                if (x < 0)
                    x = 0;
            }
        }
    }
    prc->Set(x, y, x + cx, y + cy);
    *ppsz = puntc->szName;
}

void SimUIForm::UpdateHoverLabel(UpdateMap *pupd)
{
    Rect rcNew;
    const char *psz;
    CalcHoverLabel(&rcNew, &psz);
    if (rcNew.left == m_rcHover.left && rcNew.top == m_rcHover.top &&
            rcNew.right == m_rcHover.right &&
            rcNew.bottom == m_rcHover.bottom && psz == m_pszHover) {
        return;
    }
    if (!m_rcHover.IsEmpty())
        pupd->InvalidateRect(&m_rcHover);
    if (!rcNew.IsEmpty())
        pupd->InvalidateRect(&rcNew);
    m_rcHover = rcNew;
    m_pszHover = psz;
}

void SimUIForm::DrawHoverLabel(DibBitmap *pbm)
{
    if (m_rcHover.IsEmpty() || m_pszHover == NULL)
        return;
    Rect rc = m_rcHover;
    pbm->Shadow(rc.left, rc.top, rc.Width(), rc.Height());
    pbm->Shadow(rc.left, rc.top, rc.Width(), rc.Height());
    DrawBorder(pbm, &rc, 1, GetColor(kiclrWhite));
    gapfnt[kifntShadow]->DrawText(pbm, (char *)m_pszHover, rc.left + 3,
            rc.top + 2);
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

// Only the player's own units and buildings answer when selected; walls
// make no sound

void SimUIForm::PlaySelectSfx(Gob *pgob)
{
    if (gfGodMode)
        return;
    if (pgob->GetSide() != gpplrLocal->GetSide())
        return;
    if (pgob->GetType() == kgtWall)
        return;
    if (pgob->GetFlags() & kfGobStructure) {
        gsndm.PlaySfx(((StructConsts *)gapuntc[((StructGob *)pgob)->GetUnitType()])->sfxSelect);
    } else if (pgob->GetSide() == gpplrLocal->GetSide()) {
        gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)gapuntc[((UnitGob *)pgob)->GetUnitType()])->sfxcSelect));
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
        gsndm.PlayResponseSfx(SfxFromCategory(((MobileUnitConsts *)
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
	m_axSrc = NULL;
	m_aySrc = NULL;
	m_nPpt = 1 << 16;
	m_cxDisp = 0;
	m_cyDisp = 0;
	m_pbFogLevel = NULL;
	m_pdwColor = NULL;
	m_pbNearest = NULL;
	m_ppal = NULL;
}

MiniMapControl::~MiniMapControl()
{
	gpmm = NULL;
	delete m_pbm;
	delete[] m_axSrc;
	delete[] m_aySrc;
	delete[] m_pbFogLevel;
	delete[] m_pdwColor;
	delete[] m_pbNearest;
}

// Mini map layout. The terrain is kept in a dib at the mini map's display
// size, each pixel the average colour of the part of the map it covers
// (from the full size tiles). The fog is kept per display pixel too,
// interpolated between tiles so its edges are soft. Units are drawn on top
// at their current (interpolated) positions every paint so they move
// smoothly.

#define kpctMiniMapTarget 34	// mini map height as % of the playfield height
#define kcxyMiniMapMin 96
#define knPptMax (4 << 16)		// at most 4 pixels per tile (16.16)

void MiniMapControl::CalcLayout(int *pnBase, int *pcxDisp, int *pcyDisp, int *pnPpt)
{
	Size siz;
	ggame.GetPlayfieldSize(&siz);
	Size sizMap;
	gsim.GetLevel()->GetTileMap()->GetMapSize(&sizMap);
	int ctx = sizMap.cx / gcxTile;
	int cty = sizMap.cy / gcyTile;
	int ctMax = ctx > cty ? ctx : cty;
	if (ctMax < 1)
		ctMax = 1;

	int cxyTarget = siz.cy * kpctMiniMapTarget / 100;
	if (cxyTarget > siz.cx / 3)
		cxyTarget = siz.cx / 3;
	if (cxyTarget < kcxyMiniMapMin)
		cxyTarget = kcxyMiniMapMin;
	int nPpt = (int)(((long long)cxyTarget << 16) / ctMax);
	if (nPpt > knPptMax)
		nPpt = knPptMax;

	*pnPpt = nPpt;
	*pnBase = nPpt >= (2 << 16) ? 2 : 1;
	*pcxDisp = (int)(((long long)ctx * nPpt) >> 16);
	*pcyDisp = (int)(((long long)cty * nPpt) >> 16);
}

int MiniMapControl::CalcWidth()
{
	int nBase, cxDisp, cyDisp, nPpt;
	CalcLayout(&nBase, &cxDisp, &cyDisp, &nPpt);
	return cxDisp + 1;
}

bool MiniMapControl::Init(Form *pfrm, IniReader *pini, FindProp *pfind)
{
	// Base initialization

	if (!Control::Init(pfrm, pini, pfind))
		return false;

	// Remember the size of the input UI area since our border drawing needs it

	Size sizDib;
	ggame.GetInputUIForm()->GetFormMgr()->GetDib()->GetSize(&sizDib);
	m_cyInputUI = sizDib.cy;

	m_cxyBorder = 1;

	// Layout

	CalcLayout(&m_nScale, &m_cxDisp, &m_cyDisp, &m_nPpt);
	gsim.SetMiniMapScale(m_nScale);
	m_rc.left = m_rc.right - (m_cxDisp + m_cxyBorder);
	m_rc.bottom = m_rc.top + m_cyDisp + m_cxyBorder;
	m_xOff = 0;
	m_yOff = 0;

	// Terrain dib at the base scale

	ggobm.GetMapSize(&m_ctx, &m_cty);
	m_pbm = CreateDibBitmap(NULL, m_cxDisp, m_cyDisp);
	if (m_pbm == NULL)
		return false;
	m_pbFogLevel = new byte[m_cxDisp * m_cyDisp];
	memset(m_pbFogLevel, 255, m_cxDisp * m_cyDisp);
	m_pdwColor = new dword[m_cxDisp * m_cyDisp];
	memset(m_pdwColor, 0, m_cxDisp * m_cyDisp * sizeof(dword));
	m_pbNearest = new byte[32 * 32 * 32];
	memset(m_pbNearest, 0, 32 * 32 * 32);
	m_ppal = gsim.GetLevel()->GetPalette();

	// Stretch tables: display pixel -> terrain dib pixel

	delete[] m_axSrc;
	delete[] m_aySrc;
	m_axSrc = new int[m_cxDisp];
	m_aySrc = new int[m_cyDisp];
	for (int x = 0; x < m_cxDisp; x++)
		m_axSrc[x] = (int)(((long long)x * m_ctx * m_nScale) / m_cxDisp);
	for (int y = 0; y < m_cyDisp; y++)
		m_aySrc[y] = (int)(((long long)y * m_cty * m_nScale) / m_cyDisp);

	// Stuff constants

	TileMap *ptmap = gsim.GetLevel()->GetTileMap();
	MiniTileSetHeader *pmtseth = ptmap->GetMiniTileSetHeader(m_nScale);
	m_pbTileData = (byte *)(pmtseth + 1);
	m_pwTileMap = ptmap->m_pwMapData;
	m_pbFogMap = gsim.GetLevel()->GetFogMap()->GetMapPtr();
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

// Fills a rect clipped to the mini map's display area

static void FillClipped(DibBitmap *pbm, Rect *prcClip, int x, int y, int cx, int cy, byte clr)
{
	Rect rc;
	rc.Set(x, y, x + cx, y + cy);
	if (!rc.Intersect(&rc, prcClip))
		return;
	pbm->Fill(rc.left, rc.top, rc.Width(), rc.Height(), clr);
}

void MiniMapControl::OnPaint(DibBitmap *pbm)
{
	Rect rcForm;
	m_pfrm->GetRect(&rcForm);
	int x = m_rc.left + rcForm.left;
	int y = m_rc.top + rcForm.top;
	int xMap = x + m_cxyBorder;
	int yMap = y + m_cxyBorder;

	// HD pictures (hdsprites.cpp) of buildings under the minimap mustn't show
	// through it where its colours happen to match the ground's

	HdNoteCovered(pbm, x, y, m_rc.Width(), m_rc.Height());

	// Border: top, and left down to the input bar

	int iclr = kiclr0CyanSide;
	pbm->Fill(x, y, m_rc.Width(), m_cxyBorder, GetColor(iclr));
	pbm->Fill(x, y, m_cxyBorder, m_rc.Height() - m_cyInputUI + 1, GetColor(iclr));
	pbm->Fill(x, y + m_rc.Height() - m_cyInputUI + 1, m_cxyBorder, m_cyInputUI, GetColor(kiclrBlack));

	// Stretch the terrain to the display size

	Size sizDst;
	pbm->GetSize(&sizDst);
	Rect rcClip;
	rcClip.Set(xMap, yMap, xMap + m_cxDisp, yMap + m_cyDisp);
	Rect rcDst;
	rcDst.Set(0, 0, sizDst.cx, sizDst.cy);
	if (!rcClip.Intersect(&rcClip, &rcDst))
		return;

	byte *pbSrcBase = m_pbm->GetBits();
	byte *pbDstBase = pbm->GetBits();
	int cbRowDst = pbm->GetRowBytes();
	byte *mpShadow = gmpiclriclrShadow;
	bool fTrueColor = pbm->GetTrueColor() != NULL;
	static const byte s_abBayer4[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
	for (int yD = rcClip.top; yD < rcClip.bottom; yD++) {
		int yS = yD - yMap;
		byte *pbSrc = pbSrcBase + (long)yS * m_cbRowBytes + (rcClip.left - xMap);
		byte *pbFog = m_pbFogLevel + (long)yS * m_cxDisp + (rcClip.left - xMap);
		dword *pdwClr = m_pdwColor + (long)yS * m_cxDisp + (rcClip.left - xMap);
		byte *pbDst = pbDstBase + (long)yD * cbRowDst + rcClip.left;
		for (int xD = rcClip.left; xD < rcClip.right; xD++) {
			int nCov = *pbFog++;
			byte b = *pbSrc++;
			dword clr = *pdwClr++;
			if (nCov >= 255) {
				*pbDst++ = m_clrBlack;
				continue;
			}
			if (fTrueColor) {
				// True colour: the averaged terrain, darkened smoothly

				*pbDst++ = b;
				int n = 255 - nCov;
				dword rb = ((clr & 0xff00ff) * n >> 8) & 0xff00ff;
				dword g = ((clr & 0xff00) * n >> 8) & 0xff00;
				pbm->SetTrueColor(xD, yD, 0xff000000 | rb | g);
				continue;
			}

			// 8 bits: shadow steps, dithered

			int nLevel = (nCov * 8 * 16 + s_abBayer4[(yD & 3) * 4 + (xD & 3)] * 255) / (255 * 16);
			if (nLevel >= 8) {
				b = m_clrBlack;
			} else if (mpShadow != NULL) {
				while (nLevel-- > 0)
					b = mpShadow[b];
			}
			*pbDst++ = b;
		}
	}

	// Units and buildings, where they are being drawn right now

	int nAlpha = 256;
	if (gsim.IsSmoothMotionActive())
		nAlpha = gsim.GetInterpolationAlpha();
	int cxyUnit = m_nPpt >> 16;
	if (cxyUnit < 2)
		cxyUnit = 2;
	for (Gob *pgob = ggobm.GetFirstGob(); pgob != NULL; pgob = ggobm.GetNextGob(pgob)) {
		dword ff = pgob->GetFlags();
		if (!(ff & kfGobUnit))
			continue;
		if ((ff & (kfGobMobileUnit | kfGobActive)) == kfGobMobileUnit)
			continue;
		WCoord wx, wy;
		pgob->GetDrawPosition(nAlpha, &wx, &wy);
		TCoord tx = TcFromWc(wx);
		TCoord ty = TcFromWc(wy);
		if (tx < 0 || ty < 0 || tx >= m_ctx || ty >= m_cty)
			continue;
		if (IsFogOpaque(m_pbFogMap[ty * m_ctx + tx]))
			continue;
		UnitGob *punt = (UnitGob *)pgob;
		byte clr = (ff & kfGobSelected) ? m_clrWhite : m_aclrSide[punt->GetSide()];
		int xU = xMap + (int)(((long long)wx * m_nPpt) >> 24);
		int yU = yMap + (int)(((long long)wy * m_nPpt) >> 24);
		if (ff & kfGobStructure) {
			StructConsts *pstruc = (StructConsts *)punt->GetConsts();
			int cx = (int)(((long long)pstruc->ctx * m_nPpt) >> 16);
			int cy = (int)(((long long)pstruc->cty * m_nPpt) >> 16);
			FillClipped(pbm, &rcClip, xU, yU, cx < 2 ? 2 : cx, cy < 2 ? 2 : cy, clr);
		} else {
			// Mobile units are positioned by their center
			FillClipped(pbm, &rcClip, xU - cxyUnit / 2, yU - cxyUnit / 2, cxyUnit, cxyUnit, clr);
		}
	}

	// Draw box showing the visible area of the map

	WCoord wxView, wyView;
	gsim.GetViewPos(&wxView, &wyView);
	Size siz;
	ggame.GetPlayfieldSize(&siz);
	Rect rc;
	rc.left = xMap + (int)(((long long)wxView * m_nPpt) >> 24);
	rc.top = yMap + (int)(((long long)wyView * m_nPpt) >> 24);
	rc.right = rc.left + (int)(((long long)WcFromPc(siz.cx < kpcMax ? siz.cx : kpcMax) * m_nPpt) >> 24);
	rc.bottom = rc.top + (int)(((long long)WcFromPc(siz.cy < kpcMax ? siz.cy : kpcMax) * m_nPpt) >> 24);
	if (rc.right > rcClip.right)
		rc.right = rcClip.right;
	if (rc.bottom > rcClip.bottom)
		rc.bottom = rcClip.bottom;
	if (rc.left < rcClip.left)
		rc.left = rcClip.left;
	if (rc.top < rcClip.top)
		rc.top = rcClip.top;
	if (rc.Width() > 1 && rc.Height() > 1)
		DrawBorder(pbm, &rc, 1, GetColor(kiclrWhite));
}

//TUNE:
#define kctMiniMapInvalidateRate 25

void MiniMapControl::Update()
{
	// Units are drawn on the mini map as it paints, so repaint it every
	// update (with smooth motion it is also repainted every frame)

	m_wfMm &= ~kfMmRedraw;
	Invalidate();
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
	WCoord wcxView = WcFromPc(sizPlayfield.cx < kpcMax ? sizPlayfield.cx : kpcMax);
	WCoord wcyView = WcFromPc(sizPlayfield.cy < kpcMax ? sizPlayfield.cy : kpcMax);

	if (pevt->eType == penDownEvent) {
		// Pen down inside the view rect? Then drag it; otherwise center the
		// view on the point clicked

		WCoord wxPen = (WCoord)(((long long)xPen << 24) / m_nPpt);
		WCoord wyPen = (WCoord)(((long long)yPen << 24) / m_nPpt);
		if (!(wxPen >= wxView && wxPen < wxView + wcxView && wyPen >= wyView && wyPen < wyView + wcyView)) {
			wxView = wxPen - wcxView / 2;
			wyView = wyPen - wcyView / 2;
		}

		s_xPenDown = xPen;
		s_yPenDown = yPen;
		s_wxViewDown = wxView;
		s_wyViewDown = wyView;
	} else {
		wxView = s_wxViewDown + (WCoord)(((long long)(xPen - s_xPenDown) << 24) / m_nPpt);
		wyView = s_wyViewDown + (WCoord)(((long long)(yPen - s_yPenDown) << 24) / m_nPpt);
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

	// Redraw the terrain, Galaxite and fog of this rect of tiles. The fog
	// at a pixel depends on the tiles around it, so one more tile around.

	TRect trc = *ptrc;
	trc.left = trc.left > 0 ? trc.left - 1 : 0;
	trc.top = trc.top > 0 ? trc.top - 1 : 0;
	trc.right = trc.right < m_ctx ? trc.right + 1 : m_ctx;
	trc.bottom = trc.bottom < m_cty ? trc.bottom + 1 : m_cty;
	int xD0 = (int)((long long)trc.left * m_cxDisp / m_ctx);
	int yD0 = (int)((long long)trc.top * m_cyDisp / m_cty);
	int xD1 = (int)(((long long)trc.right * m_cxDisp + m_ctx - 1) / m_ctx);
	int yD1 = (int)(((long long)trc.bottom * m_cyDisp + m_cty - 1) / m_cty);
	if (xD1 > m_cxDisp)
		xD1 = m_cxDisp;
	if (yD1 > m_cyDisp)
		yD1 = m_cyDisp;

	TileMap *ptmap = gsim.GetLevel()->GetTileMap();
	int cxMap = m_ctx * gcxTile;
	int cyMap = m_cty * gcyTile;
	int cSamplesX = cxMap / m_cxDisp;
	int cSamplesY = cyMap / m_cyDisp;
	cSamplesX = cSamplesX < 1 ? 1 : (cSamplesX > 6 ? 6 : cSamplesX);
	cSamplesY = cSamplesY < 1 ? 1 : (cSamplesY > 6 ? 6 : cSamplesY);
	byte *argbGx = m_ppal->argb[m_clrGalaxite];

	for (int yD = yD0; yD < yD1; yD++) {
		for (int xD = xD0; xD < xD1; xD++) {
			// Terrain: average of samples over the area this pixel covers

			int r = 0, g = 0, b = 0, c = 0;
			for (int sy = 0; sy < cSamplesY; sy++) {
				int yM = (int)(((long long)(yD * cSamplesY + sy) * 2 + 1) * cyMap / (2 * m_cyDisp * cSamplesY));
				for (int sx = 0; sx < cSamplesX; sx++) {
					int xM = (int)(((long long)(xD * cSamplesX + sx) * 2 + 1) * cxMap / (2 * m_cxDisp * cSamplesX));
					int tx = xM / gcxTile;
					int ty = yM / gcyTile;
					long offset = (long)ty * m_ctx + tx;
					byte *argb;
					if (HasGalaxite(m_pbFogMap[offset])) {
						argb = argbGx;
					} else {
						byte *pbTile = ptmap->m_apbTileData[BigWord(m_pwTileMap[offset]) / 4];
						byte bPixel = pbTile[(yM - ty * gcyTile) * gcxTile + (xM - tx * gcxTile)];
						argb = m_ppal->argb[bPixel];
					}
					r += argb[0];
					g += argb[1];
					b += argb[2];
					c++;
				}
			}
			r /= c;
			g /= c;
			b /= c;
			m_pbm->GetBits()[(long)yD * m_cbRowBytes + xD] = NearestColor(r, g, b);
			m_pdwColor[(long)yD * m_cxDisp + xD] = ((dword)r << 16) | ((dword)g << 8) | (dword)b;

			// Fog: coverage interpolated between tile centres

			int nFx = (int)(((long long)(2 * xD + 1) * m_ctx << 8) / (2 * m_cxDisp)) - 128;
			int nFy = (int)(((long long)(2 * yD + 1) * m_cty << 8) / (2 * m_cyDisp)) - 128;
			int tx0 = nFx >> 8, ty0 = nFy >> 8;
			int fx = nFx & 0xff, fy = nFy & 0xff;
			int c00 = FogCoverage(tx0, ty0), c10 = FogCoverage(tx0 + 1, ty0);
			int c01 = FogCoverage(tx0, ty0 + 1), c11 = FogCoverage(tx0 + 1, ty0 + 1);
			int cTop = c00 + (((c10 - c00) * fx) >> 8);
			int cBot = c01 + (((c11 - c01) * fx) >> 8);
			int nCov = cTop + (((cBot - cTop) * fy) >> 8);

			// Mostly dark over unexplored tiles, so their terrain doesn't show

			nCov = nCov * 3 / 2;
			if (nCov > 256)
				nCov = 256;
			nCov = (nCov * nCov * (3 * 256 - 2 * nCov)) / (256 * 256);
			if (nCov <= 2)
				nCov = 0;
			if (nCov >= 254)
				nCov = 255;
			m_pbFogLevel[(long)yD * m_cxDisp + xD] = (byte)(nCov > 255 ? 255 : nCov);
		}
	}
}

// Fog coverage of a tile, 0 (revealed) .. 256 (unexplored)

int MiniMapControl::FogCoverage(int tx, int ty)
{
	// Off the map: mirrored across the edge tile, as in the main view

	if (tx < 0)
		tx = -tx;
	if (tx >= m_ctx)
		tx = 2 * (m_ctx - 1) - tx;
	if (ty < 0)
		ty = -ty;
	if (ty >= m_cty)
		ty = 2 * (m_cty - 1) - ty;
	tx = tx < 0 ? 0 : (tx >= m_ctx ? m_ctx - 1 : tx);
	ty = ty < 0 ? 0 : (ty >= m_cty ? m_cty - 1 : ty);
	int nBits = m_pbFogMap[ty * m_ctx + tx] & kbfFogMask;
	int cRevealed = (nBits & 1) + ((nBits >> 1) & 1) + ((nBits >> 2) & 1) + ((nBits >> 3) & 1);
	return (4 - cRevealed) * 64;
}

// Nearest palette colour, cached by 15 bit colour

byte MiniMapControl::NearestColor(int r, int g, int b)
{
	int key = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
	if (m_pbNearest[key] != 0)
		return m_pbNearest[key] - 1;
	int cEntries = BigWord(m_ppal->cEntries);
	if (cEntries > 255)
		cEntries = 255;
	int iBest = 0;
	int nBest = 0x7fffffff;
	for (int i = 0; i < cEntries; i++) {
		int dr = m_ppal->argb[i][0] - r;
		int dg = m_ppal->argb[i][1] - g;
		int db = m_ppal->argb[i][2] - b;
		int n = dr * dr * 3 + dg * dg * 4 + db * db * 2;
		if (n < nBest) {
			nBest = n;
			iBest = i;
		}
	}
	m_pbNearest[key] = (byte)(iBest + 1);
	return (byte)iBest;
}

} // namespace wi
