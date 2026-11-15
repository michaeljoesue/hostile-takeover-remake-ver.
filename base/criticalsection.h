#ifndef __CRITICALSECTION_H__
#define __CRITICALSECTION_H__

#include "inc/basictypes.h"
#include <mutex>

namespace base {

class CriticalSection {
public:
    void Enter() {
        mutex_.lock();
    }
    void Leave() {
        mutex_.unlock();
    }
private:
    std::recursive_mutex mutex_;
};

// CritScope, for serializing exection through a scope

class CritScope {
public:
    CritScope(CriticalSection *pcrit) {
        pcrit_ = pcrit;
        pcrit_->Enter();
    }
    ~CritScope() {
        pcrit_->Leave();
    }
private:
    CriticalSection *pcrit_;
};

} // namespace base

#endif // __CRITICALSECTION_H__
