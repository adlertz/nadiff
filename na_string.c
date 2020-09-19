#include "na_string.h"

#include "error.h"

int
get_char(struct line * l, unsigned idx)
{
    if (idx >= l->len)
        return -1;

    return l->data[idx];
}

bool
is_char_at_idx(struct line * l, unsigned idx, char expected)
{
    if (idx >= l->len)
        return false;

    return l->data[idx] == expected;
}

bool
get_number(struct line * l, unsigned * cur_pos, unsigned * out_num)
{
    unsigned num = 0;
    unsigned pos = *cur_pos;

    while (true) {
        int digit = get_char(l, pos);
        if (digit < 0) {
            na_printf("Failed to parse number at row %u\n", l->row);
            return false;
        }

        /* if not a digit */
        if (digit < '0' || digit > '9')
            break;

        unsigned n = digit - '0';
        num = num * 10 + n;
        pos++;
    }


    *cur_pos = pos;
    *out_num = num;

    return true;
}

