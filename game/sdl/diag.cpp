// Crash and hang reports.
//
// If the game crashes, or stops answering Windows (the window shows "Not
// responding") for 10 seconds, a report is added to
// HostileTakeover-report.txt next to the game: what happened and where in
// the game's code it was (the call stack as addresses in
// HostileTakeover.exe). Those addresses can be turned into function names
// and lines with the very same build:
//
//   addr2line -f -C -i -e HostileTakeover.exe <address> ...
//
// The heartbeat is the game's message loop (SdlEventServer::Wait): while it
// runs, the window answers Windows.

#include "game/sdl/diag.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <time.h>

namespace wi {

static volatile LONG64 s_msBeat;
static HANDLE s_hthrMain;
static char s_szReport[MAX_PATH];
static char s_szExe[MAX_PATH];
static ULONG_PTR s_nbExe;
static const ULONG_PTR knExePreferredBase = 0x140000000;
static CRITICAL_SECTION s_csReport;

void DiagHeartbeat()
{
    InterlockedExchange64(&s_msBeat, (LONG64)GetTickCount64());
}

static FILE *OpenReport()
{
    return fopen(s_szReport, "a");
}

static void WriteHeader(FILE *pf, const char *pszWhat)
{
    time_t t = time(NULL);
    char szTime[64];
    strftime(szTime, sizeof(szTime), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(pf, "\n==== %s  %s\n", szTime, pszWhat);
    fprintf(pf, "exe: %s (loaded at %p)\n", s_szExe, (void *)s_nbExe);
}

// One stack frame: the address in HostileTakeover.exe as addr2line wants
// it (its preferred load address), or the module it is in

static void WriteFrame(FILE *pf, int n, DWORD64 rip)
{
    HMODULE hmod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(ULONG_PTR)rip, &hmod);
    if (hmod == (HMODULE)s_nbExe) {
        fprintf(pf, "  #%02d 0x%llx\n", n,
                (unsigned long long)(rip - s_nbExe + knExePreferredBase));
        return;
    }
    char szMod[MAX_PATH] = "?";
    if (hmod != NULL)
        GetModuleFileNameA(hmod, szMod, sizeof(szMod));
    const char *pszName = strrchr(szMod, '\\');
    pszName = pszName != NULL ? pszName + 1 : szMod;
    fprintf(pf, "  #%02d %s+0x%llx\n", n, pszName,
            (unsigned long long)(rip - (DWORD64)(ULONG_PTR)hmod));
}

// Walk the stack from a thread context using the x64 unwind tables

static void WriteStack(FILE *pf, CONTEXT *pctx)
{
    CONTEXT ctx = *pctx;
    for (int n = 0; n < 48 && ctx.Rip != 0; n++) {
        WriteFrame(pf, n, ctx.Rip);
        DWORD64 nbImage = 0;
        PRUNTIME_FUNCTION pfn = RtlLookupFunctionEntry(ctx.Rip, &nbImage, NULL);
        if (pfn == NULL) {
            // Leaf function: the return address is on top of the stack
            if (IsBadReadPtr((void *)(ULONG_PTR)ctx.Rsp, 8))
                break;
            ctx.Rip = *(DWORD64 *)(ULONG_PTR)ctx.Rsp;
            ctx.Rsp += 8;
        } else {
            PVOID pvHandler = NULL;
            DWORD64 nbEstablisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, nbImage, ctx.Rip, pfn, &ctx,
                    &pvHandler, &nbEstablisher, NULL);
        }
    }
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS *pep)
{
    EnterCriticalSection(&s_csReport);
    FILE *pf = OpenReport();
    if (pf != NULL) {
        char sz[128];
        snprintf(sz, sizeof(sz), "CRASH: exception 0x%08lx",
                (unsigned long)pep->ExceptionRecord->ExceptionCode);
        WriteHeader(pf, sz);
        if (pep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
                pep->ExceptionRecord->NumberParameters >= 2) {
            fprintf(pf, "%s address %p\n",
                    pep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                    (void *)pep->ExceptionRecord->ExceptionInformation[1]);
        }
        fprintf(pf, "stack:\n");
        WriteStack(pf, pep->ContextRecord);
        fclose(pf);
    }
    LeaveCriticalSection(&s_csReport);
    return EXCEPTION_CONTINUE_SEARCH;
}

// Watches the heartbeat. After 10 s without one, it writes where the game
// is (three samples a second apart show what it keeps doing), once per hang.

static DWORD WINAPI Watchdog(LPVOID)
{
    bool fReported = false;
    while (true) {
        Sleep(1000);
        LONG64 msBeat = InterlockedCompareExchange64(&s_msBeat, 0, 0);
        if (msBeat == 0)
            continue;
        LONG64 msSince = (LONG64)GetTickCount64() - msBeat;
        if (msSince < 10000) {
            fReported = false;
            continue;
        }
        if (fReported)
            continue;
        fReported = true;

        EnterCriticalSection(&s_csReport);
        FILE *pf = OpenReport();
        if (pf != NULL) {
            char sz[128];
            snprintf(sz, sizeof(sz), "HANG: no answer for %lld s", (long long)(msSince / 1000));
            WriteHeader(pf, sz);
            for (int nSample = 0; nSample < 3; nSample++) {
                if (nSample != 0)
                    Sleep(1000);
                if (SuspendThread(s_hthrMain) == (DWORD)-1)
                    break;
                CONTEXT ctx;
                memset(&ctx, 0, sizeof(ctx));
                ctx.ContextFlags = CONTEXT_FULL;
                if (GetThreadContext(s_hthrMain, &ctx)) {
                    fprintf(pf, "stack (sample %d):\n", nSample + 1);
                    WriteStack(pf, &ctx);
                }
                ResumeThread(s_hthrMain);
            }
            fclose(pf);
        }
        LeaveCriticalSection(&s_csReport);
    }
    return 0;
}

void DiagInit()
{
    InitializeCriticalSection(&s_csReport);
    s_nbExe = (ULONG_PTR)GetModuleHandleA(NULL);
    GetModuleFileNameA(NULL, s_szExe, sizeof(s_szExe));
    strcpy(s_szReport, s_szExe);
    char *pszSlash = strrchr(s_szReport, '\\');
    if (pszSlash != NULL)
        pszSlash[1] = 0;
    else
        s_szReport[0] = 0;
    strncat(s_szReport, "HostileTakeover-report.txt", sizeof(s_szReport) - strlen(s_szReport) - 1);

    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
            &s_hthrMain, THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
            FALSE, 0);
    SetUnhandledExceptionFilter(CrashFilter);
    HANDLE hthr = CreateThread(NULL, 0, Watchdog, NULL, 0, NULL);
    if (hthr != NULL)
        CloseHandle(hthr);
}

} // namespace wi

#else

namespace wi {
void DiagInit() {}
void DiagHeartbeat() {}
}

#endif
