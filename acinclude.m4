dnl @synopsis AC_C_BOUNDED
dnl
dnl The OpenBSD project has added a new function attribute 'bounded' to
dnl GCC. Despite its usefulness, it has not been migrated into the
dnl official GCC. The compiler warns about unknown attribute directives
dnl even when invoked with no warning flags. Users may find these
dnl warnings alarming while they should be considered as informational
dnl only.
dnl
dnl This macro defines a C preprocessor symbol HAVE_BOUNDED if the 
dnl compiler supports the attribute 'bounded'.
dnl
dnl You may want to get yourself a copy of OpenBSD gcc-local manual page.
dnl It is available at http://www.openbsd.org.
dnl
dnl @version 1.0
dnl @author Hannu Lyytinen <hlyytine@cs.uku.fi>
dnl
AC_DEFUN([AC_C_BOUNDED], [
  AC_CACHE_CHECK([whether C compiler supports attribute bounded],
    ac_cv_c_bounded, [
      ac_cv_c_bounded=no
      if test "x$GCC" = "xyes"; then
        echo 'void func(char *, int) __attribute__ ((__bounded__ (__buffer__, 1, 2)));' > conftest.c
        $CC -c conftest.c 2>conftest.err && {
          grep 'attribute directive ignored' conftest.err >/dev/null ||
            ac_cv_c_bounded=yes
        }
        rm -f conftest.c conftest.o conftest.err
      fi
    ])
  if test "x$ac_cv_c_bounded" = "xyes"; then
    AC_DEFINE(HAVE_BOUNDED, ,
      [Define this if your compiler supports the function attribute 'bounded'.])
  fi
])

dnl @synopsis AC_PROG_CC_WARNINGS([ANSI])
dnl
dnl Enables a reasonable set of warnings for the C compiler.  Optionally,
dnl if the first argument is nonempty, turns on flags which enforce and/or
dnl enable proper ANSI C if such are known with the compiler used.
dnl
dnl Currently this macro knows about GCC, Solaris C compiler,
dnl Digital Unix C compiler, C for AIX Compiler, HP-UX C compiler,
dnl IRIX C compiler, NEC SX-5 (Super-UX 10) C compiler, and Cray J90
dnl (Unicos 10.0.0.8) C compiler.
dnl
dnl @version 1.2
dnl @author Ville Laurikari <vl@iki.fi>
dnl
AC_DEFUN([AC_PROG_CC_WARNINGS], [
  ansi=$1
  if test -z "$ansi"; then
    msg="for C compiler warning flags"
  else
    msg="for C compiler warning and ANSI conformance flags"
  fi
  AC_CACHE_CHECK($msg, ac_cv_prog_cc_warnings, [
    if test -n "$CC"; then
      cat > conftest.c <<EOF
int main(int argc, char **argv) { return 0; }
EOF

      dnl GCC
      if test "$GCC" = "yes"; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-Wall"
        else
          ac_cv_prog_cc_warnings="-Wall -ansi -pedantic"
        fi

      dnl Solaris C compiler
      elif $CC -flags 2>&1 | grep "Xc.*strict ANSI C" > /dev/null 2>&1 &&
           $CC -c -v -Xc conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-v"
        else
          ac_cv_prog_cc_warnings="-v -Xc"
        fi

      dnl HP-UX C compiler
      elif $CC > /dev/null 2>&1 &&
           $CC -c -Aa +w1 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="+w1"
        else
          ac_cv_prog_cc_warnings="+w1 -Aa"
        fi

      dnl Digital Unix C compiler
      elif $CC -c -verbose -w0 -warnprotos -std1 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-verbose -w0 -warnprotos"
        else
          ac_cv_prog_cc_warnings="-verbose -w0 -warnprotos -std1"
        fi

      dnl C for AIX Compiler
      elif $CC 2>&1 | grep AIX > /dev/null 2>&1 &&
           $CC -c -qlanglvl=ansi -qinfo=all conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd"
        else
          ac_cv_prog_cc_warnings="-qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd -qlanglvl=ansi"
        fi

      dnl IRIX C compiler
      elif $CC -c -fullwarn -ansi -ansiE conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-fullwarn"
        else
          ac_cv_prog_cc_warnings="-fullwarn -ansi -ansiE"
        fi

      dnl The NEC SX-5 (Super-UX 10) C compiler
      elif $CC -c -pvctl[,]fullmsg -Xc conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-pvctl[,]fullmsg"
        else
          ac_cv_prog_cc_warnings="-pvctl[,]fullmsg -Xc"
        fi

      dnl The Cray J90 (Unicos 10.0.0.8) C compiler
      elif $CC -c -h msglevel 2 conftest.c > /dev/null 2>&1 &&
           test -f conftest.o; then
        if test -z "$ansi"; then
          ac_cv_prog_cc_warnings="-h msglevel 2"
        else
          ac_cv_prog_cc_warnings="-h msglevel 2 -h conform"
        fi

      fi
      rm -f conftest.*
    fi
    if test -n "$ac_cv_prog_cc_warnings"; then
      CFLAGS="$CFLAGS $ac_cv_prog_cc_warnings"
    else
      ac_cv_prog_cc_warnings="unknown"
    fi
  ])
])

