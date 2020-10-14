#ifndef _NADIFF_TYPES_H_
#define _NADIFF_TYPES_H_

#include <stdbool.h>

// TODO create macro of arrays and alloc functions

struct uint_array {
    unsigned * data;
    unsigned size;
    unsigned cap;
};

struct diff_array {
    struct diff * data;
    unsigned size;
    unsigned cap;
};

struct hunk_array {
    struct hunk * data;
    unsigned size;
    unsigned cap;
};

struct hunk_line_array {
    struct hunk_line * data;
    unsigned size;
    unsigned cap;
};

enum hunk_line_type {
    PRE_LINE,
    POST_LINE,
    NEUTRAL_LINE,
};

struct hunk_line {
    char * line;
    unsigned len;
    enum hunk_line_type type;
};

struct hunk {
    unsigned pre_line_nr;
    unsigned post_line_nr;
    unsigned pre_num_lines;
    unsigned post_num_lines;

    char * section_name;

    struct hunk_line_array hla;
};

/*
 * Does this diff represent a new, deleted or changed file?
 */
enum diff_status {
    DIFF_STATUS_CHANGED,
    DIFF_STATUS_NEW,
    DIFF_STATUS_DELETED
};

struct diff {
    struct hunk_array ha;
    char const * pre_img_name;
    char const * post_img_name;

    /* pre and post img names withouth potential a/ b/ and full path */
    char const * short_pre_img_name;
    char const * short_post_img_name;

    enum diff_status status;

    /* some diffs contain only renames or mode changes */
    bool expect_line_changes;
};


enum render_line_type {
    RENDER_LINE_PRE,
    RENDER_LINE_POST,
    RENDER_LINE_NORMAL,
    RENDER_LINE_SECTION_NAME,
    RENDER_LINE_PRE_LINE,
    RENDER_LINE_POST_LINE,
    RENDER_LINE_SPACE,
};

struct render_line {
    enum render_line_type type;
    char * data; /* probably just pointing to data inside a hunk_line */
    unsigned len;
    unsigned line_nr;
};

struct render_line_array {
    struct render_line * data;
    unsigned size;
    unsigned cap;
};

struct render_line_pair {
    bool is_populated;
    struct render_line_array a0;
    struct render_line_array a1;

    unsigned len_a0;
    unsigned len_a1;
};

struct render_line_pair_array {
    struct render_line_pair * data;
    unsigned size;
    unsigned cap;
};


#endif
