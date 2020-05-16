#ifndef _NADIFF_ALLOC_H_
#define _NADIFF_ALLOC_H_

#include "types.h"

struct diff * alloc_diff(struct diff_array * a);

struct hunk * alloc_hunk(struct hunk_array * a);

struct hunk_line * alloc_hunk_line(struct hunk_line_array * a);

struct render_line * alloc_render_line(struct render_line_array * a);

struct render_line_pair * alloc_render_line_pair(struct render_line_pair_array * a);


#endif
