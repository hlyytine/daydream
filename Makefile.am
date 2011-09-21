SUBDIRS = lib batch bulletins configs data ddftp ddfv ddsnoop ddz \
          display docs doors hydracom main python questionnaire \
	  scripts server telnetd utils 

bin_SCRIPTS = secure.sh

daydreamdir = @prefix@
daydream_SCRIPTS = ddlogin

aclocaldir = $(datadir)/aclocal
aclocal_SCRIPTS = daydream.m4

EXTRA_DIST = daydream.cfg daydream.sh daydream.m4 config.h.in fakeyacc

make-links:
	@for i in batch bulletins configs data display questionnaire; do \
	    if test -h $(DESTDIR)$(prefix)/$$i -o ! -d $(DESTDIR)$(prefix)/$$i; then \
		rm -f $(DESTDIR)$(prefix)/$$i; \
		$(LN_S) $(datadir)/$$i $(DESTDIR)$(prefix)/$$i; \
	    else \
		rm -f $(datadir)/$$i; \
		$(LN_S) $(DESTDIR)$(prefix)/$$i $(datadir)/$$i; \
	    fi \
	done

install-exec-local: make-links
	$(mkinstalldirs) $(DESTDIR)$(prefix)/doors
	$(mkinstalldirs) $(DESTDIR)$(prefix)/logfiles
	$(mkinstalldirs) $(DESTDIR)$(prefix)/scripts
	$(mkinstalldirs) $(DESTDIR)$(prefix)/temp
	$(mkinstalldirs) $(DESTDIR)$(prefix)/users
	$(mkinstalldirs) $(DESTDIR)$(prefix)/utils
	$(INSTALL_PROGRAM) $(top_builddir)/daydream-config $(DESTDIR)$(bindir)/daydream-config
	$(INSTALL_DATA) $(top_builddir)/config.h $(DESTDIR)$(includedir)/config.h

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/aliens
	$(INSTALL_DATA) $(top_srcdir)/daydream.cfg $(DESTDIR)$(prefix)/daydream.cfg

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

