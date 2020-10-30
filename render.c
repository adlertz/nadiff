#include "render.h"
#include "error.h"
#include "vt100.h"
#include "alloc.h"
#include "compare.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static bool redraw = false;
static unsigned diff_idx = 0;
static unsigned diff_start = 0;
static unsigned horizontal_offset = 0;

static unsigned list_visible_start = 0;
static unsigned list_visible_end = 0;

static struct window list_window;
static struct window diff0_window;
static struct window diff1_window;

#define LINE_NBR_WIDTH 5
#define MOVE_DIFF_LINES 5

struct coord {
    int x, y;
};

struct window {
    struct coord tl, br;
};

/* Custom strlen function that takes tabs into consideration */
static size_t
strlen_tabs(char const * data, unsigned len)
{
    size_t space_len = 0;
    for (unsigned i = 0; i < len; ++i) {
        char c = data[i];
        if (c == '\t')
            space_len += 4;
        else
            space_len++;
    }

    return space_len;
}

static void
draw_list(struct diff_array * da, struct window * list)
{
    unsigned list_width = list->br.x - list->tl.x;

    char line[list_width];
    for (unsigned i = 0; i < list_width; ++i)
        line[i] = '-';

    vt100_set_pos(list->tl.x, list->tl.y + 1);
    vt100_write(line, list_width, list_width);

    for (unsigned i = list_visible_start; i < da->size; ++i) {
        struct diff * d = &da->data[i];

        /* Are we outside screen ? */
        if ((i - list_visible_start + 3) > list->br.y)
            break;

        if (diff_idx == i) {
            vt100_set_inverted_colors();
        } else {
            vt100_set_default_colors();
        }

        vt100_set_pos(list->tl.x, list->tl.y + i - list_visible_start + 2);

        if (strcmp(d->short_pre_img_name, d->short_post_img_name) == 0) {
            vt100_write(d->short_pre_img_name, strlen(d->short_pre_img_name), list_width);
        } else {
            char name[200];
            snprintf(name, 200, "%s -> %s", d->short_pre_img_name, d->short_post_img_name);
            vt100_write(name, strlen(name), list_width);
        }
    }

    vt100_set_default_colors();
}

static bool
convert_tabs(char ** data, unsigned * len)
{
    if (*len == 0)
        return true;

    size_t space_len = strlen_tabs(*data, *len);

    /* no tabs found? No need to reallocate line */
    if (space_len == *len)
        return true;

    char * new_data = malloc(sizeof(char) * space_len);

    unsigned si = 0;
    for (unsigned i = 0; i < *len; ++i) {
        if ((*data)[i] == '\t') {
            new_data[si++] = '~';
            new_data[si++] = ' ';
            new_data[si++] = ' ';
            new_data[si++] = ' ';
        } else {
            new_data[si++] = (*data)[i];
        }
    }
    free(*data);

    *data = new_data;
    *len = space_len;

    return true;
}

static void
render_section_name(char * section_name, struct render_line_array * a0,
    struct render_line_array * a1, bool is_first_section)
{
    if (section_name == NULL)
        return;

    unsigned len = strlen(section_name);

    convert_tabs(&section_name, &len);

    /* add some padding before the next section */
    if (!is_first_section) {
        for (unsigned i = 0; i < 2; ++i) {
            struct render_line * l0 = alloc_render_line(a0);
            struct render_line * l1 = alloc_render_line(a1);
            l0->type = l1->type = RENDER_LINE_SPACE;
        }
    }

    struct render_line * l0 = alloc_render_line(a0);
    *l0 = (struct render_line) {
        .type = RENDER_LINE_SECTION_NAME, .data = section_name, .len = len
    };

    struct render_line * l1 = alloc_render_line(a1);
    *l1 = (struct render_line) {
        .type = RENDER_LINE_SECTION_NAME, .data = section_name, .len = len
    };
}

