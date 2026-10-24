dnl
dnl Test for libEBML and libMatroska, and define MATROSKA_CFLAGS and MATROSKA_LIBS
dnl

dnl MKVToolNix-Mod: required, from libs/ (there are no internal copies)
EBML_MATROSKA_INTERNAL=no
PKG_CHECK_MODULES([EBML],[libebml >= 1.4.7],[],
  [AC_MSG_ERROR([libEBML 1.4.7 or later not found (libs/libebml, built by build.sh)])])
PKG_CHECK_MODULES([MATROSKA],[libmatroska >= 1.7.2],[],
  [AC_MSG_ERROR([libMatroska 1.7.2 or later not found (libs/libmatroska, built by build.sh)])])

AC_SUBST(EBML_MATROSKA_INTERNAL)
AC_SUBST(EBML_CFLAGS)
AC_SUBST(EBML_LIBS)
AC_SUBST(MATROSKA_CFLAGS)
AC_SUBST(MATROSKA_LIBS)
