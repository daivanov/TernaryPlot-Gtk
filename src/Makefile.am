bin_PROGRAMS = ternaryplot

ternaryplot_LDADD = @DEPS_LIBS@
INCLUDES = @DEPS_CFLAGS@

AM_CFLAGS = -Wall -Wextra

ternaryplot_SOURCES = \
    main.c \
    ternaryplot.h ternaryplot.c

EXTRA_DIST = \
    ternaryplot-marshallers.list

BUILT_SOURCES = \
    ternaryplot-marshallers.h \
    ternaryplot-marshallers.c

nodist_ternaryplot_SOURCES = $(BUILT_SOURCES)

ternaryplot-marshallers.c : ternaryplot-marshallers.list ternaryplot-marshallers.h
	@GLIB_GENMARSHAL@ --body --prefix=ternaryplot_marshal $< > $@

ternaryplot-marshallers.h: ternaryplot-marshallers.list
	@GLIB_GENMARSHAL@ --header --prefix=ternaryplot_marshal $< > $@

CLEANFILES = $(BUILT_SOURCES)

