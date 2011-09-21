AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib

bin_PROGRAMS = yelld ddcfg conv18to34 ddcallers robowriter ddtop txtdiz \
               stripansi fixbases tosser pcb2ansi listclean ddwho ax2dd \
               msgbaseopt ddftpwho logrot runas
	       
bin_SCRIPTS = zipadder	       
sbin_SCRIPTS = strconv	       

yelld_SOURCES = yelld.c
ddcfg_SOURCES = ddcfg.c
conv18to34_SOURCES = conv18to34.c
ddcallers_SOURCES = ddcallers.c
robowriter_SOURCES = robowriter.c
ddtop_SOURCES = ddtop.c
txtdiz_SOURCES = txtdiz.c
stripansi_SOURCES = stripansi.c
fixbases_SOURCES = fixbases.c
tosser_SOURCES = tosser.c
pcb2ansi_SOURCES = pcb2ansi.c
listclean_SOURCES = listclean.c
ddwho_SOURCES = ddwho.c
ax2dd_SOURCES = ax2dd.c
msgbaseopt_SOURCES = msgbaseopt.c
ddftpwho_SOURCES = ddftpwho.c
logrot_SOURCES = logrot.c
runas_SOURCES = runas.c

yelld_LDADD = @LIBSOCKET@ @LIBNSL@
robowriter_LDADD = $(top_builddir)/lib/libdd.la \
		   $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
fixbases_LDADD = $(top_builddir)/lib/libdd.la \
		 $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
fixbases_LDFLAGS = -static
tosser_LDADD = $(top_builddir)/lib/libdd.la \
	       $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
tosser_LDFLAGS = -static
listclean_LDADD = $(top_builddir)/lib/libdd.la @LIBSOCKET@ @LIBNSL@
msgbaseopt_LDADD = $(top_builddir)/lib/libdd.la \
		   $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
msgbaseopt_LDFLAGS = -static
logrot_LDADD = $(top_builddir)/lib/libddcommon.la
logrot_LDFLAGS = -static
runas_LDADD = $(top_builddir)/lib/libddcommon.la
runas_LDFLAGS = -static

EXTRA_DIST = zipadder strconv

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/utils
	@for i in $(bin_PROGRAMS) $(bin_SCRIPTS); do \
	    rm -f $(DESTDIR)$(prefix)/utils/$$i; \
	    $(LN_S) $(DESTDIR)$(bindir)/$$i $(DESTDIR)$(prefix)/utils/$$i; \
	done
	@for i in $(sbin_SCRIPTS); do \
	    rm -f $(DESTDIR)$(prefix)/utils/$$i; \
	    $(LN_S) $(DESTDIR)$(sbindir)/$$i $(DESTDIR)$(prefix)/utils/$$i; \
	done

