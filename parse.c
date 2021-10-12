#include "parse.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "alloc.h"
#include "io.h"
#include "error.h"
#include "na_string.h"

static bool
is_hunk_header(const struct line * l)
{
    /* Format is @@ -<NUM>,<NUM> +<NUM>,<NUM> @@ */
    return is_char_at_idx(l, 0, '@') && is_char_at_idx(l, 1, '@');
}

static bool
is_diff_header(const struct line * l)
{
    /* Format is diff --git <filename> <filename> */
    return line_starts_with_string(l, "diff --git ");
}

static bool
read_old_mode(const struct line * l)
{
    return line_starts_with_string(l, "old mode");
}

static bool
read_new_mode(const struct line * l)
{
    /* TODO also look at the mode */
    return line_starts_with_string(l, "new mode");
}

static bool
read_copy_from(const struct line * l)
{
    return line_starts_with_string(l, "copy from");
}

static bool
read_copy_to(const struct line * l)
{
    return line_starts_with_string(l, "copy to");
}

static bool
read_index_line(const struct line * l)
{
    return line_starts_with_string(l, "index");
}

static bool
is_post_image_add(const struct line * l)
{
    return is_char_at_idx(l, 0, '+');
}

static int
is_pre_image_add(const struct line * l)
{
    return is_char_at_idx(l, 0, '-');
}

static bool
is_extended_header_new_line(const struct line * l)
{
    return line_starts_with_string(l, "new file");
}

static bool
read_delete_line(const struct line * l)
{
    return line_starts_with_string(l, "delete");
}

static bool
is_similarity_index_line(const struct line * l)
{
    return line_starts_with_string(l, "similarity index");
}

static bool
is_rename_from_line(const struct line * l)
{
    return line_starts_with_string(l, "rename from");
}

static bool
is_rename_to_line(const struct line * l)
{
    return line_starts_with_string(l, "rename to");
}

static bool
is_dissimiliarity_index_line(const struct line * l)
{
    return line_starts_with_string(l, "dissimilarity index");
}

static bool
is_binary_file(const struct line * l)
{
    /* TODO Verify that this is the only format of Binary file output. Also add filename checks */
    return line_starts_with_string(l, "Binary files ");
}

/*
 * One more more extended header lines:
 * old mode <mode>
 * new mode <mode>
 * deleted file mode <mode>
 * new file mode <mode>
 * copy from <path>
 * copy to <path>
 * rename from <path>
 * rename to <path>
 * similarity index <number>
 * dissimilarity index <number>
 * index <hash>..<hash> <mode>
 */
static bool
read_extended_header_lines(struct diff * d)
{
    d->status = DIFF_STATUS_CHANGED;
    for (;;) {
        struct line * l = stdin_read_line();
        if (read_old_mode(l)) {
            /* if old mode, we also expect new mode */
            l = stdin_read_line();
            if (!read_new_mode(l)) {
                na_printf("Expected new mode header line at line %u\n", l->row);
                return false;
            }
        } else if (read_copy_from(l)) {
            /* if copy from then we expect copy to */
            l = stdin_read_line();
            if (!read_copy_to(l)) {
                na_printf("Expected copy to header line at line %u\n", l->row);
                return false;
            }
        } else if (is_extended_header_new_line(l)) {
            d->status = DIFF_STATUS_NEW;
        } else if (read_delete_line(l)) {
            d->status = DIFF_STATUS_DELETED;
        } else if (is_similarity_index_line(l) || is_dissimiliarity_index_line(l)) {
            /* expect rename from, and rename to */
            l = stdin_read_line();
            if (!is_rename_from_line(l)) {
                na_printf("Expected rename from line at line %u\n", l->row);
                return false;
            }
            l = stdin_read_line();
            if (!is_rename_to_line(l)) {
                na_printf("Expected rename to line at line %u\n", l->row);
                return false;
            }
        } else if (read_index_line(l)) {
            /* expect this extended header to be last */
            l = stdin_read_line();

            /* if binary file then we don't to anything else */
            if (!is_binary_file(l)) {

                /* there are files that have no data in them */
                if (!is_diff_header(l) && l->data != NULL)
                    d->expect_line_changes = true;

                stdin_reset_cur_line();
            }
            return true;
        } else {
            /* found a line which is not an extended header line */
            stdin_reset_cur_line();
            return true;
        }
    }
}

