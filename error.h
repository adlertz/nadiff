#ifndef _NADIFF_ERROR_H_
#define _NADIFF_ERROR_H_

#include <stdio.h>

#define try_ret(x)          \
    do {                    \
        bool success = (x); \
        if (!success)       \
            return false;   \
    } while (0);

#endif
