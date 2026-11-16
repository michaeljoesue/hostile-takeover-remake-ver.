#include "ht.h"

namespace wi {

class StructureBuildForm : public Form
{
public:
	BuilderGob *GetOwner() {
		return m_pgobOwner;
	}
	void SetOwner(BuilderGob *pgobOwner);
	void UpdateStructureInfo(ListItem *pli);

	// Form overrides

	void EndForm(int nResult = kidcCancel);
	virtual void OnControlSelected(word idc);
	virtual void OnControlNotify(word idc, int nNotify);
	virtual void OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd);
	virtual bool OnPenEvent(Event *pevt);
	virtual bool OnRightDown(Event *pevt);

private:
	BuilderGob *m_pgobOwner;
	bool m_fLimitReached;
	int m_iliHover;
};


//
// HqGob implementation
//

BuilderConsts gbldcHq;
StructureBuildForm *HqGob::s_pfrmBuild = NULL;
PlaceStructureForm *gpfrmPlace = NULL;
static TBitmap *s_ptbmPlacementGood = NULL;
static TBitmap *s_ptbmPlacementBad = NULL;

//
// Gob methods
//

bool HqGob::InitClass(IniReader *pini)
{
	gbldcHq.gt = kgtHeadquarters;
	gbldcHq.ut = kutHeadquarters;
	gbldcHq.umCanBuild = kumStructures & ~(kumHeadquarters | kumReplicator);
	gbldcHq.wf |= kfUntcStructureBuilder;

	// Preload the HQ's build form

	s_pfrmBuild = new StructureBuildForm();
	if (s_pfrmBuild == NULL) {
		Assert("fatal error");
		return false;
	}

	if (!s_pfrmBuild->Init(gpmfrmm, gpiniForms, kidfBuildStructure)) {
		Assert("fatal error");
		return false;
	}
	gpmfrmm->RemoveForm(s_pfrmBuild);

	// Preload the HQ's place form

	gpfrmPlace = new PlaceStructureForm();
	if (gpfrmPlace == NULL) {
		Assert("fatal error");
		return false;
	}

	if (!gpfrmPlace->Init(gpfrmmSim, gpiniForms, kidfPlaceStructure)) {
		Assert("fatal error");
		return false;
	}
	gpfrmmSim->RemoveForm(gpfrmPlace);

	// Preload the placement tile bitmaps

	s_ptbmPlacementGood = LoadTBitmap("placementGood.tbm");
	s_ptbmPlacementBad = LoadTBitmap("placementBad.tbm");

	// Sound effects

	gbldcHq.sfxUnitBuildAbort = ksfxHeadquartersAbortConstruction;
	gbldcHq.sfxUnitBuild = ksfxHeadquartersConstruct;
	gbldcHq.sfxUnitReady = ksfxHeadquartersStructureReady;
	gbldcHq.sfxAbortRepair = ksfxHeadquartersAbortRepair;
	gbldcHq.sfxRepair = ksfxHeadquartersRepair;
	gbldcHq.sfxDamaged = ksfxHeadquartersDamaged;
	gbldcHq.sfxDestroyed =  ksfxHeadquartersDestroyed;
	gbldcHq.sfxSelect = ksfxHeadquartersSelect;

	return BuilderGob::InitClass(&gbldcHq, pini);
}

void HqGob::ExitClass()
{
	delete s_ptbmPlacementGood;
	s_ptbmPlacementGood = NULL;
	delete s_ptbmPlacementBad;
	s_ptbmPlacementBad = NULL;
	delete gpfrmPlace;
	gpfrmPlace = NULL;
	delete s_pfrmBuild;
	s_pfrmBuild = NULL;

	BuilderGob::ExitClass(&gbldcHq);
}

HqGob::HqGob() : BuilderGob(&gbldcHq)
{
	m_bq.SetSize(1);	// no queuing
}

HqGob::~HqGob()
{
	// if the structure placement form is up, pull it down

	if (gpfrmPlace->GetOwner() != NULL)
		gpfrmPlace->OnControlSelected(kidcCancel);
}

void HqGob::InitMenu(Form *pfrm)
{
	// toggle on/off the right stuff

	Control *pctl = pfrm->GetControlPtr(kidcBuild);
	pctl->Show(!m_bq.IsFull());

	pctl = pfrm->GetControlPtr(kidcAbortBuild);
	pctl->Show(!m_bq.IsEmpty());

	BuilderGob::InitMenu(pfrm);
}

