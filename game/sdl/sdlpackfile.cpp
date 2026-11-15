#include "game/sdl/sdlpackfile.h"
#include "game/mempdbreader.h"
#include "game/ht.h"
#include <string>

namespace wi {

PdbReader *SdlPackFileReader::OpenPdb(const char *pszDir,
        const char *pszFn) {
	// Load data from this directory

    std::string strPath;
    if (pszDir != NULL) {
        strPath = std::string(pszDir) + "\\" + pszFn;
    } else {
        strPath = pszFn;
    }

	// PdbReader over memory
	MemPdbReader *ppdbReader = new MemPdbReader;
	if (!ppdbReader->Open(strPath.c_str())) {
		delete ppdbReader;
        Trace("MemPdbReader::Open(%s) failed", pszFn);
		return NULL;
	}

    return ppdbReader;
}

} // namespace wi
