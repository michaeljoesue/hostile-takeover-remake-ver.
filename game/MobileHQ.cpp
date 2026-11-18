#include "ht.h"

namespace wi {

static MobileUnitConsts gConsts;


static int s_anMovingStripIndices[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

bool MobileHqGob::InitClass(IniReader *pini)
{
	gConsts.gt = kgtMobileHeadquarters;
	gConsts.ut = kutMobileHeadquarters;
	// Needs the Advanced VTS upgrade only (not an R&D Center as well)
	gConsts.upgmPrerequisites = kupgmAdvancedVTS;

	// Initialize the frame indices arrays

	gConsts.anFiringStripIndices = s_anMovingStripIndices;
	gConsts.anMovingStripIndices = s_anMovingStripIndices;
	gConsts.anIdleStripIndices = s_anMovingStripIndices;

	// Sound effects

	gConsts.sfxFire = ksfxNothing;
	gConsts.sfxImpact = ksfxNothing;

	gConsts.sfxcDestroyed = ksfxcVehicleDestroyed;
	gConsts.sfxcSelect = ksfxcMale01Select;
	gConsts.sfxcMove = ksfxcMale01Move;
	gConsts.sfxcAttack = ksfxcMale01Attack;

	return MobileUnitGob::InitClass(&gConsts, pini);
}

void MobileHqGob::ExitClass()
{
	MobileUnitGob::ExitClass(&gConsts);
}

MobileHqGob::MobileHqGob() : MobileUnitGob(&gConsts)
{
	m_wfMunt &= ~kfMuntAggressivenessBits;
	m_nDeploy = 0;
	m_txDeploy = 0;
	m_tyDeploy = 0;
}

bool MobileHqGob::CanTransform(TPoint *ptp)
{
	// optionally return the point we decide is ok so we can use it when we 
	// decide to transform

	Assert(gbldcHq.ctx == 3 && gbldcHq.cty == 2);	// Because of the various '-1's sprinkled below

	// Can't transform on top of Galaxite. Check for it. watch the map edge too.

	TPoint tpt;
	GetTilePosition(&tpt);
	TCoord tx = tpt.tx - 1;
	TCoord ty = tpt.ty - 1;
	if (tx < 0)
		tx = 0;
	if (ty < 0)
		ty = 0;
	TCoord txR = tx + gbldcHq.ctx;
	TCoord tyB = ty + gbldcHq.cty;

	// this logic works because we won't actually occupy the spot in txR, just up to it.
	Size siz;
	gsim.GetLevel()->GetTileMap()->GetTCoordMapSize(&siz);
	if (txR > siz.cx) {
		txR = siz.cx;
		tx = txR - gbldcHq.ctx;
	}
	if (tyB > siz.cy) {
		tyB = siz.cy;
		ty = tyB - gbldcHq.cty;
	}
	if (ptp != NULL) {
		ptp->tx = tx;
		ptp->ty = ty;
	}
	bool fOccupied = false;
	TCoord txT = tx;
	FogMap *pfogm = gsim.GetLevel()->GetFogMap();
	for (; ty < tyB; ty++) {
		for (tx = txT; tx < txR; tx++) {
			Gob *pgob;
			if (!IsTileFree(tx, ty, kbfReserved | kbfStructure, &pgob)) {
				if (pgob != this) {
					fOccupied = true;
					break;
				}
			}
			if (pfogm->GetGalaxite(tx, ty) != 0) {
				fOccupied = true;
				break;
			}
		}
	}

	return !fOccupied;
}

// Deploy (select a Dominion, right-click it, left-click where the
// Headquarters goes): the HQ's tiles must be on the map, free of buildings
// and units (pgobIgnore, the Dominion itself, doesn't count) and of Galaxite

bool MobileHqGob::CanDeployAt(TCoord tx, TCoord ty, Gob *pgobIgnore)
{
	Size siz;
	gsim.GetLevel()->GetTileMap()->GetTCoordMapSize(&siz);
	if (tx < 0 || ty < 0 || tx + gbldcHq.ctx > siz.cx ||
			ty + gbldcHq.cty > siz.cy) {
		return false;
	}
	FogMap *pfogm = gsim.GetLevel()->GetFogMap();
	for (TCoord tyT = ty; tyT < ty + gbldcHq.cty; tyT++) {
		for (TCoord txT = tx; txT < tx + gbldcHq.ctx; txT++) {
			Gob *pgob = NULL;
			if (!IsTileFree(txT, tyT, kbfReserved | kbfStructure, &pgob) &&
					(pgob == NULL || pgob != pgobIgnore)) {
				return false;
			}
			if (pfogm->GetGalaxite(txT, tyT) != 0)
				return false;
		}
	}
	return true;
}

// The Dominion transforms from the tile below the middle of the HQ's top row
// (see CanTransform), so it drives there first

void MobileHqGob::OrderDeploy(TCoord txHq, TCoord tyHq)
{
	m_txDeploy = txHq;
	m_tyDeploy = tyHq;

	TPoint tpt;
	GetTilePosition(&tpt);
	if (tpt.tx == txHq + 1 && tpt.ty == tyHq + 1) {
		m_nDeploy = 0;
		gcmdq.Enqueue(kmidClearActionCommand, m_gid);
		gcmdq.Enqueue(kmidTransformCommand, m_gid);
		return;
	}

	WCoord wx = WcFromTc(txHq + 1) + kwcTileHalf;
	WCoord wy = WcFromTc(tyHq + 1) + kwcTileHalf;
	MobileUnitGob::SetTarget(kgidNull, wx, wy, wx, wy, 0,
			m_pmuntc->GetMoveDistPerUpdate());
	m_nDeploy = 1;
}

// Any other order calls the deploy off

void MobileHqGob::SetTarget(Gid gid, WCoord wx, WCoord wy, WCoord wxCenter,
		WCoord wyCenter, TCoord tcRadius, WCoord wcMoveDistPerUpdate)
{
	m_nDeploy = 0;
	MobileUnitGob::SetTarget(gid, wx, wy, wxCenter, wyCenter, tcRadius,
			wcMoveDistPerUpdate);
}

int MobileHqGob::ProcessStateMachineMessage(State st, Message *pmsg)
{
	// Deploy: the move starts (kstMove), then ends (kstGuard): on the spot,
	// transform; stopped short (blocked), give up

	if (pmsg->mid == kmidReservedEnter && m_nDeploy != 0) {
		if (st == kstMove) {
			m_nDeploy = 2;
		} else if (st == kstGuard && m_nDeploy == 2) {
			m_nDeploy = 0;
			TPoint tpt;
			GetTilePosition(&tpt);
			if (tpt.tx == m_txDeploy + 1 && tpt.ty == m_tyDeploy + 1 &&
					CanTransform()) {
				gcmdq.Enqueue(kmidTransformCommand, m_gid);
			} else if (m_pplr == gpplrLocal) {
				ShowAlert("Dominion can't deploy there");
			}
		}
	}
	if (pmsg->mid == kmidStopCommand)
		m_nDeploy = 0;

BeginStateMachine
	OnMsg(kmidTransformCommand)

		// Due to client-server lag the space wanted for transformation
		// may no longer be free. Double-check and if it isn't simply fail.
		// UNDONE: bzzzt

		TPoint tptDest;
		if (!CanTransform(&tptDest))
			return knHandled;

		// Deactivate the MobileHQ

		Deactivate();

		// UNDONE: do some cool animation

		// Remove self from the Gob list

		ggobm.RemoveGob(this);

		// Start sfx

		if (m_pplr == gpplrLocal)
			gsndm.PlaySfx(ksfxMobileHeadquartersDeploy);

		// Create Headquarters

		HqGob *pgobHq = (HqGob *)CreateGob(kgtHeadquarters);
		if (pgobHq != NULL) {

			// Carry the MHQ's health forward to the HQ

			fix fxHealth = (fix)divfx(mulfx(m_fxHealth, gapuntc[kutHeadquarters]->GetArmorStrength()), m_puntc->GetArmorStrength());
			pgobHq->Init(WcFromTc(tptDest.tx), WcFromTc(tptDest.ty), m_pplr, fxHealth, 0, NULL);
		}

		// Delete self

		delete this;
		return knDeleted;

	OnMsg(kmidAttackCommand)

		// UNDONE: MobileHqs don't have an attack at the moment so they ignore
		// their auto-response impulses to chase their attacker

		// DO NOTHING (override Unit's Attack response)

            return knHandled;
        }
    } else {
        return (int)MobileUnitGob::ProcessStateMachineMessage(st, pmsg); 
    }
    return (int)MobileUnitGob::ProcessStateMachineMessage(st, pmsg);
}

} // namespace wi
