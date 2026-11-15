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

} // namespace base