static bool
populate_render_line_arrays(struct diff * d, struct render_line_pair * p)
{
    if (p->is_populated)
        return true;

    enum { STATE_NORMAL, STATE_PRE, STATE_POST } state = STATE_NORMAL;
    struct render_line_array * a0 = &p->a0;
    struct render_line_array * a1 = &p->a1;
    struct hunk_array const * ha = &d->ha;

    for (unsigned i = 0; i < ha->size; ++i) {
        struct hunk * h = &ha->data[i];

        bool is_first_section = i == 0;
        render_section_name(h->section_name, a0, a1, is_first_section);

        struct hunk_line_array const * hla = &h->hla;
        unsigned pre_line_nr = h->pre_line_nr;
        unsigned post_line_nr = h->post_line_nr;
        unsigned num_pre_lines = 0;
        unsigned num_post_lines = 0;

        for (unsigned j = 0; j < hla->size; ++j) {
            struct hunk_line * hl = &hla->data[j];

            convert_tabs(&hl->line, &hl->len);

            struct render_line * l0 = NULL;
            struct render_line * l1 = NULL;

            switch (hl->type) {
            case PRE_LINE:
                l0 = alloc_render_line(a0);
                *l0 = (struct render_line) {
                    .type = RENDER_LINE_PRE,
                    .data = hl->line, .len = hl->len, .line_nr = pre_line_nr++
                };

                state = STATE_PRE;

                num_pre_lines++;

                break;
            case POST_LINE:
                l1 = alloc_render_line(a1);
                *l1 = (struct render_line) {
                    .type = RENDER_LINE_POST,
                    .data = hl->line, .len = hl->len, .line_nr = post_line_nr++
                };

                state = STATE_POST;

                num_post_lines++;

                break;
            default:
                if (state == STATE_PRE) {
                    /* pre -> normal. We should pad with post lines. */
                    if (num_post_lines) {
                        /* Sanity check, we should not have any post lines */
                        na_printf("Should not encounter any post lines\n");
                        return false;
                    }

                    for (unsigned i = 0; i < num_pre_lines; ++i) {
                        l1 = alloc_render_line(a1);
                        l1->type = RENDER_LINE_POST_LINE;
                    }
                } else if (state == STATE_POST) {
                    /* pre -> post -> normal or post -> normal. Should possibly pad either
                     * pre or post lines. */
                    if (num_pre_lines > num_post_lines) {
                        /* More pre than post lines, pad with post lines */
                        for (unsigned p = 0; p < (num_pre_lines - num_post_lines); ++p) {
                            struct render_line *l1 = alloc_render_line(a1);
                            l1->type = RENDER_LINE_POST_LINE;
                        }
                    } else if (num_post_lines > num_pre_lines) {
                        /* more post than pre lines, pad with pre lines */
                        for (unsigned p = 0; p < (num_post_lines - num_pre_lines); ++p) {
                            struct render_line *l0 = alloc_render_line(a0);
                            l0->type = RENDER_LINE_PRE_LINE;
                        }
                    }
                }

                num_pre_lines = 0;
                num_post_lines = 0;
                state = STATE_NORMAL;

                l0 = alloc_render_line(a0);
                *l0 = (struct render_line) {
                    .type = RENDER_LINE_NORMAL,
                    .data = hl->line, .len = hl->len, .line_nr = pre_line_nr++
                };

                l1 = alloc_render_line(a1);
                *l1 = (struct render_line) {
                    .type = RENDER_LINE_NORMAL,
                    .data = hl->line, .len = hl->len, .line_nr = post_line_nr++
                };
            }

            if (l0 && l0->len > p->max_len_a0)
                p->max_len_a0 = l0->len;
            if (l1 && l1->len > p->max_len_a1)
                p->max_len_a1 = l1->len;
        }

        /* It could be that we are ending with a pre or a post instead of a normal.
         * Then we need to make sure we are not missing to add any pad lines */
        if (num_pre_lines || num_post_lines) {
            if (num_pre_lines > num_post_lines) {
                /* more pre than post, pad with post lines. */
                for (unsigned p = 0; p < (num_pre_lines - num_post_lines); ++p) {
                    struct render_line *l1 = alloc_render_line(a1);
                    l1->type = RENDER_LINE_POST_LINE;
                }
            } else if (num_post_lines > num_pre_lines) {
                /* more post than pre, pad with pre lines. */
                for (unsigned p = 0; p < (num_post_lines - num_pre_lines); ++p) {
                    struct render_line *l0 = alloc_render_line(a0);
                    l0->type = RENDER_LINE_PRE_LINE;
                }
            }

        }
    }

    p->is_populated = true;

    return true;
}

