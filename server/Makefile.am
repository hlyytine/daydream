AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib -I$(top_srcdir)

EXTRA_DIST = main.c lists.c config.c data.h

bin_PROGRAMS =

if BUILD_SERVER
bin_PROGRAMS += server
endif

server_SOURCES = main.c lists.c config.c
server_LDADD = -L/usr/X11R6/lib -lX11 -lXext -lEZ -lm -ltiff -lpng -ljpeg $(top_builddir)/lib/libddcommon.la
