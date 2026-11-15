#ifndef __MESSAGEQUEUE_H__
#define __MESSAGEQUEUE_H__

#include "base/criticalsection.h"
#include "base/eventserver.h"
#include "inc/basictypes.h"
#include <string.h>

namespace base {

// The main thread's message queue. The SDL event server posts a message
// here whenever native events are waiting; HostGetEvent pulls them off and
// turns them into game Events.

struct Message {
    Message() { memset(this, 0, sizeof(*this)); }
    int id;
    long ms;
    Message *pmsgNext;
};

class MessageQueue {
public:
    MessageQueue(EventServer *es);
    virtual ~MessageQueue();

    void Stop();
    bool Get(Message *pmsg, long64 ctWait = kctForever);
    void Post(Message *pmsg);
    void Post(int id) {
        Message msg;
        msg.id = id;
        Post(&msg);
    }

private:
    void Clear();

    EventServer *es_;
    bool stop_;
    Message *pmsg_;
    CriticalSection crit_;
};

} // namespace base

#endif // __MESSAGEQUEUE_H__
