// Windows entry point. SDL2main maps WinMain to SDL_main, which does the
// SDL setup common to the game and then calls the game entrypoint
// wi::GameMain.

#include "game/ht.h"
#include "game/sdl/sdleventserver.h"
#include "base/thread.h"

int SDL_main(int argc, char *argv[])
{
    // The game runs on this thread. SDL events wake up its message queue.
    base::Thread *main_thread = new base::Thread(new wi::SdlEventServer());

    // Let the host have a pointer to the thread
    wi::HostSetGameThread(main_thread);

    wi::GameMain((char *)"");
    return 0;
}
