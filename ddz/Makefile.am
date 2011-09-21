AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib

bin_PROGRAMS = ddrz ddsz

ddrz_SOURCES = ddrz.c timing.c zm.c crctab.c rbsb.c zm.h crctab.h rbsb.h
ddrz_CPPFLAGS = -DDDRZ $(AM_CPPFLAGS)

ddsz_SOURCES = ddsz.c timing.c zm.c crctab.c rbsb.c zm.h crctab.h rbsb.h
ddsz_CPPFLAGS = -DDDSZ $(AM_CPPFLAGS)
ddsz_LDADD = $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
ddsz_LDFLAGS = -static

EXTRA_DIST = TODO zmodem.h ddrz.c ddsz.c crctab.c rbsb.c zm.c \
	     timing.c

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/utils
	@for i in $(bin_PROGRAMS) $(bin_SCRIPTS); do \
	    rm -f $(DESTDIR)$(prefix)/utils/$$i; \
	    $(LN_S) $(DESTDIR)$(bindir)/$$i $(DESTDIR)$(prefix)/utils/$$i; \
	done
