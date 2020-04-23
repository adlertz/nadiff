static bool
is_binary_file(struct line * l)
{
    if (l->data == NULL)
        return false;

    /* TODO Verify that this is the only format of Binary file output. Also add filename checks */
    unsigned i = 0;
    return is_char_at_idx(l, i++, 'B')
        && is_char_at_idx(l, i++, 'i')
        && is_char_at_idx(l, i++, 'n')
        && is_char_at_idx(l, i++, 'a')
        && is_char_at_idx(l, i++, 'r')
        && is_char_at_idx(l, i++, 'y')
        && is_char_at_idx(l, i++, ' ')
        && is_char_at_idx(l, i++, 'f')
        && is_char_at_idx(l, i++, 'i')
        && is_char_at_idx(l, i++, 'l')
        && is_char_at_idx(l, i++, 'e')
        && is_char_at_idx(l, i++, 's')
        && is_char_at_idx(l, i++, ' ');
}

            l = stdin_read_line();

            /* if binary file then we don't to anything else */
            if (!is_binary_file(l)) {
                d->expect_line_changes = true;
                stdin_reset_cur_line();
            }
 * PRE_/POST_CHANGED_LINE instead.