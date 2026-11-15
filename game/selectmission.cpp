#include "game/ht.h"
#include "base/format.h"
#include "game/completemanager.h"

namespace wi {

#define kcUnlockAhead 1
#define kfItemLocked 0x8000

SelectMissionForm::SelectMissionForm(MissionList *pml,
        const MissionIdentifier *pmiidFind) {
    m_pml = pml;
    m_pmiidFind = pmiidFind;
    m_mt = kmtStory;
    m_cMagic = 0;
    m_fMagicUnlock = false;
#ifdef DEBUG
    m_fMagicUnlock = true;
#endif
    memset(m_aplstc, 0, sizeof(m_aplstc));
}

bool SelectMissionForm::Init(FormMgr *pfrmm, IniReader *pini, word idf) {
	if (!ShellForm::Init(pfrmm, pini, idf))
		return false;

    // Format the lists, one per mission type

    int aidcList[] = { kidcStoryList, kidcChallengeList };
    for (int i = 0; i < ARRAYSIZE(aidcList); i++) {
        ListControl *plstc = (ListControl *)GetControlPtr(aidcList[i]);
        m_aplstc[i] = plstc;
        Rect rc;
        plstc->GetRect(&rc);
        Font *pfnt = gapfnt[kifntShadow];
        int cxComplete = pfnt->GetTextExtent("Complete");
        int xTitle = rc.Width() / 2 - cxComplete * 3 / 2;
        int xRightComplete = rc.Width() - 10;
        int xLeftComplete = xRightComplete - cxComplete - cxComplete / 2;
        plstc->SetTabStops(xTitle, xLeftComplete, xRightComplete);
        plstc->SetTabFlags(kfLstTabEllipsis, kfLstTabCenter, 0);
        plstc->SetFlags(plstc->GetFlags() | kfLstcKeepInteriorPositioning);
    }

    // If asked to find a certain mission, find it first to see what
    // type it is, and switch the radio button bar to that type.

    int iMission = -1;
	int cMissions = m_pml->GetCount();
    if (m_pmiidFind != NULL) {
        for (int i = 0; i < cMissions; i++) {
            MissionIdentifier miid;
            m_pml->GetMissionIdentifier(i, &miid);
            if (stricmp(miid.szLvlFilename, m_pmiidFind->szLvlFilename) == 0) {
                iMission = i;
                break;
            }
        }
    }

    // Init the lists

    MissionType mt = InitLists(iMission);
    SwitchToMissionType(mt);

	return true;
}

int SelectMissionForm::IndexFromMissionType(MissionType mt) {
    switch (mt) {
    case kmtStory:
        return 0;
    case kmtChallenge:
        return 1;
    default:
        return -1;
    }
}

MissionType SelectMissionForm::MissionTypeFromIndex(int i) {
    switch (i) {
    case 0:
        return kmtStory;
    case 1:
        return kmtChallenge;
    default:
        return kmtUnknown;
    }
}

void SelectMissionForm::SwitchToMissionType(MissionType mt) {
    m_mt = mt;
    RadioButtonBarControl *prbbc =
            (RadioButtonBarControl *)GetControlPtr(kidcCategories);
    prbbc->SetSelectionIndex(IndexFromMissionType(mt));
    for (int i = 0; i < ARRAYSIZE(m_aplstc); i++) {
        bool fShow = false;
        if (i == IndexFromMissionType(mt)) {
            fShow = true;
        }
        m_aplstc[i]->Show(fShow);
    }
    UpdateDescription();
}

MissionType SelectMissionForm::InitLists(int iMissionSelect) {
    // Fill in the lists, and along the way keep track of useful selection
    // indexes.

    int ailiFirstIncomplete[kmtUnknown + 1];
    memset(ailiFirstIncomplete, 0xff, sizeof(ailiFirstIncomplete));
    int iliLastCompleteStory = -1;
    int iliSelectSpecial = -1;
    MissionType mtSelectSpecial = kmtUnknown;

    int cMissions = m_pml->GetCount();
	for (int i = 0; i < cMissions; i++) {
        MissionDescription md;
        if (!m_pml->GetMissionDescription(i, &md)) {
            continue;
        }
        if (md.mt != kmtStory && md.mt != kmtChallenge) {
            continue;
        }

        // The first locked mission is kcUnlockAhead missions ahead of the
        // last complete story mission.

        ListControl *plstc = m_aplstc[IndexFromMissionType(md.mt)];
        bool fLocked = false;
        if (md.mt == kmtStory) {
            int iliMissionLocked = iliLastCompleteStory + 1 + kcUnlockAhead;
            int iliNext = plstc->GetCount();
            if (iliNext >= iliMissionLocked) {
                fLocked = true;
            }
        }

        // Get the status - LOCKED, Complete, or nothing

        MissionIdentifier miid;
        m_pml->GetMissionIdentifier(i, &miid);
        dword dw = (dword)i;
        const char *pszStatus = "";
        if (fLocked) {
            pszStatus = "LOCKED";
            dw |= kfItemLocked;
        } else {
            if (gpcptm->IsComplete(&miid)) {
                pszStatus = "Complete";
            }
        }

        // Add the item

        plstc->Add(base::Format::ToString("%s\t%s", md.szLvlTitle,
                pszStatus), (void *)(pword)dw);

        // Track the first incomplete for each mission type.

        if (ailiFirstIncomplete[md.mt] == -1) {
            if (!gpcptm->IsComplete(&miid)) {
                ailiFirstIncomplete[md.mt] = plstc->GetCount() - 1;
            }
        }

        // Track the last complete for story missions, for mission unlocking

        if (md.mt == kmtStory) {
            if (gpcptm->IsComplete(&miid)) {
                iliLastCompleteStory = plstc->GetCount() - 1;
            }
        }

        // This is passed in when the form needs to select a certain
        // mission when it first shows.

        if (i == iMissionSelect) {
            iliSelectSpecial = plstc->GetCount() - 1;
            mtSelectSpecial = md.mt;
        }
    }

    // The initially selected missions are the first incomplete missions
    // for each mission type.

    MissionType mtSelect = kmtStory;
    for (int i = 0; i < ARRAYSIZE(m_aplstc); i++) {
        ListControl *plstc = m_aplstc[i];

        // Is this the list that is awarded the special selection?

        if (i == IndexFromMissionType(mtSelectSpecial)) {
            plstc->Select(iliSelectSpecial, true, true);
            mtSelect = mtSelectSpecial;
            continue;
        }

        int iliSelect = ailiFirstIncomplete[MissionTypeFromIndex(i)];
        if (iliSelect < 0) {
            iliSelect = 0;
        }
        plstc->Select(iliSelect, true, true);
    }
    return mtSelect;
}

bool SelectMissionForm::OnPenEvent(Event *pevt) {
    if (m_mt != kmtStory || pevt->eType != penUpEvent) {
        return ShellForm::OnPenEvent(pevt);
    }

    Control *pctlCaptureBefore = GetControlCapture();
    bool f = ShellForm::OnPenEvent(pevt);
    Control *pctlCaptureAfter = GetControlCapture();

#define kcMagic 5

    // Tap down on back, move your finger off so it unhighlights, then
    // let up. Do this  kcMagic times and get a play button.
    if (pctlCaptureBefore != NULL && pctlCaptureAfter == NULL) {
        if (pctlCaptureBefore->GetId() == kidcCancel) {
            m_cMagic++;
            if (m_cMagic >= kcMagic) {
                m_cMagic = 0;
                m_fMagicUnlock = true;
            }
        }
    }
    return f;
}

void SelectMissionForm::OnControlSelected(word idc) {
    switch (idc) {
    case kidcOk:
    case kidcCancel:
        EndForm(idc);
        break;

    case kidcSetupGame:
        {
            ShellForm *pfrm = (ShellForm *)gpmfrmm->LoadForm(gpiniForms,
                    kidfInGameOptions, new InGameOptionsForm());
            if (pfrm != NULL) {
                pfrm->DoModal();
                delete pfrm;
            }
        }
        break;
    }
}

void SelectMissionForm::OnControlNotify(word idc, int nNotify) {
    if (idc == kidcCategories) {
        RadioButtonBarControl *prbbc =
                (RadioButtonBarControl *)GetControlPtr(kidcCategories);
        int iButtonSelected = prbbc->GetSelectionIndex();
        if (iButtonSelected < 0) {
            iButtonSelected = 0;
        }
        MissionType mtNew = MissionTypeFromIndex(iButtonSelected);
        if (mtNew == m_mt) {
            return;
        }
        SwitchToMissionType(mtNew);
    }

    if (idc == kidcStoryList || idc == kidcChallengeList) {
        // Update the mission description
        UpdateDescription();
    }

    // Handle button hiding

    bool fShow = true;
    ListControl *plstc = m_aplstc[IndexFromMissionType(m_mt)];
    if (plstc->GetSelectedItemIndex() == -1) {
        fShow = false;
    }
    if (m_mt == kmtStory) {
        if (IsSelectedMissionLocked(plstc)) {
            fShow = false;
        }
    }
    if (m_fMagicUnlock) {
        fShow = true;
    }
    GetControlPtr(kidcOk)->Show(fShow);
}

int SelectMissionForm::GetSelectedMissionIndex(ListControl *plstc) {
    pword pw = (pword)plstc->GetSelectedItemData();
    return (int)(pw & ~kfItemLocked);
}

bool SelectMissionForm::IsSelectedMissionLocked(ListControl *plstc) {
    pword pw = (pword)plstc->GetSelectedItemData();
    return (pw & kfItemLocked) != 0;
}

void SelectMissionForm::UpdateDescription() {
    LabelControl *plbl = (LabelControl *)GetControlPtr(kidcMissionPackInfo);
    ListControl *plstc = m_aplstc[IndexFromMissionType(m_mt)];
    if (plstc->GetSelectedItemIndex() == -1) {
        plbl->SetText("");
        return;
    }
    int i = GetSelectedMissionIndex(plstc);
    MissionDescription md;
    if (!m_pml->GetMissionDescription(i, &md)) {
        plbl->SetText("");
        return;
    }
    plbl->SetText(base::Format::ToString("%s by Spiffcode, Inc.",
            md.mt == kmtStory ? "Story Missions" : "Challenge Missions"));
}

bool SelectMissionForm::GetSelectedMission(MissionIdentifier *pmiid) {
    ListControl *plstc = m_aplstc[IndexFromMissionType(m_mt)];
    int i = GetSelectedMissionIndex(plstc);
    return m_pml->GetMissionIdentifier(i, pmiid);
}

} // namespace wi
