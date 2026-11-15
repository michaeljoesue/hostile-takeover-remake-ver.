#include "base/messagequeue.h"
#include "base/tick.h"

namespace base {

MessageQueue::MessageQueue(EventServer *es) : es_(es), stop_(false),
        pmsg_(NULL)
{
}

MessageQueue::~MessageQueue()
{
    Clear();
    delete es_;
}

void MessageQueue::Stop()
{
    stop_ = true;
    es_->WakeUp();
}

bool MessageQueue::Get(Message *pmsg, long64 ctWait)
{
    long64 tStart = GetTickCount();
    while (true) {
        {
            CritScope cs(&crit_);
            if (pmsg_ != NULL) {
                Message *pmsgT = pmsg_;
                pmsg_ = pmsg_->pmsgNext;
                *pmsg = *pmsgT;
                delete pmsgT;
                return true;
            }
        }

        // If asked, stop before waiting

        if (stop_) {
            break;
        }

        // Calculate wait remaining

        long64 ctNext = kctForever;
        if (ctWait != kctForever) {
            ctNext = ctWait - (GetTickCount() - tStart);
            if (ctNext < 0) {
                ctNext = 0;
            }
        }

        // Wait and multiplex

        es_->Wait(ctNext);

        // If the requested time has expired, return

        if (ctWait != kctForever && GetTickCount() - tStart >= ctWait) {
            return false;
        }
    }
    return false;
}

void MessageQueue::Post(Message *pmsg)
{
    if (stop_)
        return;

    // Copy message before putting in queue
    Message *pmsgNew = new Message;
    if (pmsgNew == NULL) {
        return;
    }
    *pmsgNew = *pmsg;
    pmsgNew->pmsgNext = NULL;

    // Put new message at the end
    CritScope cs(&crit_);
    Message **ppmsgT = &pmsg_;
    while (*ppmsgT != NULL) {
        ppmsgT = &((*ppmsgT)->pmsgNext);
    }
    *ppmsgT = pmsgNew;

    es_->WakeUp();
}

void MessageQueue::Clear()
{
    CritScope cs(&crit_);
    while (pmsg_ != NULL) {
        Message *pmsgT = pmsg_;
        pmsg_ = pmsg_->pmsgNext;
        delete pmsgT;
    }
}

} // namespace base
