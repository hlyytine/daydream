AM_CPPFLAGS = -I$(top_srcdir)/main -I$(top_srcdir)/lib \
	      -DFTP_DATA_BOTTOM=40000 -DFTP_DATA_TOP=44999

sbin_PROGRAMS = ddftpd

ddftpd_SOURCES = ddftpd.c ftpcmd.y md5.c popen.c ftp.h ddftp.h
ddftpd_LDADD = @LIBSOCKET@ @LIBNSL@ $(top_builddir)/lib/libddcommon.la
ddftpd_LDFLAGS = -static

EXTRA_DIST = COPYRIGHT
