AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib

bin_PROGRAMS = ddfv

ddfv_SOURCES = common.cc direntry.cc dirlist.cc entrypack.cc extract.cc \
               ddfv.cc filelist.cc libddc++.cc lightbar.cc listentry.cc \
	       listviewer.cc parser.cc common.h entrypack.h libddc++.h \
	       lightbar.h listviewer.h

ddfv_LDADD = $(top_builddir)/lib/libdd.la $(top_builddir)/lib/libddcommon.la \
	     @LIBSOCKET@ @LIBNSL@
