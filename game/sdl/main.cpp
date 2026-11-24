// Windows entry point. SDL3's SDL_main.h maps WinMain to main, which sets up
// the game thread and calls the game entrypoint wi::GameMain.

#include "game/ht.h"
#include "game/sdl/sdleventserver.h"
#include "base/thread.h"
#include "game/sdl/diag.h"
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[])
{
    // Crash and hang reports
    wi::DiagInit();

    // The game runs on this thread. SDL events wake up its message queue.
    new base::Thread(new wi::SdlEventServer());

    // -pdb <mission pack> and -l <level>, e.g. from the map editor's Run
    wi::HostSetCommandLine(argc, argv);

    wi::GameMain((char *)"");
    return 0;
}