static bool
display_line_number(struct render_line * l, char * line, int window_width)
{
    const char * LINE_NUMBER = "%4u";

    switch (l->type) {
    case RENDER_LINE_SPACE:
    case RENDER_LINE_SECTION_NAME:
        return true;
    case RENDER_LINE_NORMAL:
        vt100_set_default_colors();
        snprintf(line, window_width, LINE_NUMBER, l->line_nr);
        break;
    case RENDER_LINE_POST_LINE:
    case RENDER_LINE_PRE_LINE:
        if (l->type == RENDER_LINE_PRE_LINE)
            vt100_set_red_foreground();
        else
            vt100_set_green_foreground();
        snprintf(line, window_width, "   ░");
        break;
    case RENDER_LINE_PRE:
    case RENDER_LINE_POST:
        if (l->type == RENDER_LINE_PRE)
            vt100_set_red_foreground();
        else
            vt100_set_green_foreground();
        snprintf(line, window_width, LINE_NUMBER, l->line_nr);
        break;
    default:
        na_printf("Should not enter here with type: %u\n" , l->type);
        return false;
    }

    vt100_write(line, strlen(line), window_width);

    return true;
}

static bool
display_line(struct render_line * l, int window_width)
{
    switch (l->type) {
    case RENDER_LINE_SPACE:
    case RENDER_LINE_PRE_LINE:
    case RENDER_LINE_POST_LINE:
        return true;
    case RENDER_LINE_SECTION_NAME:
        vt100_set_underline();
        break;
    case RENDER_LINE_NORMAL:
        vt100_set_default_colors();
        break;
    case RENDER_LINE_POST:
        vt100_set_green_foreground();
        break;
    case RENDER_LINE_PRE:
        vt100_set_red_foreground();
        break;
    default:
        na_printf("Should not enter here with type: %u\n" , l->type);
        return false;
    }

    if (horizontal_offset < l->len)
        vt100_write(l->data + horizontal_offset, l->len - horizontal_offset, window_width);

    return true;
}

static bool
draw_windows(struct diff * d, struct window * diff0, struct window * diff1,
    struct render_line_pair * p)
{
    unsigned cur_vt100_diff0_row = diff0->tl.y;
    unsigned cur_vt100_diff1_row = diff1->tl.y;

    unsigned diff0_width = diff0->br.x - diff0->tl.x;
    unsigned diff1_width = diff1->br.x - diff1->tl.x;
    /* draw pre and post names */

    vt100_set_pos(diff0->tl.x, cur_vt100_diff0_row++);
    vt100_write(d->pre_img_name, strlen(d->pre_img_name), diff0_width);

    /* for now let's assume diff0 and diff1 have same width */
    char diff_line[diff0_width];
    for (unsigned i = 0; i < diff0_width; ++i)
        diff_line[i] = '-';

    vt100_set_pos(diff0->tl.x, cur_vt100_diff0_row++);
    vt100_write(diff_line, diff0_width, diff0_width);

    vt100_set_pos(diff1->tl.x, cur_vt100_diff1_row++);
    vt100_write(d->post_img_name, strlen(d->post_img_name), diff1_width);

    vt100_set_pos(diff1->tl.x, cur_vt100_diff1_row++);
    vt100_write(diff_line, diff0_width, diff0_width);

    /* let's display hunks */
    struct render_line_array * a0 = &p->a0;
    struct render_line_array * a1 = &p->a1;

