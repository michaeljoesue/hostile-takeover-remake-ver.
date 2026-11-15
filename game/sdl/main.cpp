// Windows entry point. SDL3's SDL_main.h maps WinMain to main, which sets up
// the game thread and calls the game entrypoint wi::GameMain.

#include "game/ht.h"
#include "game/sdl/sdleventserver.h"
#include "base/thread.h"
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[])
{
    // The game runs on this thread. SDL events wake up its message queue.
    base::Thread *main_thread = new base::Thread(new wi::SdlEventServer());

    // Let the host have a pointer to the thread
    wi::HostSetGameThread(main_thread);

    // -pdb <mission pack> and -l <level>, e.g. from the map editor's Run
    wi::HostSetCommandLine(argc, argv);

    wi::GameMain((char *)"");
    return 0;
}
