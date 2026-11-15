#ifndef __EVENTSERVER_H__
#define __EVENTSERVER_H__

#include "inc/basictypes.h"

namespace base {

const long64 kctForever = -1;

// Blocks a message queue until there is something to do. The game's main
// thread uses an SDL based implementation (see game/sdl/sdleventserver.h).

class EventServer {
public:
    virtual ~EventServer() {}

    // Wait up to ctWait (1/100ths of a second), or until woken up.
    virtual bool Wait(long64 ctWait) = 0;
    virtual void WakeUp() = 0;
};

} // namespace base

#endif // __EVENTSERVER_H__
