AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib

EXTRA_DIST = 2types.h hydra.h hydracom.h license.doc fmisc.c hydra.c \
	     hydracom.c misc.c unix.c 

bin_PROGRAMS =

if BUILD_HYDRACOM
bin_PROGRAMS += hydracom
endif

hydracom_SOURCES = dos_file.c fmisc.c hydra.c hydracom.c misc.c unix.c 
hydracom_LDADD = @CURSES_LIBS@ $(top_builddir)/lib/libddcommon.la
hydracom_LDFLAGS = -static
