#include "ht.h"

namespace wi {

//
// CommandCenterGob implementation
//
// A building with no functions: no power, nothing built or researched, and
// it can't be built either: missions place it. Its menu only repairs it (by
// the player, or the auto repair of the player or a computer player); it
// isn't sold. Like any building it can be attacked and destroyed, and a
// Corporate Raider can capture it. It is drawn with the R&D Center's classic art; its HD pictures
// are the dish version of the R&D Center's (HdAnimation in GobTemplates).
//

static StructConsts gConsts;

bool CommandCenterGob::InitClass(IniReader *pini)
{
	gConsts.gt = kgtCommandCenter;
	gConsts.ut = kutCommandCenter;
	gConsts.umPrerequisites = kumImpossible;	// never built

	gConsts.sfxAbortRepair = ksfxResearchCenterAbortRepair;
	gConsts.sfxRepair = ksfxResearchCenterRepair;
	gConsts.sfxDamaged = ksfxResearchCenterDamaged;
	gConsts.sfxSelect = ksfxResearchCenterSelect;
	gConsts.sfxDestroyed = ksfxResearchCenterDestroyed;
	gConsts.sfxImpact = ksfxNothing;

	return StructGob::InitClass(&gConsts, pini);
}

void CommandCenterGob::ExitClass()
{
	StructGob::ExitClass(&gConsts);
}

CommandCenterGob::CommandCenterGob() : StructGob(&gConsts)
{
}

// The building menu, Repair only (Sell is greyed out)

void CommandCenterGob::InitMenu(Form *pfrm)
{
	StructGob::InitMenu(pfrm);
	ButtonControl *pbtn = (ButtonControl *)pfrm->GetControlPtr(kidcSelfDestruct);
	if (pbtn != NULL)
		pbtn->Enable(false);
}

int CommandCenterGob::ProcessStateMachineMessage(State st, Message *pmsg)
{
BeginStateMachine
	// Not sold

	OnMsg(kmidSelfDestructCommand)

            return knHandled;
        }
    } else {
        return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
    }
    return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
}

} // namespace wi
