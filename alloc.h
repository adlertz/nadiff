#ifndef _NADIFF_ALLOC_H_
#define _NADIFF_ALLOC_H_

#define START_CAP_ITEMS 16
#define GROWTH_FACTOR 2

#include "types.h"

#include <stddef.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

static bool
cap_grow(void ** data, unsigned sz_elem, unsigned size, unsigned * cap) {
    if (*data == NULL) {
        *cap = START_CAP_ITEMS;
        *data = malloc(*cap * sz_elem);
    } else if (size > *cap) {
        *cap = *cap * GROWTH_FACTOR;
        *data = realloc(*data, *cap * sz_elem);
    }

    return data != NULL;
}

static struct diff *
allocate_diff(struct diff_array * a)
{
    if (!cap_grow(&a->data, sizeof(struct diff), a->size + 1, &a->cap))
        return NULL;
    struct diff * n = &a->data[a->size++];
    *n = (struct diff) {0};
    return n;
}

static struct hunk *
allocate_hunk(struct hunk_array * a)
{
    if (!cap_grow(&a->data, sizeof(struct hunk), a->size + 1, &a->cap))
        return NULL;
    struct hunk * c = &a->data[a->size++];
    *c = (struct hunk) {0};
    return c;
}

static struct hunk_line *
allocate_hunk_line(struct hunk_line_array * a)
{
    if (!cap_grow(&a->data, sizeof(struct hunk_line), a->size + 1, &a->cap))
        return NULL;
    struct hunk_line * n = &a->data[a->size++];
    *n = (struct hunk_line) {0};
    return n;
}

static struct render_line *
allocate_render_line(struct render_line_array * a)
{
    if (!cap_grow(&a->data, sizeof(struct render_line), a->size + 1, &a->cap))
        return NULL;
    struct render_line * n = &a->data[a->size++];
    *n = (struct render_line) {0};
    return n;
}

static struct render_line_pair *
allocate_render_line_pair(struct render_line_pair_array * a)
{
    if (!cap_grow(&a->data, sizeof(struct render_line_pair), a->size + 1, &a->cap))
        return NULL;
    struct render_line_pair * n = &a->data[a->size++];
    *n = (struct render_line_pair) {0};
    return n;
}

#pragma GCC diagnostic pop

#endif
