AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib

bin_PROGRAMS = wall dded faker new setlrp conftop grab whof log

wall_SOURCES = wall.c
dded_SOURCES = dded.c
faker_SOURCES = faker.c
new_SOURCES = new.c
setlrp_SOURCES = setlrp.c
conftop_SOURCES = conftop.c
grab_SOURCES = grab.c
whof_SOURCES = whof.c
log_SOURCES = log.c

wall_LDADD = @LIBSOCKET@ @LIBNSL@
dded_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
faker_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
new_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
setlrp_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
conftop_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
grab_LDADD = $(top_builddir)/lib/libdd.la $(top_builddir)/lib/libddcommon.la \
	     @LIBSOCKET@ @LIBNSL@
whof_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
log_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/doors
	@for i in $(bin_PROGRAMS); do \
	    rm -f $(DESTDIR)$(prefix)/doors/$$i; \
	    $(LN_S) $(DESTDIR)$(bindir)/$$i $(DESTDIR)$(prefix)/doors/$$i; \
	done
