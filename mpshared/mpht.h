#ifndef __MPHT_H__
#define __MPHT_H__

#include "inc/basictypes.h"
#include "mpshared/side.h"
#include "base/misc.h"

namespace wi {

#pragma pack(push, 2)

#define knVersionLevelSupported 1

// NOTE: Gob ids (gid) and StateMachine ids (smid) are interchangeable

typedef word Gid;	// gid
typedef word StateMachineId; // smid
typedef word Side; // side
typedef short UnitType;	// ut
typedef unsigned int UnitMask;	// um
typedef int TCoord;
typedef int WCoord;
typedef word Pid; // pid

const int kcPlayersMax = 5 + 1;	// + 1 computer controlled neutral player

const Pid kpidNeutral = 0xffff;

const int kcbLevelTitle = 40;
#define kcbFilename 29
const int kcbPlayerName = 32;

// Side masks

typedef word SideMask; // sidm

const SideMask ksidmNeutral = 1 << ksideNeutral;
const SideMask ksidmSide1 = 1 << kside1;
const SideMask ksidmSide2 = 1 << kside2;
const SideMask ksidmSide3 = 1 << kside3;
const SideMask ksidmSide4 = 1 << kside4;
const SideMask ksidmSide5 = 1 << kside5;
const SideMask ksidmAll = ksidmNeutral | ksidmSide1 | ksidmSide2 | ksidmSide3 | ksidmSide4 | ksidmSide5;

#define GetSideMask(side) (1 << side)

const word kfPlrInUse = 0x0001;
const word kfPlrComputer = 0x0004;
const word kfPlrUnfulfilled = 0x0008;
const word kfPlrStructureAttacked = 0x0010;
const word kfPlrComputerOvermind = 0x0020;
const word kfPlrAutoRepair = 0x0040;
const word kfPlrSummaryShown = 0x0200;

// Size

struct Size // siz
{
	int cx;
	int cy;
};

struct WPoint // pt
{
	WCoord wx, wy;
};

// Simulation messages. Commands are queued as arrays of Message structures,
// so all union'ed structs are 4 bytes length multiples.

struct Message // msg
{
	word mid;
	// UNDONE: smidSender isn't used except by DEBUG_HELPERS
	StateMachineId smidSender;
	StateMachineId smidReceiver;
	word wDummy; // for alignment
	int tDelivery;

	// MessageId-specific arguments

	union {
		struct {
			Gid gidAssailant;
			Side sideAssailant;
			// UNDONE: maybe a damage type (e.g., small-arms, area weapon)
			short nDamage;
			word wDummy;
		} Hit;

		struct {
			word sfx;
			word wDummy;
		} PlaySfx;

		struct {
			Gid gidEnemy;
			word wDummy;
		} EnemyNearby;

		struct {
			Gid gidTarget;
			WPoint wptTarget;
			WPoint wptTargetCenter;
			TCoord tcTargetRadius;
			WCoord wcMoveDistPerUpdate;
			word wDummy;
		} MoveCommand;

		// NOTE: both the gidTarget and wptTarget must be filled out so the
		// attacker can continue on to the target's location even if it no
		// longer exists.

		struct {
			Gid gidTarget;
			WPoint wptTarget;
			WPoint wptTargetCenter;
			TCoord tcTargetRadius;
			WCoord wcMoveDistPerUpdate;
			word wDummy;
		} AttackCommand;

		struct {
			word wfUpgrade;
			word wDummy;
		} UpgradeCommand;

		struct {
			word ut;
			WPoint wpt;
			word wDummy;
		} BuildOtherCommand;

		struct {
			UnitType ut;
			word wDummy;
		} AbortBuildOtherCommand;

		struct {
			Gid gidTarget;
			word wDummy;
		} DeliverCommand;

		struct {
			Gid gidTarget;
			WPoint wptTarget;
			word wDummy;
		} MineCommand;

		struct {
			word nArea;
			word wDummy;
		} GuardAreaCommand;

