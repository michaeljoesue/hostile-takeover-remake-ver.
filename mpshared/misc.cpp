#include "mpshared/misc.h"
#include <string.h>

namespace wi {

// Zero terminating counted strcpy

void strncpyz(char *pszDst, const char *pszSrc, int cb)
{
    if (cb <= 0)
        return;
	cb--;
	while (cb-- != 0) {
		if ((*pszDst++ = *pszSrc++) == 0)
			return;
	}
	*pszDst = 0;
}

int stricmp(const char *psz1, const char *psz2)
{
	while (true) {
		byte b1 = (byte)*psz1++;
		if (b1 >= 'A' && b1 <= 'Z')
			b1 += 'a' - 'A';
		byte b2 = (byte)*psz2++;
		if (b2 >= 'A' && b2 <= 'Z')
			b2 += 'a' - 'A';		
		if (b1 != b2)
			return b1 - b2;
		if (b1 == 0)
			return 0;
	}
}

} // namespace wi
