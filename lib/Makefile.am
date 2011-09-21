AM_CPPFLAGS = -I$(top_srcdir)/main 

noinst_PROGRAMS = testlib

include_HEADERS = ddlib.h ddcommon.h
lib_LTLIBRARIES = libddcommon.la libdd.la

libdd_la_SOURCES = libdd.c ddlib.h
libdd_la_LIBADD = $(top_builddir)/lib/libddcommon.la

libddcommon_la_SOURCES = libddcommon.c ddcommon.h ddutmpx.c ddutmpx.h \
			 deldir.c
libddcommon_la_LIBADD = @LTLIBOBJS@ 

testlib_SOURCES = testlib.c
testlib_LDADD = libdd.la @LIBSOCKET@ @LIBNSL@

