AC_DEFUN([AX_GTEST],[
  GTEST_CFLAGS=
  GTEST_LIBS=
  GTEST_TYPE=no

  CPPFLAGS_SAVED="$CPPFLAGS"
  AC_LANG_PUSH(C++)

  PKG_CHECK_EXISTS([gtest],[gtest_found=yes],[gtest_found=no])

  if test x"$gtest_found" = xyes; then
    PKG_CHECK_MODULES([gtest],[gtest],[gtest_found=yes])
    GTEST_CFLAGS="`$PKG_CONFIG --cflags gtest`"
    GTEST_LIBS="`$PKG_CONFIG --libs gtest`"
    GTEST_TYPE=system

  else
    dnl MKVToolNix-Mod: required (the unit tests), from libs/
    AC_MSG_ERROR([GoogleTest not found (libs/googletest, built by build.sh)])
  fi

  AC_LANG_POP
  CPPFLAGS="$CPPFLAGS_SAVED"

  AC_SUBST(GTEST_CFLAGS)
  AC_SUBST(GTEST_LIBS)
  AC_SUBST(GTEST_TYPE)
])

AX_GTEST
AC_PATH_PROG(VALGRIND, valgrind,, $PATH)
