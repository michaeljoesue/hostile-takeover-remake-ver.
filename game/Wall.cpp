#include "ht.h"

namespace wi {

//
// WallGob implementation
//
// A wall is one tile, owned by a side (neutral walls belong to no one). It
// blocks the tile like any structure, can be selected, repaired, sold and
// destroyed, and its picture joins the walls next to it (any side's).
// Units never pick walls as targets on their own: only when ordered to.
//

static StructConsts gConsts;
static AnimationData *s_panidWallExplosion;

bool WallGob::InitClass(IniReader *pini)
{
	gConsts.gt = kgtWall;
	gConsts.ut = kutWall;
	gConsts.umPrerequisites = kumImpossible;	// never built

	gConsts.sfxAbortRepair = ksfxMachineGunTowerAbortRepair;
	gConsts.sfxRepair = ksfxMachineGunTowerRepair;
	gConsts.sfxDamaged = ksfxNothing;
	gConsts.sfxSelect = ksfxMachineGunTowerSelect;
	gConsts.sfxDestroyed = ksfxMachineGunTowerDestroyed;
	gConsts.sfxImpact = ksfxNothing;

	if (!StructGob::InitClass(&gConsts, pini))
		return false;

	// A wall doesn't see far

	gConsts.wf &= ~kfUntcLargeDefog;

	s_panidWallExplosion = LoadAnimationData("vexplosion.anir");
	return s_panidWallExplosion != NULL;
}

void WallGob::ExitClass()
{
	delete s_panidWallExplosion;
	s_panidWallExplosion = NULL;
	StructGob::ExitClass(&gConsts);
}

WallGob::WallGob() : StructGob(&gConsts)
{
}

WallGob *WallGob::CreateAt(TCoord tx, TCoord ty, Player *pplr, fix fxHealth)
{
	if (pplr == NULL)
		return NULL;
	WallGob *pwall = new WallGob();
	if (pwall == NULL)
		return NULL;
	if (!pwall->Init(WcFromTc(tx), WcFromTc(ty), pplr, fxHealth, 0, NULL)) {
		delete pwall;
		return NULL;
	}
	return pwall;
}

bool WallGob::IsWallAt(TCoord tx, TCoord ty)
{
	TCoord ctx, cty;
	ggobm.GetMapSize(&ctx, &cty);
	if (tx < 0 || ty < 0 || tx >= ctx || ty >= cty)
		return false;
	Gob *pgob = ggobm.GetShadowGob(tx, ty);
	return pgob != NULL && pgob->GetType() == kgtWall && (pgob->GetFlags() & kfGobActive);
}

bool WallGob::Init(WCoord wx, WCoord wy, Player *pplr, fix fxHealth, dword ff, const char *pszName)
{
	if (!StructGob::Init(wx, wy, pplr, fxHealth, ff & ~kfGobBeingBuilt, pszName))
		return false;
	RedrawNeighbours();
	return true;
}

// Which neighbours are walls: 1 up, 2 right, 4 down, 8 left (the frame of
// the walls animation)

int WallGob::CalcNeighbourMask()
{
	TCoord tx = TcFromWc(m_wx);
	TCoord ty = TcFromWc(m_wy);
	int nMask = 0;
	if (IsWallAt(tx, ty - 1))
		nMask |= 1;
	if (IsWallAt(tx + 1, ty))
		nMask |= 2;
	if (IsWallAt(tx, ty + 1))
		nMask |= 4;
	if (IsWallAt(tx - 1, ty))
		nMask |= 8;
	return nMask;
}

void WallGob::RedrawNeighbours()
{
	TCoord tx = TcFromWc(m_wx);
	TCoord ty = TcFromWc(m_wy);
	static const int s_adx[4] = { 0, 1, 0, -1 };
	static const int s_ady[4] = { -1, 0, 1, 0 };
	for (int n = 0; n < 4; n++) {
		if (IsWallAt(tx + s_adx[n], ty + s_ady[n]))
			ggobm.GetShadowGob(tx + s_adx[n], ty + s_ady[n])->MarkRedraw();
	}
	MarkRedraw();
}

void WallGob::Draw(DibBitmap *pbm, int xViewOrigin, int yViewOrigin, int nLayer)
{
	// A destroyed wall is gone (the explosion covers it)

	if (m_st == kstDying)
		return;

	// Strip 0 healthy, 1 damaged; the frame joins the neighbours

	int nStrip = m_fxHealth * 2 < m_pstruc->GetArmorStrength() ? 1 : 0;
	if (m_ani.GetStrip() != nStrip)
		m_ani.SetStrip(nStrip);
	m_ani.SetFrame(CalcNeighbourMask());
	StructGob::Draw(pbm, xViewOrigin, yViewOrigin, nLayer);
}

void WallGob::SetHealth(fix fxHealth)
{
	// UnitGob's, without StructGob's strip change (Draw picks the strip)

	UnitGob::SetHealth(fxHealth);
}

bool WallGob::IsTakeoverable(Player *pplr)
{
	return false;
}

void WallGob::InitMenu(Form *pfrm)
{
	StructGob::InitMenu(pfrm);
}

int WallGob::ProcessStateMachineMessage(State st, Message *pmsg)
{
BeginStateMachine
	OnMsg(kmidHit)
		// Damage, without the base-under-attack alert and smoke of the
		// other buildings

		fix fxDamage = itofx(pmsg->Hit.nDamage);
		if (m_pplr->GetHandicap() & kfHcapIncreasedArmor)
			fxDamage = (fix)mulfx(fxDamage, (itofx(knDecreasedDamagePercent) / 100));
		fix fxHealthPrev = m_fxHealth;
		SetHealth(subfx(m_fxHealth, fxDamage));
		if (m_fxHealth <= 0) {
			SetState(kstDying);
		} else {
			ShowDamageIndicator();
			if (fxHealthPrev * 2 >= m_pstruc->GetArmorStrength() && m_fxHealth * 2 < m_pstruc->GetArmorStrength())
				MarkRedraw();
		}

		// Friends nearby fight back

		NotifyNearbyAlliesOfHit(pmsg->Hit.gidAssailant);

	//-----------------------------------------------------------------------

	State(kstIdle)
		OnEnter
			m_ani.Start(m_fxHealth * 2 < m_pstruc->GetArmorStrength() ? 1 : 0, 0, kfAniDone);

		OnUpdate
			// Nothing animates

			DefUpdate();

	//-----------------------------------------------------------------------

	State(kstDying)
		OnEnter
			if (m_ff & kfGobActive)
				Deactivate();
			if (m_pplr != NULL && m_pplr->GetSide() != ksideNeutral)
				m_pplr->IncStructuresLost();
			gsndm.PlaySfxAt(m_pstruc->sfxDestroyed, m_wx, m_wy);
			RedrawNeighbours();

			Gob *pgobExpl = CreateAnimGob(m_wx + kwcTileHalf, m_wy + kwcTileHalf,
					kfAnmDeleteWhenDone | kfAnmSmokeFireLayer, NULL, s_panidWallExplosion);
			if (pgobExpl != NULL)
				pgobExpl->SetOwner(m_pplr);

			// The tile opens once the explosion has died down

			gsmm.SendDelayedMsg(kmidDelete, 60, m_gid, m_gid);

		OnUpdate

		OnMsg(kmidDelete)
			Delete();
			return knDeleted;

		DiscardMsgs

	//-----------------------------------------------------------------------

            return knHandled;
        }
    } else {
        return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
    }
    return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
}

} // namespace wi