void HqGob::OnMenuItemSelected(int idc)
{
	switch (idc) {
	case kidcBuild: 
		{
			gpmfrmm->AddForm(s_pfrmBuild);
			s_pfrmBuild->SetOwner(this);
			int idc;
			s_pfrmBuild->DoModal(&idc);
			gpmfrmm->RemoveForm(s_pfrmBuild);

			if (idc == kidcCancel)
				break;

			gpfrmPlace->SetOwner(this, GetUnitConsts(idc)->ut);
			m_unvl.MinSkip();
			gpfrmmSim->AddForm(gpfrmPlace);
		}
		break;

	case kidcAbortBuild:
		if (PopupConfirmation("ABORT BUILD")) {
			gcmdq.Enqueue(kmidAbortBuildOtherCommand, m_gid);
			if (m_pplr == gpplrLocal)
				gsndm.PlaySfx(m_pbldrc->sfxUnitBuildAbort);
		}
		break;

	default:
		BuilderGob::OnMenuItemSelected(idc);
		break;
	}
}

// take down the build and place forms, HQ is no longer
// in working order

void HqGob::Deactivate()
{
	BuilderGob::Deactivate();

	if (gpfrmPlace->GetOwner() == this)
		gpfrmPlace->OnControlSelected(kidcCancel);
	if (s_pfrmBuild->GetOwner() == this)
		s_pfrmBuild->EndForm();
}

//
// StateMachine methods
//


void HqGob::RemoveScorch(StructGob *pstru)
{
	// Get surrounding tile rect

	TRect trc;
	pstru->GetTileRect(&trc);

	// Inflate by one tile

	trc.Inflate(1, 1);

	// Clip to  map

	if (trc.left < 0)
		trc.left = 0;
	if (trc.top < 0)
		trc.top = 0;
	TCoord ctx, cty;
	ggobm.GetMapSize(&ctx, &cty);
	if (trc.right > ctx)
		trc.right = ctx;
	if (trc.bottom > cty)
		trc.bottom = cty;

	// Remove scorch gobs that'll be underneath this structure

	for (int ty = trc.top; ty < trc.bottom; ty++) {
		for (int tx = trc.left; tx < trc.right; tx++) {
			Gid gid = ggobm.GetFirstGid(tx, ty);
			while (gid != kgidNull) {
				Gid gidNext = ggobm.GetNextGid(gid);
				Gob *pgob = ggobm.GetGob(gid, false);
				if (pgob != NULL && pgob->GetType() == kgtScorch) {
					ggobm.RemoveGob(pgob);
					delete pgob;
				}
				gid = gidNext;
			}
		}
	}
}

int HqGob::ProcessStateMachineMessage(State st, Message *pmsg)
{
BeginStateMachine
	OnMsg(kmidBuildOtherCommand)
		// Only if the queue is empty since unit only knows how to build one at a time

		if (m_bq.IsEmpty()) {
			// Counts will already have been checked in the order UI. However since the order is queued, the UI only
			// guesses if it is possible to build based on current state when the button is pressed. Here we make a
			// bedrock decision. This ensures the limits are enforced and that the same decision gets made on all
			// clients in a multiplayer game.

			if (ggobm.IsBelowLimit(knLimitStruct, m_pplr)) {
				// BUGBUG: due to network lag we can't assume the destination position 
				// hasn't become occupied between when this command was issued and when
				// it is received.
				StructGob *pstru = (StructGob *)CreateGob(gapuntc[pmsg->BuildOtherCommand.ut]->gt);
				if (pstru == NULL)
					return knHandled;
				pstru->Init(pmsg->BuildOtherCommand.wpt.wx, pmsg->BuildOtherCommand.wpt.wy, m_pplr, 0, kfGobBeingBuilt, NULL);
				pstru->SetHealth(0);

				// Clear out scorch marks that are here

				RemoveScorch(pstru);

				Build(pmsg->BuildOtherCommand.ut, pstru->GetId());
			}
		}

	OnMsg(kmidAbortBuildOtherCommand)
		AbortBuild(true);

	OnMsg(kmidSelfDestructCommand)
		
		// override and call abort build here so we can tell it to refund the value.
		// AbortBuild is also in BuilderGob::Deactivate but will do nothing if called
		// a 2nd time, and there it wouldnt refund if we left it

		AbortBuild(true);
		SelfDestruct();

	State(kstBuildOtherCompleting)
		OnEnter

			// it's now too late to decide to abort

			TakedownConfirmation();

			gsmm.SendMsg(kmidBuildComplete, m_gidBuildVisible);

			// Notify the BuildMgr that this Unit is complete

			gsim.GetBuildMgr()->OnBuilt(GetBuiltGob(), this);

			ClearBuiltGob();
			SetState(kstIdle);

		// These are here to keep the message from routing up to BuilderGob's message handler
		OnUpdate
		OnExit

            return knHandled;
        }
    } else {
        return (int)BuilderGob::ProcessStateMachineMessage(st, pmsg);
    }
    return (int)BuilderGob::ProcessStateMachineMessage(st, pmsg);
}

