SUBDIRS = examples

EXTRA_DIST = dd.py _dd.c

pythondir = @python_libpath@
AM_CPPFLAGS = -I@python_incpath@ -I$(top_srcdir)/main -I$(top_srcdir)/lib

python_LTLIBRARIES = 

if BUILD_PYTHON
python_LTLIBRARIES += libddpython.la
endif

libddpython_la_SOURCES = _dd.c
libddpython_la_LIBADD = $(top_builddir)/lib/libdd.la

