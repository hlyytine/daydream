AM_CPPFLAGS = -I$(top_srcdir)/lib -DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO \
	      -DOLD_ENVIRON -DENV_HACK -DLOGIN=\"@LOGIN@\" 

sbin_PROGRAMS = ddtelnetd
man_MANS = telnetd.8
EXTRA_DIST = $(man_MANS)

ddtelnetd_SOURCES = authenc.c global.c slc.c state.c sys_term.c telnetd.c \
		    termstat.c utility.c defs.h ext.h \
		    telnetd.h
ddtelnetd_LDADD = $(top_builddir)/lib/libddcommon.la @LIBSOCKET@ @LIBNSL@
ddtelnetd_LDFLAGS = -static