		struct {
			UnitMask um;
		} HuntEnemiesCommand;
	};
};

// NOTE: be sure to update the corresponding string table in StateMachine.cpp

enum MessageId { // mid
	kmidReservedNull,
	kmidReservedEnter,
	kmidReservedExit,
	kmidReservedUpdate,
	kmidHit,
	kmidNearbyAllyHit,
	kmidDelete,
	kmidEnemyNearby,
	kmidPowerLowHigh,
	kmidMoveWaitingNearby,
	kmidBeingUpgraded,
	kmidUpgradeComplete,
	kmidMoveCommand,
	kmidAttackCommand,
	kmidAnimationComplete,
	kmidBuildOtherCommand,
	kmidAbortBuildOtherCommand,
	kmidBuildComplete,
	kmidFireComplete,
	kmidSpawnSmoke,
	kmidSelfDestructCommand,
	kmidRepairCommand,
	kmidUpgradeCommand,
	kmidAbortUpgradeCommand,
	kmidTransformCommand,
	kmidDeliverCommand,
	kmidGalaxiteDelivery,
	kmidMineCommand,
	kmidMoveAction,
	kmidAttackAction,
	kmidGuardAction,
	kmidGuardVicinityAction,
	kmidGuardAreaAction,
	kmidHuntEnemiesAction,
	kmidFire,
	kmidHeal,
	kmidPlaySfx,

	// Added in 2.0 (at the end, so saved games keep their message numbers)

	kmidAttackMoveCommand,		// Move, attacking enemies on the way (MoveCommand)
	kmidStopCommand,			// Stop and guard
	kmidClearActionCommand,		// A new player order replaces the current one
	kmidForceAttackCommand,		// Ctrl+right-click: attack even a friend (AttackCommand)
	kmidGuardUnitCommand,		// Ctrl+Alt+right-click on a friend: guard it (AttackCommand.gidTarget)
};

STARTLABEL(MessageNames)
	LABEL(kmidReservedNull)
	LABEL(kmidReservedEnter)
	LABEL(kmidReservedExit)
	LABEL(kmidReservedUpdate)
	LABEL(kmidHit)
	LABEL(kmidNearbyAllyHit)
	LABEL(kmidDelete)
	LABEL(kmidEnemyNearby)
	LABEL(kmidPowerLowHigh)
	LABEL(kmidMoveWaitingNearby)
	LABEL(kmidBeingUpgraded)
	LABEL(kmidUpgradeComplete)
	LABEL(kmidMoveCommand)
	LABEL(kmidAttackCommand)
	LABEL(kmidAnimationComplete)
	LABEL(kmidBuildOtherCommand)
	LABEL(kmidAbortBuildOtherCommand)
	LABEL(kmidBuildComplete)
	LABEL(kmidFireComplete)
	LABEL(kmidSpawnSmoke)
	LABEL(kmidSelfDestructCommand)
	LABEL(kmidRepairCommand)
	LABEL(kmidUpgradeCommand)
	LABEL(kmidAbortUpgradeCommand)
	LABEL(kmidTransformCommand)
	LABEL(kmidDeliverCommand)
	LABEL(kmidGalaxiteDelivery)
	LABEL(kmidMineCommand)
	LABEL(kmidMoveAction)
	LABEL(kmidAttackAction)
	LABEL(kmidGuardAction)
	LABEL(kmidGuardVicinityAction)
	LABEL(kmidGuardAreaAction)
	LABEL(kmidHuntEnemiesAction)
	LABEL(kmidFire)
	LABEL(kmidHeal)
	LABEL(kmidPlaySfx)
	LABEL(kmidAttackMoveCommand)
	LABEL(kmidStopCommand)
	LABEL(kmidClearActionCommand)
	LABEL(kmidForceAttackCommand)
	LABEL(kmidGuardUnitCommand)
ENDLABEL(MessageNames)

#pragma pack(pop)

} // namespace wi

#endif // __MPHT_H__