    for (unsigned i = diff_start; i < a0->size; ++i) {
        if (cur_vt100_diff0_row == diff0->br.y)
            break;

        struct render_line * l = &a0->data[i];

        char line[diff0_width];

        vt100_set_pos(diff0->tl.x, cur_vt100_diff0_row);

        try_ret(display_line_number(l, line, diff0_width));

        vt100_set_pos(diff0->tl.x + LINE_NBR_WIDTH, cur_vt100_diff0_row);

        try_ret(display_line(l, diff0_width - LINE_NBR_WIDTH));

        cur_vt100_diff0_row++;

    }

    vt100_set_default_colors();

    for (unsigned i = diff_start; i < a1->size; ++i) {

        if (cur_vt100_diff1_row == diff1->br.y)
            break;

        struct render_line * l = &a1->data[i];

        char line[diff1_width];
        line[diff1_width - 1] = '\n';

        vt100_set_pos(diff1->tl.x, cur_vt100_diff1_row);

        try_ret(display_line_number(l, line, diff1_width));

        vt100_set_pos(diff1->tl.x + LINE_NBR_WIDTH, cur_vt100_diff1_row);

        try_ret(display_line(l, diff1_width - LINE_NBR_WIDTH));

        cur_vt100_diff1_row++;
    }

    vt100_set_default_colors();

    return true;
}

static void
calculate_dimensions(struct vt100_dims * d, struct window * list, struct window * diff0, struct window * diff1)
{
    /* Layout:
     * +-+-----+-----+
     * | |     |     |
     * |A|  B  |  C  |
     * | |     |     |
     * +-+-----+-----+
     *
     * where A is diff list, and B and C are diff0 (pre) and diff1 (post) windows.
     */

    /* calculate sizes of list and code windows */
    unsigned window_max = 200;
    unsigned list_max = 40;
    unsigned screen_width = d->cols - 1;
    unsigned screen_height = d->rows - 1;

    unsigned list_width = MIN(screen_width * 0.1, list_max);

    int x = (screen_width - list_width) / 2;
    unsigned window_width = MIN(x, window_max);

    /* if window is smaller than window max, we could add to list_width if division rounds down */
    if (window_width == x) {
        list_width += (screen_width - list_width) - x * 2;
    }

    *list = (struct window) {
        .tl = { .x = 1, .y = 1 },
        .br = { .x = list_width - 1, .y = screen_height + 1}
    };

    *diff0 = (struct window) {
        .tl = { .x = list->br.x + 1, .y = 1 },
        .br = { .x = list->br.x + window_width + 1, .y = screen_height  + 1}
    };

    *diff1 = (struct window) {
        .tl = { .x = diff0->br.x + 1, .y = 1 },
        .br = { .x = diff0->br.x + window_width + 1, .y = screen_height + 1}
    };
}

static
bool is_terminal_too_small(struct vt100_dims * d)
{
    /* we need at least 10 width list, 20 width diff A and 20 width diff B */
    return d->cols < 101 || d->rows < 21;
}

static bool
update_display(struct diff_array * da, struct render_line_pair_array * pa)
{
    struct vt100_dims dims;
    try_ret(vt100_get_window_size(&dims));

    vt100_clear_screen();

    if (is_terminal_too_small(&dims)) {
        vt100_set_pos(1, 1);
        static const char * const error_msg = "Increase terminal size..";
        vt100_write(error_msg, strlen(error_msg), dims.cols);
        return true;
    }

    calculate_dimensions(&dims, &list_window, &diff0_window, &diff1_window);

    draw_list(da, &list_window);

    struct diff * diff = &da->data[diff_idx];

    struct render_line_pair * p = &pa->data[diff_idx];

    try_ret(populate_render_line_arrays(diff, p));

    try_ret(draw_windows(diff, &diff0_window, &diff1_window, p));

    return true;
}