//===========================================================================
// StructureBuildForm implementation

void StructureBuildForm::SetOwner(BuilderGob *pgobOwner)
{
	Size sizDib;
	m_pfrmm->GetDib()->GetSize(&sizDib);
	Rect rc;
	rc.left = ((sizDib.cx - m_rc.Width()) / 2) & ~1;
	rc.top = 0; // (sizDib.cy - m_rc.Height()) / 2;
	rc.right = rc.left + m_rc.Width();
	rc.bottom = rc.top + m_rc.Height();
	SetRect(&rc);

	m_wf |= kfFrmAutoTakedown | kfFrmNoEcom;
	m_pgobOwner = pgobOwner;
	m_iliHover = -1;

	BuildListControl *plstc = (BuildListControl *)GetControlPtr(kidcList);
	plstc->Clear();

	// Initialize list with available structure types.
	// UNDONE: need to link list to dynamic build capability

	Player *pplr = pgobOwner->GetOwner();
	UnitMask umOwned = (pplr->GetUnitMask() & kumStructures) | pplr->GetUpgrades();
	BuilderConsts *pbldrc = (BuilderConsts *)pgobOwner->GetConsts();
	UnitMask umAllowed = pbldrc->umCanBuild & (gfGodMode ? kumAll : pplr->GetAllowedUnits());
	UpgradeMask upgmOwned = pplr->GetUpgradeMask();
	for (int i = 0; i < kutMax; i++) {
		UnitConsts *puntc = gapuntc[i];

		// Does this player have what it takes to build this structure/unit?

		bool fDisabled = true;
		if (gfGodMode || ((umOwned & puntc->umPrerequisites) == puntc->umPrerequisites && 
				(upgmOwned & puntc->upgmPrerequisites) == puntc->upgmPrerequisites))
			fDisabled = false;

		// Yep, is this one of the types this build form allows?

		if (puntc->um & umAllowed) {

			// Yep, add it to the list

			int nStripIcon = puntc->panid->GetStripIndex("icon");
			if (nStripIcon != -1) {
				plstc->Add(puntc->panid, nStripIcon, 0, (void *)(pword)puntc->ut, fDisabled);
			}
		}
	}
	
	// Select the first unit by default

	plstc->Select(m_pgobOwner->GetLastSelection());
	plstc->SetQueueInfo(NULL, NULL);

	// If we're at the structure limit, hide the order button, show the limit reached text
	// Otherwise hide the limit reached text

	m_fLimitReached = !ggobm.IsBelowLimit(knLimitStruct, m_pgobOwner->GetOwner());
	GetControlPtr(kidcLimitReached)->Show(m_fLimitReached);

	// No Build button: clicking a building builds it

	GetControlPtr(kidcOk)->Show(false);
}

