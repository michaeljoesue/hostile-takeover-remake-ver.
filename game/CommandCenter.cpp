#include "ht.h"

namespace wi {

//
// CommandCenterGob implementation
//
// A building with no functions: no menu (it isn't repaired or sold), no
// power, nothing built or researched, and it can't be built either:
// missions place it. It can be selected and attacked and destroyed like any
// building. It is drawn with the R&D Center's classic art; its HD pictures
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

// No menu: nothing to order, and it isn't repaired or sold

void CommandCenterGob::PopupMenu()
{
}

bool CommandCenterGob::IsTakeoverable(Player *pplr)
{
	return false;
}

int CommandCenterGob::ProcessStateMachineMessage(State st, Message *pmsg)
{
BeginStateMachine
	// Not repaired or sold

	OnMsg(kmidRepairCommand)

	OnMsg(kmidSelfDestructCommand)

            return knHandled;
        }
    } else {
        return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
    }
    return (int)StructGob::ProcessStateMachineMessage(st, pmsg);
}

} // namespace wi
