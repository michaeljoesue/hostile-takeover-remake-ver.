#ifndef __DIAG_H__
#define __DIAG_H__

// Crash and hang reports (HostileTakeover-report.txt next to the game)

namespace wi {

void DiagInit();
void DiagHeartbeat();   // the message loop is alive

} // namespace wi

#endif // __DIAG_H__
