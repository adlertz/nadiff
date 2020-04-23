.RECIPEPREFIX = >

# paths
PREFIX = /usr/local

src = $(wildcard *.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)  # one dependency file for each source

CFLAGS = -g

nadiff: $(obj)
> $(CC) -o $@ $^

-include $(dep)   # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
> @$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

install: nadiff
> mkdir -p ${PREFIX}/bin
> cp -f nadiff git-nadiff ${PREFIX}/bin
> chmod 755 ${PREFIX}/bin/nadiff

uninstall:
> rm -f $(addprefix ${PREFIX}/bin/, nadiff git-nadiff)

.PHONY: clean
clean:
> rm -f *.o *.d nadiff