void StructureBuildForm::UpdateStructureInfo(ListItem *pli)
{
	StructConsts *pstruc = (StructConsts *)gapuntc[(pword)pli->pvData];

	// Update Cost

	LabelControl *plbl = (LabelControl *)GetControlPtr(kidcCost);
	char szT[kcbStructUnitName];
	itoa(pstruc->GetCost(), szT, 10);
	plbl->SetText(szT);
	PipMeterControl *pmtr = (PipMeterControl *)GetControlPtr(kidcCostMeter);
	pmtr->SetValue((pstruc->GetCost() * 100) / GetUnitCostMax());

	// Update Name

	plbl = (LabelControl *)GetControlPtr(kidcName);
	strcpy(szT, pstruc->szLongName);
	HtStrupr(szT);
	plbl->SetText(szT);

	// Update Power Demand

	plbl = (LabelControl *)GetControlPtr(kidcPowerDemand);
	itoa(pstruc->nPowerDemand, szT, 10);
	plbl->SetText(szT);
	pmtr = (PipMeterControl *)GetControlPtr(kidcPowerDemandMeter);

	// Use gnPowerSupplyMax as the scaler instead of gnPowerDemandMax so
	// both supply and demand can be compared on the same scale

	pmtr->SetValue((pstruc->nPowerDemand * 100) / gnPowerSupplyMax);

	// Update Power Supply

	plbl = (LabelControl *)GetControlPtr(kidcPowerSupply);
	itoa(pstruc->nPowerSupply, szT, 10);
	plbl->SetText(szT);
	pmtr = (PipMeterControl *)GetControlPtr(kidcPowerSupplyMeter);
	pmtr->SetValue((pstruc->nPowerSupply * 100) / gnPowerSupplyMax);

	// Update Firepower

	DamageMeterControl *pdmtr = (DamageMeterControl *)GetControlPtr(kidcWeaponStrengthMeter);
	pdmtr->SetUnitConsts(pstruc);

	// Update Armor

	char *pszT;
	fix fxOneThirdRange = (fix)divfx(subfx(gfxStructureArmorStrengthMax, gfxStructureArmorStrengthMin), itofx(3));
	plbl = (LabelControl *)GetControlPtr(kidcArmorStrength);
	pszT = "MEDIUM";
	if (pstruc->GetArmorStrength() == 0)
		pszT = "NONE";
	else if (pstruc->GetArmorStrength() < addfx(gfxStructureArmorStrengthMin, fxOneThirdRange))
		pszT = "LIGHT";
	else if (pstruc->GetArmorStrength() >= subfx(gfxStructureArmorStrengthMax, fxOneThirdRange))
		pszT = "HEAVY";
	plbl->SetText(pszT);
	pmtr = (PipMeterControl *)GetControlPtr(kidcArmorStrengthMeter);
	pmtr->SetValue(((int)fxtoi(pstruc->GetArmorStrength()) * 100) / fxtoi(gfxStructureArmorStrengthMax));

	// Update Description. Substitute the list of prerequisites if the
	// item is disabled.

	plbl = (LabelControl *)GetControlPtr(kidcDescription);
	if (pli->fDisabled) {
		char szT[120];
		GetPrerequisiteString(szT, pstruc);
		
		char szT2[120];
		sprintf(szT2, "This building requires: %s.", szT);
		plbl->SetText(szT2);

	} else {
		plbl->SetText(pstruc->szDescription);
	}

}

void StructureBuildForm::EndForm(int nResult)
{
	ListControl *plstc = (ListControl *)GetControlPtr(kidcList);
	m_pgobOwner->SetLastSelection(plstc->GetSelectedItemIndex());
	m_pgobOwner = NULL;
	Form::EndForm(nResult);
}

void StructureBuildForm::OnControlSelected(word idc)
{
	switch (idc) {
	case kidcCancel:
		EndForm(kidcCancel);
		return;

	case kidcHelp:
		Help("buildings", true);
		break;

	case kidcList:
		{
			ListControl *plstc = (ListControl *)GetControlPtr(kidcList);
			if (plstc->GetSelectedItem() == NULL)
				return;
			UpdateStructureInfo(plstc->GetSelectedItem());
		}
		break;

	case kidcOk:
		ListControl *plstc = (ListControl *)GetControlPtr(kidcList);
		if (plstc->GetSelectedItem() != NULL)
			EndForm(gapuntc[UnitTypeFromPVoid(plstc->GetSelectedItemData())]->gt);
		return;
	}
}

void StructureBuildForm::OnControlNotify(word idc, int nNotify)
{
	if (idc != kidcList) {
		return;
    }

	if (nNotify != knNotifySelectionChange && nNotify != knNotifySelectionTap) {
        return;
    }

    ListControl *plstc = (ListControl *)GetControlPtr(kidcList);
    ListItem *pli = plstc->GetSelectedItem();
    if (pli == NULL) {
		GetControlPtr(kidcOk)->Show(false);
		return;
    }
    UpdateStructureInfo(pli);

    // A click on a building starts placing it

    if (plstc->IsUserClick()) {
        if (!pli->fDisabled && !m_fLimitReached) {
            OnControlSelected(kidcOk);
        } else {
            gsndm.PlaySfx(ksfxGuiFormHide);
        }
    }
}

// The selection box and the info follow the building under the mouse

bool StructureBuildForm::OnPenEvent(Event *pevt)
{
    if (pevt->eType == penHoverEvent) {
        ListControl *plstc = (ListControl *)GetControlPtr(kidcList);
        int ili = plstc->HitTestItem(pevt->x, pevt->y);
        if (ili != m_iliHover) {
            m_iliHover = ili;
            if (ili >= 0)
                plstc->HoverSelect(ili);
        }
    }
    return Form::OnPenEvent(pevt);
}

