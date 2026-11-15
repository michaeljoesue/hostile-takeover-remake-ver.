#include "base/format.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string>

namespace base {

static std::string s_str;

const char *Format::ToString(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char sz[512];
    vsnprintf(sz, sizeof(sz), fmt, va);
    va_end(va);
    s_str = sz;
    const char *psz = s_str.c_str();
    return psz;
}

} // namespace base
