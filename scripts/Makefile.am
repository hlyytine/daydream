sbin_SCRIPTS = cfg2dbinds desc.pl longtoshort.pl clean.pl fix.sh \
	update-2.06-to-2.07

EXTRA_DIST = README cfg2dbinds desc.pl longtoshort.pl clean.pl fix.sh \
	update-2.06-to-2.07

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/scripts
	@for i in $(sbin_SCRIPTS); do \
	    rm -f $(DESTDIR)$(prefix)/scripts/$$i; \
	    $(LN_S) $(DESTDIR)$(sbindir)/$$i $(DESTDIR)$(prefix)/scripts/$$i; \
	done