// Right-click anywhere closes the menu

bool StructureBuildForm::OnRightDown(Event *pevt)
{
    OnControlSelected(kidcCancel);
    return true;
}

void StructureBuildForm::OnPaintBackground(DibBitmap *pbm, UpdateMap *pupd)
{
	RawBitmap *prbm = LoadRawBitmap("buildformbkgd.rbm");
	BltHelper(pbm, prbm, pupd, m_rc.left, m_rc.top);
	delete prbm;
}

//===========================================================================
// PlaceStructureForm implementation
//

PlaceStructureForm::PlaceStructureForm()
{
	m_pgobOwner = NULL;
	m_fDragging = false;
	m_fHaveMouse = false;
	m_fValid = false;
	m_wf |= kfFrmTranslucent;
}

void PlaceStructureForm::OnControlSelected(word idc)
{
	gtimm.RemoveTimer(this);

	if (idc == kidcCancel) {
		gpfrmmSim->RemoveForm(this);
		m_pgobOwner = NULL;
		return;
	}

	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.smidSender = ksmidNull;
	msg.mid = kmidBuildOtherCommand;
	msg.BuildOtherCommand.ut = m_pstruc->ut;
	msg.BuildOtherCommand.wpt.wx = WcFromTc(m_tx);
	msg.BuildOtherCommand.wpt.wy = WcFromTc(m_ty);
	msg.smidReceiver = m_pgobOwner->GetId();
	gcmdq.Enqueue(&msg);

	if (m_pgobOwner->GetOwner() == gpplrLocal) 
		gsndm.PlaySfx(((BuilderConsts *)m_pgobOwner->GetConsts())->sfxUnitBuild);

	gpfrmmSim->RemoveForm(this);

	m_pgobOwner = NULL;
}

void PlaceStructureForm::SetOwner(BuilderGob *pgobOwner, UnitType ut)
{
	gtimm.AddTimer(this, kctMapScrollRate);

	// form is loaded as the size of the whole playfield
	// draw the structure placement indicator near the HQ
	m_pgobOwner = pgobOwner;
	m_pstruc = (StructConsts *)gapuntc[ut];
	WPoint wptT;
	m_pgobOwner->GetPosition(&wptT);

	WCoord wxView, wyView;
	gsim.GetViewPos(&wxView, &wyView);
	Size sizPlayfield;
	ggame.GetPlayfieldSize(&sizPlayfield);
	WCoord wxViewMax, wyViewMax;
	wxViewMax = WcFromPc(sizPlayfield.cx) + wxView;
	wyViewMax = WcFromPc(sizPlayfield.cy) + wyView;

	// be consistent about appearing below the HQ unless we will be hidden

	StructConsts *pstrucOwner = (StructConsts *)pgobOwner->GetConsts();
	WCoord wxTarget = wptT.wx;
	WCoord wyTarget = wptT.wy + WcFromTc(pstrucOwner->ctyReserve);

	// now make sure we're onscreen

	wxViewMax -= WcFromTc(m_pstruc->ctxReserve);
	if (wxTarget < wxView)
		wxTarget = wxView;
	else if (wxTarget >  wxViewMax)
		wxTarget = wxViewMax;
	m_tx = m_txStart = TcFromWc(wxTarget);

	if (wyTarget < wyView) {
		wyTarget = wyView;
	} else {

		// watch out for being behind the minimap as well on the screen

		Form *pfrmMiniMap = gpmfrmm->GetFormPtr(kidfMiniMap);
		Rect rc;
		pfrmMiniMap->GetRect(&rc);
		WCoord wyMiniMap = WcFromUpc(rc.top) + wyView;
		if (wyTarget > wyMiniMap - WcFromTc(m_pstruc->ctyReserve) ) {

			// below the top of the mini map - check the horizontal

			WCoord wxMiniMap = WcFromUpc(rc.left) + wxView;
			if (wxTarget + WcFromTc(m_pstruc->ctxReserve) > wxMiniMap) {

				// behind the mini map, go above

				wyTarget = wyMiniMap - WcFromTc(m_pstruc->ctyReserve);
			} else {

				// not behind minimap, just watch screen bottom

				wyViewMax -= WcFromTc(m_pstruc->ctyReserve);
				if (wyTarget > wyViewMax)
					wyTarget = wyViewMax;
			}
		} 
	}
	
	m_ty = m_tyStart = TcFromWc(wyTarget);

	m_wxPen = m_wxDragStart = 0;
	m_wyPen = m_wyDragStart = 0;

	// The form covers the whole playfield (forms.ini has a handheld sized
	// 320x320 rect, so clicks on the right of a PC screen missed it)

	Rect rcForm;
	rcForm.Set(0, 0, sizPlayfield.cx, sizPlayfield.cy);
	SetRect(&rcForm);

	// Modern placement: the building follows the mouse, a left click
	// places it, right click or Escape cancels. No Ok / Cancel buttons.

	GetControlPtr(kidcOk)->Show(false);
	GetControlPtr(kidcCancel)->Show(false);

	// use UpdatePlacementIndicator to get the buttons initialized to the correct location

	UpdatePlacementIndicator(wxView, wyView, wxView, wyView);

	int x, y;
	m_fHaveMouse = false;
	if (HostGetMousePosition(&x, &y) && x < sizPlayfield.cx && y < sizPlayfield.cy)
		FollowMouse(x, y);
}

