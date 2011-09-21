batchdir = $(datadir)/batch
batch_SCRIPTS = batch1.logoff

EXTRA_DIST = batch1.logoff

install-data-local:
	rm -f $(DESTDIR)$(batchdir)/batch2.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch2.logoff
	rm -f $(DESTDIR)$(batchdir)/batch3.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch3.logoff
	rm -f $(DESTDIR)$(batchdir)/batch4.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch4.logoff
	rm -f $(DESTDIR)$(batchdir)/batch10.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch10.logoff
	rm -f $(DESTDIR)$(batchdir)/batch11.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch11.logoff
	rm -f $(DESTDIR)$(batchdir)/batch12.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch12.logoff
	rm -f $(DESTDIR)$(batchdir)/batch13.logoff
	$(LN_S) batch1.logoff $(DESTDIR)$(batchdir)/batch13.logoff
