#include "game/ht.h"
#include "game/sdl/hosthelpers.h"
#include "base/tick.h"
#include "base/log.h"
#include "base/thread.h"
#include "game/completemanager.h"
#include <errno.h>
#include <time.h>
#include <SDL3/SDL.h>

namespace wi {

SdlPackFileReader gpakr;
CompleteManager *gpcptm;
base::Thread *gpgt;

bool HostInit()
{
    if (!HostHelpers::Init()) {
        return false;
    }

    gpcptm = new CompleteManager(HostHelpers::GetCompletesDir());
    gpcptm->Init();
    return true;
}

void HostExit()
{
    delete gpcptm;
    gpcptm = NULL;

    HostHelpers::Cleanup();
}

static void StepGameSpeed(int nDelta)
{
    int i = 0;
    for (; i < ARRAYSIZE(gatGameSpeeds); i++)
        if (gatGameSpeeds[i] == gtGameSpeed)
            break;
    i += nDelta;
    if (i < 0)
        i = 0;
    if (i >= ARRAYSIZE(gatGameSpeeds))
        i = ARRAYSIZE(gatGameSpeeds) - 1;
    ggame.SetGameSpeed(gatGameSpeeds[i]);
}

static void RedrawWindow()
{
    if (gpmfrmm != NULL && gpdisp != NULL) {
        gpmfrmm->DrawFrame(true);
        gpdisp->RenderGameSurface();
    }
}

// Mouse positions arrive in window coordinates; the game wants them in game
// pixels, which are scaled and letterboxed into the window.

static void MapMousePosition(SDL_Event *pevent, Event *pevt, float x, float y)
{
    if (gpdisp != NULL && gpdisp->GetRenderer() != NULL)
        SDL_ConvertEventToRenderCoordinates(gpdisp->GetRenderer(), pevent);
    if (pevent->type == SDL_EVENT_MOUSE_MOTION) {
        x = pevent->motion.x;
        y = pevent->motion.y;
    } else {
        x = pevent->button.x;
        y = pevent->button.y;
    }
    Size siz = { 0, 0 };
    if (gpdisp != NULL)
        gpdisp->GetSize(&siz);
    int xT = (int)SDL_floorf(x);
    int yT = (int)SDL_floorf(y);
    if (siz.cx > 0) {
        xT = SDL_clamp(xT, 0, siz.cx - 1);
        yT = SDL_clamp(yT, 0, siz.cy - 1);
    }
    pevt->x = xT;
    pevt->y = yT;
}

bool ProcessSdlEvent(base::Message *pmsg, Event *pevt)
{
    memset(pevt, 0, sizeof(*pevt));
    SDL_Event event;
    switch (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST,
            SDL_EVENT_LAST)) {
    case -1:
        // App is exiting, or there is an error
        pevt->eType = appStopEvent;
        break;

    case 0:
        return false;
    }

	switch (event.type) {

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button != SDL_BUTTON_LEFT)
            return false;
        pevt->eType = penDownEvent;
        MapMousePosition(&event, pevt, event.button.x, event.button.y);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button != SDL_BUTTON_LEFT)
            return false;
        pevt->eType = penUpEvent;
        MapMousePosition(&event, pevt, event.button.x, event.button.y);
		break;

	case SDL_EVENT_MOUSE_MOTION:
        pevt->eType = penMoveEvent;
        MapMousePosition(&event, pevt, event.motion.x, event.motion.y);
		break;

	case SDL_EVENT_KEY_DOWN:
        // Alt+Enter and F11 toggle fullscreen

        if ((event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT)) ||
                event.key.key == SDLK_F11) {
            if (!event.key.repeat && gpdisp != NULL) {
                gpdisp->SetFullscreen(!gpdisp->IsFullscreen());
                ggame.SavePreferences();
            }
            return false;
        }

        pevt->eType = keyDownEvent;
        switch (event.key.key) {
		case SDLK_UP:
			pevt->chr = chrPageUp;
			break;

		case SDLK_DOWN:
			pevt->chr = chrPageDown;
			break;

		case SDLK_LEFT:
			pevt->chr = chrLeft;
			break;

		case SDLK_RIGHT:
			pevt->chr = chrRight;
			break;

		case SDLK_BACKSPACE:
			pevt->chr = chrBackspace;
			break;

		case SDLK_DELETE:
			pevt->chr = chrDelete;
			break;

		case SDLK_KP_MINUS:
            StepGameSpeed(-1);
			break;

		case SDLK_KP_PLUS:
            StepGameSpeed(1);
			break;

        default:
            pevt->chr = event.key.key;
            break;
        }
        break;

	case SDL_EVENT_QUIT:
        pevt->eType = appStopEvent;
        break;

    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        gfFullscreen = true;
        RedrawWindow();
        return false;

    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        gfFullscreen = false;
        RedrawWindow();
        return false;

    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESTORED:
        RedrawWindow();
        return false;

    default:
        return false;
    }

    pevt->ms = pmsg->ms;
    if (pevt->ms == 0)
        pevt->ms = HostGetMillisecondCount();

    return true;
}