// Centers the placement under the mouse (x, y: playfield pixels)

void PlaceStructureForm::FollowMouse(int x, int y)
{
	m_fHaveMouse = true;
	m_xMouse = x;
	m_yMouse = y;

	WCoord wxView, wyView;
	gsim.GetViewPos(&wxView, &wyView);
	TCoord tx = TcFromWc(WcFromPc(x) + wxView) - (m_pstruc->ctxReserve - 1) / 2;
	TCoord ty = TcFromWc(WcFromPc(y) + wyView) - (m_pstruc->ctyReserve - 1) / 2;

	// UpdatePlacementIndicator places at start + pen - drag start

	m_txStart = tx;
	m_tyStart = ty;
	m_wxPen = m_wxDragStart = 0;
	m_wyPen = m_wyDragStart = 0;
	UpdatePlacementIndicator(wxView, wyView, wxView, wyView);
	gevm.SetRedrawFlags(kfRedrawDirty);
}

#define kwcButtonInset 56
#define kwcButtonWidth (50 + kwcButtonInset)

void PlaceStructureForm::GetSubRects(WCoord wx, WCoord wy, Rect *prcInside,
        Rect *prcOutside)
{
    WRect wrcInside;
    wrcInside.left = WcFromTc(m_tx);
    wrcInside.top = WcFromTc(m_ty);
    wrcInside.right = WcFromTc(m_tx + m_pstruc->ctxReserve);
    wrcInside.bottom = WcFromTc(m_ty + m_pstruc->ctyReserve);
    WRect wrcOutside = wrcInside;

    if (wrcOutside.Width() < WcFromTc(3)) {
        wrcOutside.Inflate((WcFromTc(3) - wrcOutside.Width()) / 2, 0);
    }

    Size siz;
    gsim.GetLevel()->GetTileMap()->GetMapSize(&siz);
    if (wrcOutside.left < kwcButtonWidth) {
        wrcOutside.left = kwcButtonWidth;
    }
    if (wrcOutside.right > WcFromUpc(siz.cx) - kwcButtonWidth) {
        wrcOutside.right = WcFromUpc(siz.cx) - kwcButtonWidth;
    }

    wrcInside.Offset(-wx, -wy);
    prcInside->FromWorldRect(&wrcInside);
    wrcOutside.Offset(-wx, -wy);
    prcOutside->FromWorldRect(&wrcOutside);
}

// do the math to move the hash marks that denote where your building might go
// but keep it tile-aligned. We make sure the hash marks stay on the map and
// stay close to where the pen is and in penmove we make sure the pen location
// we remember is always on the screen so in theory the structure placement
// indicator will always be at at least partially on screen.

