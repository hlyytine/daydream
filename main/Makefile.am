AM_CPPFLAGS = -I$(top_srcdir)/lib

bin_PROGRAMS = daydream
include_HEADERS = dd.h

daydream_SOURCES = access.c bulletin.c conference.c console.c console.h \
		   daydream.c daydream.h dd.h default.c display.c domenu.c \
		   doorport.c download.c edituser.c enterbbs.c entermsg.c \
		   filelist.c ftpul.c global.h goodbye.c hdr.h hotkey.c \
		   joinconf.c linechat.c lineed.c lists.c mailscan.c md5.c \
		   md5.h menucmd.h menus.c misc.c movefile.c newuser.c olm.c \
		   page.c prompt.c readmsgs.c replymsg.c stats.c stdiohan.c \
		   strf.h symtab.c symtab.h tagedit.c textsearch.c \
		   typetext.c unix.c upload.c user.c usered.c userlist.c \
		   utility.c utility.h version.c viewfile.c wfc.c \
		   who.c 

daydream_LDADD = $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
daydream_LDFLAGS = -static

install-exec-local:
	rm -f $(DESTDIR)$(prefix)/daydream
	$(LN_S) $(DESTDIR)$(bindir)/daydream $(DESTDIR)$(prefix)/daydream