static bool
read_pre_img_line(struct line *l)
{
    if (l->data == NULL)
        return false;

    /* TODO assert that if diff is new, then --- /dev/null as pre file */
    return true;
}

static bool
read_post_img_line(struct line *l)
{
    if (l->data == NULL)
        return false;

    /* TODO assert that if diff is deleted, then +++ /dev/null as post file */
    return true;
}


/*
 * NOTE: This function will exit program if allocation fails.
 * 'len' should include space for '\0' in the end.
 */
static char *
allocate_string(const struct line * l, unsigned offset, unsigned len)
{
    char * name = malloc(sizeof(char) * len);
    if (name == NULL) {
        na_printf("malloc failed when using line at line %u.\n", l->row);
        exit(EXIT_FAILURE);
    }

    memcpy(name, l->data + offset, len);
    /* most strings we copy end with '\0' but some don't, always add '\0' as the final character */
    name[len - 1] = '\0';
    return name;
}

/*
 * It is in the format @@ from-file-range to-file-range @@ [header].
 * The from-file-range is in the form -<start line>,<number of lines>, and to-file-range is
 * +<start line>,<number of lines>.
 * Both start-line and number-of-lines refer to position and length of hunk in preimage
 * and postimage, respectively.
 * If number-of-lines not shown it means that it is 0.
 */
static bool
set_hunk_header(struct hunk * h, const struct line * l)
{
    /* @@ -1,8 +1 @@ */
    unsigned i = 0;
    try_ret(is_char_at_idx(l, i++, '@'));
    try_ret(is_char_at_idx(l, i++, '@'));
    try_ret(is_char_at_idx(l, i++, ' '));

    try_ret(is_char_at_idx(l, i++, '-'));
    unsigned a_line_nr;
    try_ret(get_number(l, &i, &a_line_nr));
    /* this part is optional */
    unsigned a_num_lines = 0;
    if (get_char(l, i) == ',') {
        try_ret(is_char_at_idx(l, i++, ','));
        try_ret(get_number(l, &i, &a_num_lines));
    }
    try_ret(is_char_at_idx(l, i++, ' '));

    try_ret(is_char_at_idx(l, i++, '+'));
    unsigned b_line_nr;
    try_ret(get_number(l, &i, &b_line_nr));
    /* this part is optional */
    unsigned b_num_lines = 0;
    if (get_char(l, i) == ',') {
        try_ret(is_char_at_idx(l, i++, ','));
        try_ret(get_number(l, &i, &b_num_lines));
    }
    /* end optional part */

    try_ret(is_char_at_idx(l, i++, ' '));
    try_ret(is_char_at_idx(l, i++, '@'));
    try_ret(is_char_at_idx(l, i++, '@'));

    *h = (struct hunk) {
        .pre_line_nr = a_line_nr,
        .pre_num_lines = a_num_lines,
        .post_line_nr = b_line_nr,
        .post_num_lines = b_num_lines,
        .hla = {0},
        .section_name = NULL,
    };

    /* get optional section name */
    if (i + 1 < l->len) {
        try_ret(l->data[i++] == ' ');
        h->section_name = allocate_string(l, i, l->len - i);
    }

    return true;
}

static char *
find_short_name(char * name, unsigned len)
{
    /* go backwards through name to (potentially) find last '/' */
    char * short_name = name;
    for (int i = len - 1; i > 0; i--) {
        if (name[i] == '/') {
            short_name = &name[i+1];
            break;
        }
    }
    return short_name;
}

