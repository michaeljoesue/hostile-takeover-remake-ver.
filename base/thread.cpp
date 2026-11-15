#include "base/thread.h"
#include "base/tick.h"

namespace base {

Thread *Thread::s_current_;

Thread::Thread(EventServer *es) : MessageQueue(es) {
    // The first Thread created is the main (current) thread
    if (s_current_ == NULL) {
        s_current_ = this;
    }
}

Thread::~Thread() {
    Stop();
    if (s_current_ == this) {
        s_current_ = NULL;
    }
}

void Thread::RunLoop(long64 ct) {
    Thread& thread = current();

    long64 tEnd = 0;
    if (ct != kctForever) {
        tEnd = base::GetTickCount() + ct;
    }
    long64 ctNext = ct;

    while (true) {
        Message msg;
        if (!thread.Get(&msg, ctNext)) {
            return;
        }
        thread.Dispatch(&msg);

        if (ct != kctForever) {
            long64 tCur = base::GetTickCount();
            if (tCur >= tEnd) {
                return;
            }
            ctNext = tEnd - tCur;
        }
    }
}

} // namespace base
