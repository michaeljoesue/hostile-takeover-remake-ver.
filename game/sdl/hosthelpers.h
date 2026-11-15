#ifndef __HOSTHELPERS_H__
#define __HOSTHELPERS_H__

#include <stdarg.h>
#include "game/ht.h"

namespace wi {

class HostHelpers
{
public:
    static bool Init();
    static void Cleanup();

    static void Log(const char *pszFormat, ...);
    static void Log(const char *pszFormat, va_list va);
    static void MessageBox(const char *pszFormat, va_list va);
    static void Break();

    static const char *GetMainDataDir();
    static const char *GetTempDir();
    static const char *GetCompletesDir();
    static const char *GetSaveGamesDir();
    static const char *GetPrefsFilename();
};

} // namespace wi

#endif // __HOSTHELPERS_H__
