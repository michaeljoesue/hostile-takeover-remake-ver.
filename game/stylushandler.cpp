#include "game/ht.h"

namespace wi {

// Left mouse button on the battlefield:
//
// - Click a unit or building to select it; double-click (or Ctrl+click) a
//   unit to select all of yours of that type on screen; Shift+click adds or
//   removes one of yours. Clicking one of your buildings also opens its
//   menu.
// - Click the ground to deselect.
// - Drag a box to select your units in it; Shift+drag adds them.
//
// Orders are given with the right button (SimUIForm::OnRightClick).

SimUIForm::StylusHandler::StylusHandler(SimUIForm *psui)
{
    m_psui = psui;
    m_fDragging = false;
    m_ffMods = 0;
    m_gidDown = kgidNull;
    m_agidKeep = new Gid[kcpgobMax];
    m_cgidKeep = 0;
}

SimUIForm::StylusHandler::~StylusHandler()
{
    delete[] m_agidKeep;
}

bool SimUIForm::StylusHandler::OnPenEvent(Event *pevt, bool fScrollOnly)
{
    // Can't scroll the playfield from this PenHandler

    if (fScrollOnly) {
        return true;
    }

	// Dragging mode captures all pen events for the duration of the drag

	if (m_fDragging && pevt->eType != penHoldEvent) {
		OnPenDrag(pevt);

		// Force draw to keep dragging as interactive as possible
		// Raises redraw priority above next update

		gevm.SetRedrawFlags(kfRedrawDirty | kfRedrawBeforeTimer);
		return true;
	}

	switch (pevt->eType) {
	case penDownEvent:
		OnPenDown(pevt);
		break;

	case penUpEvent:
		OnPenUp(pevt);
		break;
	}

	return true;
}

void SimUIForm::StylusHandler::OnPenDown(Event *pevt)
{
    // Enter dragging mode; this'll cause OnPenDrag to be called until
    // that mode exits.

	m_fDragging = true;
    m_ffMods = pevt->ffMods;

    // Calc a hit target in world coords

	WCoord wxTarget, wyTarget;
    Gob *pgobHit = m_psui->HitTestGob(pevt->x, pevt->y, &wxTarget, &wyTarget);
    m_gidDown = pgobHit != NULL ? pgobHit->GetId() : kgidNull;

	// s_wptSelect1 & 2 are used to determine the transition from a
	// tentative selection to a real one and must contain the beginning
	// and current pen positions, respectively.

	gsim.GetViewPos(&s_wptSelect1.wx, &s_wptSelect1.wy);
	s_wptSelect1.wx += WcFromPc(pevt->x);
	s_wptSelect1.wy += WcFromPc(pevt->y);
	s_wptSelect2 = s_wptSelect1;
	gwrcSelection.Set(s_wptSelect1, s_wptSelect2);

    // Shift+drag keeps what's selected

    m_cgidKeep = 0;
    if (m_ffMods & kfModShift) {
        for (Gob *pgobT = ggobm.GetFirstGob(); pgobT != NULL; pgobT = ggobm.GetNextGob(pgobT)) {
            if ((pgobT->GetFlags() & (kfGobUnit | kfGobSelected)) == (kfGobUnit | kfGobSelected) &&
                    m_cgidKeep < kcpgobMax) {
                m_agidKeep[m_cgidKeep++] = pgobT->GetId();
            }
        }
    }

    // Select what was clicked right away, for a responsive feel

    if (pgobHit != NULL)
        m_psui->ClickSelect(pgobHit, m_ffMods);
}

void SimUIForm::StylusHandler::OnPenUp(Event *pevt)
{
    CancelModes();
}

// TUNE:
#define kwcMinSelection WcFromTile16ths(8)

// Mouse drag on the playfield rubber-bands a selection rectangle.

void SimUIForm::StylusHandler::OnPenDrag(Event *pevt)
{
    WCoord wxView, wyView;
    gsim.GetViewPos(&wxView, &wyView);

	switch (pevt->eType) {
	case penUpEvent:
		// If this is a forced up event, cancel our modes

		if (pevt->dw == 1) {
			CancelModes();
			break;
		}

		m_fDragging = false;

		if (gfDragSelecting) {
			// Invalidate this area first so we get it to redraw

			m_psui->InvalidateDragSelection();
			gfDragSelecting = false;
            break;
		}

        // A click. On the ground: deselect. On one of the player's buildings:
        // open its menu.

        if (m_gidDown == kgidNull) {
            if ((m_ffMods & kfModShift) == 0)
                gsim.ClearGobSelection();
        } else {
            Gob *pgob = ggobm.GetGob(m_gidDown);
            if (pgob != NULL && (pgob->GetFlags() & kfGobStructure) &&
                    (m_ffMods & (kfModShift | kfModCtrl)) == 0 &&
                    (gfGodMode || pgob->GetSide() == gpplrLocal->GetSide())) {
                m_psui->ShowUnitMenu(pgob);
            }
        }
		break;

	case penMoveEvent:
		{
			if (gfDragSelecting) {
				// Inval old pos of selection rect

				m_psui->InvalidateDragSelection();
			}

            // Update() code uses s_wptSelect2 to determine if edge scrolling
            // is necessary

			s_wptSelect2.wx = wxView + WcFromPc(pevt->x);
			s_wptSelect2.wy = wyView + WcFromPc(pevt->y);
 			gwrcSelection.Set(s_wptSelect1, s_wptSelect2);

			if (!gfDragSelecting) {
				if (gwrcSelection.Width() >= kwcMinSelection ||
                        gwrcSelection.Height() >= kwcMinSelection) {
					gfDragSelecting = true;
				}
			}

			if (gfDragSelecting) {
				// Inval new size of selection rect

				m_psui->InvalidateDragSelection();
                UpdateDragSelection();
			}
		}
		break;
	}
}

// Select the player's units in the box, plus the ones kept by Shift

void SimUIForm::StylusHandler::UpdateDragSelection()
{
    gsim.SetSelection(&gwrcSelection);
    for (int i = 0; i < m_cgidKeep; i++) {
        Gob *pgob = ggobm.GetGob(m_agidKeep[i]);
        if (pgob != NULL && (pgob->GetFlags() & kfGobUnit) &&
                (gfGodMode || pgob->GetSide() == gpplrLocal->GetSide())) {
            ((UnitGob *)pgob)->Select(true);
        }
    }
}

void SimUIForm::StylusHandler::CancelModes()
{
	m_fDragging = false;

	if (gfDragSelecting) {
		// Invalidate this area first so we get it to redraw

		m_psui->InvalidateDragSelection();
		gfDragSelecting = false;
	}
}

void SimUIForm::StylusHandler::CheckScroll()
{
	// If the player is rectangle selecting and at the screen edge scroll the
	// screen.

    if (!gfDragSelecting) {
        return;
    }

    Size sizPlayfield;
    ggame.GetPlayfieldSize(&sizPlayfield);
    WCoord wcxPlayfield = WcFromUpc(sizPlayfield.cx);
    WCoord wcyPlayfield = WcFromUpc(sizPlayfield.cy);

    WCoord wxView, wyView;
    gsim.GetViewPos(&wxView, &wyView);

    WCoord wxViewNew = wxView;
    WCoord wyViewNew = wyView;

    if (s_wptSelect2.wx < wxView + kwcScrollBorderSize) {
        wxViewNew -= kwcScrollStepSize;
    } else if (s_wptSelect2.wx > wxView + wcxPlayfield - kwcScrollBorderSize) {
        wxViewNew += kwcScrollStepSize;
    }

    if (s_wptSelect2.wy < wyView + kwcScrollBorderSize) {
        wyViewNew -= kwcScrollStepSize;
    } else if (s_wptSelect2.wy > wyView + wcyPlayfield - kwcScrollBorderSize) {
        wyViewNew += kwcScrollStepSize;
    }

    if (wxViewNew != wxView || wyViewNew != wyView) {
        gsim.SetViewPos(wxViewNew, wyViewNew);
        WCoord wxViewActual, wyViewActual;
        gsim.GetViewPos(&wxViewActual, &wyViewActual);
        s_wptSelect2.wx += wxViewActual - wxView;
        s_wptSelect2.wy += wyViewActual - wyView;
        gwrcSelection.Set(s_wptSelect1, s_wptSelect2);
        UpdateDragSelection();
    }
}

} // namespace wi
