#include "inc/basictypes.h"
#include "base/tick.h"
#include <chrono>
#include <time.h>

namespace base {

static std::chrono::steady_clock::duration ElapsedSinceStart() {
    // Start one second in the past, so tick counts are never zero
    static const std::chrono::steady_clock::time_point s_start =
            std::chrono::steady_clock::now() - std::chrono::seconds(1);
    return std::chrono::steady_clock::now() - s_start;
}

long64 GetTickCount() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            ElapsedSinceStart()).count() / 10;
}

long64 GetMillisecondCount() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            ElapsedSinceStart()).count();
}

} // namespace base