static bool
enter_loop(int fd, struct diff_array * da, struct render_line_pair_array * pa)
{
    for (;;) {
        enum vt100_key_type key = vt100_read_key(fd);

        struct render_line_pair * p  = &pa->data[diff_idx];

        switch (key) {
        case KEY_TYPE_NONE:
        case KEY_TYPE_UNKNOWN:
            break;
        case KEY_TYPE_ERROR:
            return false;
        case KEY_TYPE_EXIT:
            return true;
        case KEY_TYPE_PREV_DIFF:
            if (diff_idx > 0) {
                diff_idx--;

                diff_start = 0;
                horizontal_offset = 0;

                if (diff_idx < list_visible_start) {
                    list_visible_start = diff_idx;

                    list_visible_end = diff_idx + (list_window.br.y - 3);
                }

                redraw = true;
            }
            break;
        case KEY_TYPE_NEXT_DIFF:
            if (diff_idx < da->size - 1) {
                diff_idx++;

                diff_start = 0;
                horizontal_offset = 0;

                if (diff_idx > list_window.br.y - 3) { // because the list from third row

                    if (diff_idx > list_visible_end)
                        list_visible_end = diff_idx;

                    list_visible_start = list_visible_end - (list_window.br.y - 3);
                }

                redraw = true;
            }
            break;
        case KEY_TYPE_PREV_CHANGE:
        case KEY_TYPE_NEXT_CHANGE:
            break;
        case KEY_TYPE_MOVE_DIFFS_UP:
            if (diff_start > 0) {
                diff_start -= MOVE_DIFF_LINES;
                redraw = true;
            }
            break;
        case KEY_TYPE_MOVE_DIFFS_DOWN:
            /* diff0_window and diff1_window are the same height and a0 and a1 are the same size */
            assert(diff0_window.br.y == diff1_window.br.y);
            assert(p->a0.size == p->a1.size);
            if (diff_start + diff0_window.br.y - 10 < p->a0.size) {
                diff_start += MOVE_DIFF_LINES;
                redraw = true;
            }
            break;
        case KEY_TYPE_MOVE_DIFFS_LEFT:
            if (horizontal_offset > 0) {
                horizontal_offset--;
                redraw = true;
            }
            break;
        case KEY_TYPE_MOVE_DIFFS_RIGHT: {
            unsigned diff0_offs = (diff0_window.br.x - diff0_window.tl.x) - LINE_NBR_WIDTH;
            unsigned diff1_offs = (diff1_window.br.x - diff1_window.tl.x) - LINE_NBR_WIDTH;
            if (horizontal_offset + diff0_offs < p->max_len_a0 ||
                horizontal_offset + diff1_offs < p->max_len_a1) {
                horizontal_offset++;
                redraw = true;
            }
            break;
        }
        }

        if (redraw) {
            redraw = false;
            try_ret(update_display(da, pa));
        }
    }

    return true;
}

static void
catch_window_change_signal(int signo)
{
    if (signo == SIGWINCH) {
        /* terminal resized */
        redraw = true;
    }
}

static void
init_vt100(int fd)
{
    /* this might not work in some terminals */
    vt100_enter_alternate_screen_buffer();

    vt100_enable_raw_mode(fd);

    vt100_hide_cursor();
}

static void
reset_vt100(int fd)
{
    vt100_show_cursor();

    vt100_disable_raw_mode(fd);

    vt100_clear_screen();
    vt100_set_pos(1,1);

    /* this might not work in some terminals */
    vt100_leave_alternate_screen_buffer();
}

bool
render(int fd, struct diff_array * da)
{
    init_vt100(fd);

    signal(SIGWINCH, catch_window_change_signal);

    /* pre allocate array data */
    struct render_line_pair_array pa = {0};
    for (unsigned i = 0; i < da->size; ++i)
        alloc_render_line_pair(&pa);

    if (!update_display(da, &pa)) {
        reset_vt100(fd);
        return false;
    }

    if (!enter_loop(fd, da, &pa)) {
        reset_vt100(fd);
        return false;
    }

    reset_vt100(fd);
    return true;
}