void PlaceStructureForm::UpdatePlacementIndicator(WCoord wxViewStart, WCoord wyViewStart, 
		WCoord wxView, WCoord wyView)
{
	// Invalidate our old location

    Rect rcInside, rcOutside;
    GetSubRects(wxViewStart, wyViewStart, &rcInside, &rcOutside);
	InvalidateRect(&rcOutside);

	// always use pen movement from start, incremental pen
	// movements get rounded down to zero when converted to
	// tiles

	m_tx = m_txStart + TcFromWc(m_wxPen) - TcFromWc(m_wxDragStart);
	m_ty = m_tyStart + TcFromWc(m_wyPen) - TcFromWc(m_wyDragStart);

	// don't let the structure outline go off map at all

	if (m_tx < 0)
		m_tx = 0;
	if (m_ty < 0)
		m_ty = 0;
	Size siz;
	gsim.GetLevel()->GetTileMap()->GetMapSize(&siz);
	siz.cx /= gcxTile;
	siz.cy /= gcyTile;
	if (m_tx + m_pstruc->ctxReserve >= siz.cx)
		m_tx = siz.cx - m_pstruc->ctxReserve;
	if (m_ty + m_pstruc->ctyReserve >= siz.cy)
		m_ty = siz.cy - m_pstruc->ctyReserve;

	// Invalidate our new location
    
    GetSubRects(wxView, wyView, &rcInside, &rcOutside);
	InvalidateRect(&rcOutside);

	// Draw 'x', 'check' buttons 
	// check gets shown/hidden in DrawPlacementTiles because other gob movement
	// affects whether or not it shows

	ButtonControl *pbtn = (ButtonControl *)GetControlPtr(kidcCancel);
	Rect rcCtl;
	pbtn->GetRect(&rcCtl);
	int y = (PcFromWc(WcFromTc(m_ty) - wyView) + (PcFromTc(m_pstruc->ctyReserve) - rcCtl.Height()) / 2);

	pbtn->SetPosition(rcOutside.right - PcFromWc(kwcButtonInset), y);
	pbtn = (ButtonControl *)GetControlPtr(kidcOk);
	pbtn->SetPosition(rcOutside.left - rcCtl.Width() +
            PcFromWc(kwcButtonInset), y);

	// See if it's valid

	bool fValid = true;
	bool fPlacementValid = ggobm.IsStructurePlacementValid(m_pstruc, m_tx, m_ty, m_pgobOwner->GetOwner());

	// Show / hide ok button?

	TerrainMap *ptmap = gsim.GetLevel()->GetTerrainMap();
	FogMap *pfogm = gsim.GetLevel()->GetFogMap();

	for (int ty = m_ty; ty < m_ty + m_pstruc->ctyReserve; ty++) {
		for (int tx = m_tx; tx < m_tx + m_pstruc->ctxReserve; tx++) {
			if (fPlacementValid && IsTileFree(tx, ty, kbfReserved | kbfStructure) && pfogm->GetGalaxite(tx, ty) == 0) {
				continue;
			} else {
				fValid = false;
				break;
			}
		}
	}


	// Modern placement: no Ok button; a left click places when valid

	m_fValid = fValid || gfGodMode;
	GetControlPtr(kidcOk)->Show(false);
	GetControlPtr(kidcCancel)->Show(false);
}

bool PlaceStructureForm::OnPenEvent(Event *pevt)
{
	switch (pevt->eType) {
	case penHoverEvent:
	case penMoveEvent:
		FollowMouse(pevt->x, pevt->y);
		return true;

	case penDownEvent:
		{
			// Left click places the building if it fits there

			FollowMouse(pevt->x, pevt->y);
			if (m_fValid) {
				OnControlSelected(kidcOk);
			} else {
				gsndm.PlaySfx(ksfxGuiFormHide);
			}
		}
		return true;
	}
	return true;
}

void PlaceStructureForm::UpdatePosition(Event *pevt)
{
    // If pen down in placement form, drag it without offsetting
    // If pen down outside placement form, center placement for under pen

    WCoord wxView, wyView;
    gsim.GetViewPos(&wxView, &wyView);
    m_wxPen = WcFromPc(pevt->x) + wxView;
    m_wyPen = WcFromPc(pevt->y) + wyView;
    m_wxDragStart = m_wxPen;
    m_wyDragStart = m_wyPen;

    TRect trcT;
    trcT.Set(m_tx, m_ty, m_tx + m_pstruc->ctxReserve,
            m_ty + m_pstruc->ctyReserve);
    if (!trcT.PtIn(TcFromWc(m_wxPen), TcFromWc(m_wyPen))) {
        // New position for the placement indicator. Center indicator underneath
        // pen

        m_txStart = TcFromWc(m_wxPen) - (m_pstruc->ctxReserve - 1) / 2;
        if (m_txStart < 0)
            m_txStart = 0;
        m_tyStart = TcFromWc(m_wyPen) - (m_pstruc->ctyReserve - 1) / 2;
        if (m_tyStart < 0)
            m_tyStart = 0;
    } else {
        // Start from current position

        m_txStart = m_tx;
        m_tyStart = m_ty;
    }
}