bool ProcessMessage(base::Message *pmsg, Event *pevt)
{
    memset(pevt, 0, sizeof(*pevt));
    switch (pmsg->id) {
    case base::kidmNullEvent:
        pevt->eType = nullEvent;
        pevt->ff = 0;
        break;

    default:
        return false;
    }

    pevt->dw = 0;
    pevt->x = pmsg->x;
    pevt->y = pmsg->y;
    pevt->ms = pmsg->ms;
    if (pmsg->ff & kfMsgCoalesce) {
        pevt->ff |= kfEvtCoalesce;
    }
    if (pmsg->ff & kfMsgCancelMode) {
        pevt->dw = 1;
    }
    
    return true;
}

bool HostGetEvent(Event *pevt, long ctWait)
{
    base::Thread& thread = base::Thread::current();
    while (true) {
        base::Message msg;
        if (!thread.Get(&msg, ctWait)) {
            return false;
        }

        if (msg.handler != NULL) {
            thread.Dispatch(&msg);
            continue;
        }

        if (msg.id == kidmSdlEvent) {
            if (ProcessSdlEvent(&msg, pevt)) {
                return true;
            }
            continue;
        }

        if (ProcessMessage(&msg, pevt)) {
            return true;
        }
    }
}

void HostOutputDebugString(char *pszFormat, ...)
{
#ifdef DEBUG
    va_list va;
    va_start(va, pszFormat);
    HostHelpers::Log(pszFormat, va);
    va_end(va);
#endif
}

long HostGetTickCount()
{
    return (long)base::GetTickCount();
}

long HostGetMillisecondCount()
{
    return (long)base::GetMillisecondCount();
}
    
// Map the arrow keys to the scroll keys the game polls (see InputUIForm)

dword HostGetCurrentKeyState(dword keyBit)
{
    const bool *pbKeys = SDL_GetKeyboardState(NULL);
    dword dw = 0;
    if (pbKeys[SDL_SCANCODE_UP])
        dw |= keyBitRockerUp;
    if (pbKeys[SDL_SCANCODE_DOWN])
        dw |= keyBitRockerDown;
    if (pbKeys[SDL_SCANCODE_LEFT])
        dw |= keyBitRockerLeft;
    if (pbKeys[SDL_SCANCODE_RIGHT])
        dw |= keyBitRockerRight;
    return dw & keyBit;
}

void HostMessageBox(TCHAR *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    HostHelpers::MessageBox(pszFormat, va);
    va_end(va);
}

// UNDONE: prefix directory?

FileHandle HostOpenFile(const char *pszFilename, word wf)
{
    const char *pszMode;
    if (wf == kfOfRead)
        pszMode = "rb";
    else if (wf == kfOfWrite)
        pszMode = "wb";
    else if (wf == (kfOfRead | kfOfWrite))
        pszMode = "rb+";

    return (FileHandle)fopen((char *)pszFilename, pszMode);
}

void HostCloseFile(FileHandle hf)
{
    fclose((FILE *)hf);
}

dword HostWriteFile(FileHandle hf, void *pv, dword cb)
{
    return fwrite(pv, 1, cb, (FILE *)hf);
}

dword HostReadFile(FileHandle hf, void *pv, dword cb)
{
    return fread(pv, 1, cb, (FILE *)hf);
}

void HostSleep(dword ct)
{
    SDL_Delay(ct * 10);
}

void HostGetSilkRect(int irc, Rect *prc)
{
    return;
}

// Figure out what kind of sound device exists, and return a SoundDevice for it

SoundDevice *HostOpenSoundDevice()
{
    return CreateSdlSoundDevice();
}

SoundDevice::~SoundDevice()
{
}

// Used for sound buffer maintenance requirements

void HostGetCurrentDate(Date *pdate)
{
    time_t result = time(NULL);
    struct tm *ptm = localtime(&result);
    pdate->nYear = ptm->tm_year + 1900;
    pdate->nMonth = ptm->tm_mon + 1;
    pdate->nDay = ptm->tm_mday;
}

bool HostSavePreferences(void *pv, int cb)
{
    LOG() << HostHelpers::GetPrefsFilename();

    FILE *pf = fopen(HostHelpers::GetPrefsFilename(), "wb");
    if (pf == NULL) {
        LOG() << "error opening preferences! " << errno;
        return false;
    }
    if (fwrite(pv, cb, 1, pf) != 1) {
        LOG() << "error writing preferences! " << errno;
        fclose(pf);
        return false;
    }
    fclose(pf);
    return true;
}

int HostLoadPreferences(void *pv, int cb)
{
    FILE *pf = fopen(HostHelpers::GetPrefsFilename(), "rb");
    if (pf == NULL) {
        return -1;
    }

    // Read prefs

    int cbRead = (int)fread(pv, 1, cb, pf);
    fclose(pf);
    return cbRead;
}

const char *HostGetMainDataDir()
{
    return HostHelpers::GetMainDataDir();
}

const char *HostGetSaveGamesDir()
{
    return HostHelpers::GetSaveGamesDir();
}

void HostNotEnoughMemory(bool fStorage, dword cbFree, dword cbNeed)
{
	HostMessageBox(TEXT((char *)"Need %ld bytes of memory but only %ld bytes are free!"), cbNeed, cbFree);
}

void HostSetGameThread(base::Thread *thread) {
    gpgt = thread;
}

base::Thread& HostGetGameThread() {
    return *gpgt;
}

} // namespace wi
