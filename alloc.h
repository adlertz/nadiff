#ifndef _NADIFF_ALLOC_H_
#define _NADIFF_ALLOC_H_

#include "types.h"

struct diff * allocate_diff(struct diff_array * a);

struct hunk * allocate_hunk(struct hunk_array * a);

struct hunk_line * allocate_hunk_line(struct hunk_line_array * a);

struct render_line * allocate_render_line(struct render_line_array * a);

struct render_line_pair * allocate_render_line_pair(struct render_line_pair_array * a);


#endif
