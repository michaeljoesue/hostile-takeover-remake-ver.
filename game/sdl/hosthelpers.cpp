// Windows implementation of the host helpers: file locations, logging and
// message boxes.

#include "game/sdl/hosthelpers.h"
#include "base/thread.h"
#include <stdio.h>
#include <string>
#include <filesystem>
#include <SDL3/SDL.h>
#include "game/ht.h"

namespace wi {

static std::string gstrMainDataDir;     // htdata832.pdb, htsfx.pdb (exe dir)
static std::string gstrUserDir;         // %APPDATA%\Spiffcode\HostileTakeover
static std::string gstrTempDir;
static std::string gstrSaveGamesDir;
static std::string gstrCompletesDir;    // "mission completed" tracking
static std::string gstrPrefsFilename;

// Directories are returned with a trailing path separator removed, except
// for the completes directory, whose users append file names directly.

static std::string StripSeparator(const char *psz)
{
    std::string str(psz);
    while (!str.empty() && (str.back() == '\\' || str.back() == '/')) {
        str.pop_back();
    }
    return str;
}

static void MakeDir(const std::string& str)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(str), ec);
}

bool HostHelpers::Init()
{
    // Game data lives next to the executable
    const char *pszBase = SDL_GetBasePath();
    if (pszBase == NULL) {
        Log("SDL_GetBasePath failed: %s", SDL_GetError());
        return false;
    }
    gstrMainDataDir = StripSeparator(pszBase);

    // Saved games and settings live in the user's roaming app data folder
    char *pszPref = SDL_GetPrefPath("Spiffcode", "HostileTakeover");
    if (pszPref == NULL) {
        Log("SDL_GetPrefPath failed: %s", SDL_GetError());
        return false;
    }
    gstrUserDir = StripSeparator(pszPref);
    SDL_free(pszPref);

    gstrTempDir = gstrUserDir + "\\tmp";
    gstrSaveGamesDir = gstrUserDir + "\\SaveGames";
    gstrCompletesDir = gstrUserDir + "\\Completes\\";
    gstrPrefsFilename = gstrUserDir + "\\prefs.bin";

    MakeDir(gstrTempDir);
    MakeDir(gstrSaveGamesDir);
    MakeDir(gstrCompletesDir);
    return true;
}

void HostHelpers::Cleanup()
{
    HostDestroyWindow();
    SDL_Quit();
}

const char *HostHelpers::GetMainDataDir()
{
    return gstrMainDataDir.c_str();
}

const char *HostHelpers::GetTempDir()
{
    return gstrTempDir.c_str();
}

const char *HostHelpers::GetSaveGamesDir()
{
    return gstrSaveGamesDir.c_str();
}

const char *HostHelpers::GetCompletesDir()
{
    return gstrCompletesDir.c_str();
}

const char *HostHelpers::GetPrefsFilename()
{
    return gstrPrefsFilename.c_str();
}

void HostHelpers::Log(const char *pszFormat, va_list va)
{
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
            pszFormat, va);
}

void HostHelpers::Log(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    Log(pszFormat, va);
    va_end(va);
}

void HostHelpers::MessageBox(const char *pszFormat, va_list va)
{
    char sz[1024];
    vsnprintf(sz, sizeof(sz), pszFormat, va);
    Log("%s", sz);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hostile Takeover",
            sz, NULL);
}

void HostHelpers::Break()
{
    Log("BREAK!!");
}


} // namespace wi
