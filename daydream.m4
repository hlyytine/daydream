# a macro to get the libs/cflags for DayDream
# adapted from libglade.m4

dnl AM_PATH_DAYDREAM([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test to see if DayDream is installed, and define DAYDREAM_CFLAGS, LIBS
dnl
AC_DEFUN(AM_PATH_DAYDREAM,
[dnl
dnl Get the cflags and libraries from the daydream-config script
dnl
AC_ARG_WITH(daydream-config,
[  --with-daydream-config=DAYDREAM_CONFIG  Location of daydream-config],
DAYDREAM_CONFIG="$withval")

if test "x$DAYDREAM_CONFIG" = "x"; then
  AC_PATH_PROG(DAYDREAM_CONFIG, daydream-config, no)
else
  AC_MSG_CHECKING([for daydream-config])
  AC_MSG_RESULT($DAYDREAM_CONFIG)
fi
AC_MSG_CHECKING(for DayDream)
if test "$DAYDREAM_CONFIG" = "no"; then
  AC_MSG_RESULT(no)
  ifelse([$2], , :, [$2])
else
  DAYDREAM_CFLAGS=`$DAYDREAM_CONFIG --cflags`
  DAYDREAM_LIBS=`$DAYDREAM_CONFIG --libs`
  AC_MSG_RESULT(yes)
  ifelse([$1], , :, [$1])
fi
AC_SUBST(DAYDREAM_CFLAGS)
AC_SUBST(DAYDREAM_LIBS)
])