static bool
set_diff_header(struct diff * d, struct line * l)
{
    /* we only accept git diff -p, where -p is default */
    const char * diff_header_start = "diff --git ";
    unsigned i = 0;
    for (; i < strlen(diff_header_start); ++i)
        try_ret(is_char_at_idx(l, i, diff_header_start[i]));

    /* find pre image name and post image name */
    unsigned start_pos = i;
    unsigned cur_pos = start_pos;

    /* iterate until we find space */
    while (true) {
        int ch = get_char(l, cur_pos);
        if (ch < 0)
            return false;

        if (ch == ' ')
            break;

        cur_pos++;
    }

    /* Add one for missing '\0' */
    unsigned pre_img_size = cur_pos - start_pos + 1;
    char * pre_img_name = allocate_string(l, start_pos, pre_img_size);

    /* move to character after space */
    cur_pos++;

    unsigned post_img_size = l->len - cur_pos;
    char * post_img_name = allocate_string(l, cur_pos, post_img_size);

    *d = (struct diff) {
        .ha = {0},
        .pre_img_name = pre_img_name,
        .post_img_name = post_img_name,
        .short_pre_img_name = find_short_name(pre_img_name, pre_img_size),
        .short_post_img_name = find_short_name(post_img_name, post_img_size)
    };

    return true;
}

static char *
extract_and_allocate_code_line(struct line * l)
{
    /* discard first char: '+', '-' or ' ' from size and start */
    unsigned len = l->len - 1;
    return allocate_string(l, 1, len);
}

static enum hunk_line_type get_hunk_line_type(struct line * l)
{
    if (is_pre_image_add(l))
        return PRE_LINE;
    else if (is_post_image_add(l))
        return POST_LINE;
    else
        return NEUTRAL_LINE;
}

static bool
read_hunk_line(struct line * l, struct hunk * h)
{
    enum hunk_line_type lt = get_hunk_line_type(l);

    struct hunk_line * hl = alloc_hunk_line(&h->hla);

    char * code = NULL;
    unsigned len = 0;
    if (l->len > 1) {
        /* if not empty line. 1 char is for initial hunk line type '+', '-' or ' '*/
        code = extract_and_allocate_code_line(l);
        len = strlen(code);
    }

    *hl = (struct hunk_line) { .line = code, .len = len, .type = lt };

    return true;
}


static bool
parse_start(struct diff_array * da)
{
    enum { STATE_EXPECT_DIFF, STATE_EXPECT_HUNK, STATE_ACCEPT_ALL } state = STATE_EXPECT_DIFF;

    struct diff * d = NULL;
    struct hunk * h = NULL;
    struct line * l = NULL;

    for (;;) {
        l = stdin_read_line();

        switch (state) {
        case STATE_EXPECT_DIFF:
            if (!is_diff_header(l)) {
                na_printf("Expected diff header at line %u\n", l->row);
                return false;
            }

            d = alloc_diff(da);
            if (!set_diff_header(d, l)) {
                na_printf("Could not set diff header at line %u\n", l->row);
                return false;
            }

            try_ret(read_extended_header_lines(d));

            if (!d->expect_line_changes) {
                l = stdin_read_line();

                if (l->data == NULL)
                    return true;

                stdin_reset_cur_line();
                /* Goto next diff */
            } else {
                l = stdin_read_line();
                if (!read_pre_img_line(l)) {
                    na_printf("Could not parse pre image line at line %u \n", l->row);
                    return false;
                }

                l = stdin_read_line();
                if (!read_post_img_line(l)) {
                    na_printf("Could not parse post image line at line %u \n", l->row);
                    return false;
                }
                state = STATE_EXPECT_HUNK;
            }

            break;

        case STATE_EXPECT_HUNK:
            if (!is_hunk_header(l)) {
                na_printf("Expected hunk header at line %u\n", l->row);
                return false;
            }

            h = alloc_hunk(&d->ha);

            if (!set_hunk_header(h, l)) {
                na_printf("Failed to set hunk header at line %u\n", l->row);
                return false;
            }

            l = stdin_read_line();
            try_ret(read_hunk_line(l, h));

            state = STATE_ACCEPT_ALL;

            break;

        case STATE_ACCEPT_ALL:
            if (l->data == NULL)
                return true;

            if (is_diff_header(l)) {
                state = STATE_EXPECT_DIFF;
                stdin_reset_cur_line();
            } else if (is_hunk_header(l)) {
                state = STATE_EXPECT_HUNK;
                stdin_reset_cur_line();
            } else {
                try_ret(read_hunk_line(l, h));
            }

            break;
        }
    }

    return true;
}

bool
parse_stdin(struct diff_array * da)
{
    return parse_start(da);
}
