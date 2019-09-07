#ifndef _NADIFF_ERROR_H
#define _NADIFF_ERROR_H

#include <stdio.h>

#define try_ret(x)          \
    do {                    \
        bool success = (x); \
        if (!success)       \
            return false;   \
    } while (0);

#define try_ret_int(x)      \
    do {                    \
        int success = (x);  \
        if (success < 0)    \
            return false;   \
    } while (0);

#define na_printf(fmt, ...) fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#endif
