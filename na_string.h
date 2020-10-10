#ifndef _NADIFF_STRING_H_
#define _NADIFF_STRING_H_

#include <stdbool.h>
#include "types.h"
#include "io.h"

int
get_char(const struct line * l, unsigned idx);

bool
is_char_at_idx(const struct line * l, unsigned idx, char expected);

/*
 * NOTE: This function will increment the current position cur_pos to after the number.
 * But only if we find an actual number.
 */
bool
get_number(const struct line * l, unsigned * cur_pos, unsigned * out_num);

bool
line_starts_with_string(const struct line * l, const char * s);

#endif