// UNDONE: we should probably create an update on the form that gets called
// after all the gobs have updated their state and use that for the placement
// indicator testing too.

// A real-time (not simulation time) timer is started when the structure 
// placement form is shown so we can scroll when the form is dragged 
// off-screen.

void PlaceStructureForm::OnTimer(long tCurrent)
{
	// The view may have scrolled (mouse at the screen edge, arrow keys)
	// under a still mouse; keep the placement under it. Something may also
	// have moved under the placement, which changes whether it fits.

	if (m_fHaveMouse) {
		FollowMouse(m_xMouse, m_yMouse);
	} else {
		WCoord wxView, wyView;
		gsim.GetViewPos(&wxView, &wyView);
		UpdatePlacementIndicator(wxView, wyView, wxView, wyView);
	}
	gevm.SetRedrawFlags(kfRedrawDirty | kfRedrawBeforeTimer);
}

void PlaceStructureForm::OnScroll(int dx, int dy)
{
	// we're scrolling - don't let the buttons get left behind
	
	ButtonControl *pbtn = (ButtonControl *)GetControlPtr(kidcCancel);
	Rect rcCtl;

	// let it invalidate, as long as the PlacementTiles draw on every paint the
	// controls will need to as well.

	pbtn->GetRect(&rcCtl);
	rcCtl.Offset(dx,dy);
	pbtn->SetRect(&rcCtl);

	pbtn = (ButtonControl *)GetControlPtr(kidcOk);
	pbtn->GetRect(&rcCtl);
	rcCtl.Offset(dx,dy);
	pbtn->SetRect(&rcCtl);
}

void PlaceStructureForm::OnPaintSimUI(DibBitmap *pbm)
{
	if (m_pgobOwner == NULL)
		return;

	WCoord wxViewOrigin, wyViewOrigin;
	gsim.GetViewPos(&wxViewOrigin, &wyViewOrigin);
    Rect rcInside, rcOutside;
    GetSubRects(wxViewOrigin, wyViewOrigin, &rcInside, &rcOutside);

    // If the outside is larger than the inside, draw leader lines from
    // the inside to the edge of the controls, if they are visible.
    // Ok is on the left.

    if (!rcOutside.Equal(&rcInside)){
        // Ok is on the left, Cancel is on the right
        Control *pctl = GetControlPtr(kidcOk);
        if (pctl->GetFlags() & kfCtlVisible) {
            Rect rcT;
            pctl->GetRect(&rcT);
            int x = m_rc.left + rcT.right;
            int y = m_rc.top + rcT.top + rcT.Height() / 2;
            pbm->Fill(x, y, rcInside.left - x, 1, GetColor(kiclrWhite));
        }

        // Cancel is on the right

        pctl = GetControlPtr(kidcCancel);
        if (pctl->GetFlags() & kfCtlVisible) {
            Rect rcT;
            pctl->GetRect(&rcT);
            int x = rcInside.right;
            int y = m_rc.top + rcT.top + rcT.Height() / 2;
            pbm->Fill(x, y, m_rc.left + rcT.left - x, 1, GetColor(kiclrWhite));
        }
    }

	bool fPlacementValid = ggobm.IsStructurePlacementValid(m_pstruc, m_tx, m_ty, m_pgobOwner->GetOwner());

	// Draw placement tiles, properly colored

	TerrainMap *ptmap = gsim.GetLevel()->GetTerrainMap();
	FogMap *pfogm = gsim.GetLevel()->GetFogMap();

	for (int ty = m_ty; ty < m_ty + m_pstruc->ctyReserve; ty++) {
		for (int tx = m_tx; tx < m_tx + m_pstruc->ctxReserve; tx++) {
			TBitmap *ptbm;
			if (fPlacementValid && IsTileFree(tx, ty, kbfReserved | kbfStructure) && pfogm->GetGalaxite(tx, ty) == 0) {
				ptbm = s_ptbmPlacementGood;
			} else {
				ptbm = s_ptbmPlacementBad;
			}
			ptbm->BltTo(pbm, (tx * gcxTile) - PcFromUwc(wxViewOrigin), (ty * gcyTile) - PcFromUwc(wyViewOrigin));
		}
	}
}

void PlaceStructureForm::OnPaintControlsSimUI(DibBitmap *pbm, UpdateMap *pupd)
{
	if (m_pgobOwner == NULL)
		return;
	Form::OnPaintControls(pbm, pupd);
}

} // namespace wi
