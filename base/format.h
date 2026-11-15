#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "inc/basictypes.h"

namespace base {

class Format {
public:
    static const char *ToString(const char *fmt, ...);

private:
};

} // namespace base

#endif // __FORMAT_H__
