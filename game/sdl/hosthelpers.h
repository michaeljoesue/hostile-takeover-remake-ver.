#ifndef __HOSTHELPERS_H__
#define __HOSTHELPERS_H__

#include <stdarg.h>
#include "game/ht.h"

namespace wi {

struct SurfaceProperties {
    int cxWidth;
    int cyHeight;
    int cbxPitch;
    int cbyPitch;
    unsigned short ffFormat;
};

const unsigned short kfDirect565 = 0x0001;
const unsigned short kfDirect888 = 0x0002;
const unsigned short kfDirect8 = 0x0004;

class HostHelpers
{
public:
    static bool Init();
    static void Cleanup();

    static void Log(const char *pszFormat, ...);
    static void Log(const char *pszFormat, va_list va);
    static void MessageBox(const char *pszFormat, va_list va);
    static void Break();
    static void GetSurfaceProperties(SurfaceProperties *pprops);

    static const char *GetMainDataDir();
    static const char *GetTempDir();
    static const char *GetCompletesDir();
    static const char *GetSaveGamesDir();
    static const char *GetPrefsFilename();
};

} // namespace wi

#endif // __HOSTHELPERS_H__
