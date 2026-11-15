#ifndef __SDLEVENTSERVER_H__
#define __SDLEVENTSERVER_H__

#include "inc/basictypes.h"
#include "base/thread.h"
#include "base/eventserver.h"
#include "game/sdl/sysmessages.h"
#include <SDL.h>

namespace wi {

// Blocks the game's message queue until an SDL event arrives, the queue is
// woken up, or the wait times out.

class SdlEventServer : public base::EventServer {
public:
    SdlEventServer() : wait_(false) {}

    virtual bool Wait(long64 ctWait) {
        // Map ctWait (100th's sec) to timeout (milliseconds)
        int timeout = -1;
        if (ctWait != base::kctForever) {
            timeout = (int)ctWait * 10;
        }

        Uint32 expiration = 0;
        if (timeout > 0) {
            expiration = SDL_GetTicks() + timeout;
        }

        wait_ = true;
        while (wait_) {
            // This pumps native events
            SDL_PumpEvents();

            // Check for a single event. Peek here, let the caller Get.
            SDL_Event event;
            switch (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT,
                    SDL_LASTEVENT)) {
            case -1:
                // App is exiting, or there is an error
                return false;

            case 1:
                // There is an event
                base::Thread::current().Post(kidmSdlEvent, NULL);
                return true;

            case 0:
                // Polling and no events?
                if (timeout == 0) {
                    return true;
                }

                // Timeout expired and no events?
                if (timeout > 0 && ((int)(SDL_GetTicks() - expiration) >= 0)) {
                    return true;
                }

                // This is how SDL_WaitEventTimeout() does waiting.
                // 10ms is fine for this game.
                SDL_Delay(10);
                break;
            }
        }
        return true;
    }

    virtual void WakeUp() { wait_ = false; }

private:
    volatile bool wait_;
};

} // namespace wi

#endif // __SDLEVENTSERVER_H__
