#ifndef _NADIFF_IO_H_
#define _NADIFF_IO_H_

#include <stdint.h>

struct line {
  char * data;
  unsigned len;
  unsigned row;
};

/* Don't free the returned line pointer, also don't use it when stdin_read_line is called again. */
struct line *
stdin_read_line(void);

/* Reset to that we read the previous line again when calling stdin_read_line */
void
stdin_reset_cur_line(void);

#endif
