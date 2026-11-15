#ifndef __THREAD_H__
#define __THREAD_H__

#include "inc/basictypes.h"
#include "base/messagequeue.h"

namespace base {

// The game runs on a single thread. Thread is the message queue of that
// thread, plus its message loop.

class Thread : public MessageQueue {
public:
    Thread(EventServer *es);
    virtual ~Thread();

    static Thread& current() { return *s_current_; }

private:
    static Thread *s_current_;
};

} // namespace base

#endif // __THREAD_H__
