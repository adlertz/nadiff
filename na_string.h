#ifndef _NADIFF_STRING_H_
#define _NADIFF_STRING_H_

#include <stdbool.h>
#include "types.h"
#include "io.h"

int
get_char(struct line * l, unsigned idx);

bool
is_char_at_idx(struct line * l, unsigned idx, char expected);

/*
 * NOTE: This function will increment the current position cur_pos to after the number.
 * But only if we find an actual number.
 */
bool
get_number(struct line * l, unsigned * cur_pos, unsigned * out_num);

#endif
