dnl
dnl Check for pugixml
dnl

PKG_CHECK_EXISTS([pugixml],[pugixml_found=yes],[pugixml_found=no])

if test x"$pugixml_found" = xyes; then
  PKG_CHECK_MODULES([PUGIXML],[pugixml],[pugixml_found=yes],[pugixml_found=no])
fi

if test x"$pugixml_found" = xno; then
  AC_LANG_PUSH(C++)

  AC_CHECK_HEADERS([pugixml.hpp])

  if test x"$ac_cv_header_pugixml_hpp" = xyes; then
    AC_CHECK_LIB([pugixml], [main], [
      PUGIXML_LIBS=-lpugixml
      pugixml_found=yes
    ])
  fi

  AC_LANG_POP
fi

dnl MKVToolNix-Mod: required, from libs/ (there is no internal copy)
if test $pugixml_found != yes; then
  AC_MSG_ERROR([pugixml not found (libs/pugixml, built by build.sh)])
fi
PUGIXML_INTERNAL=no

AC_SUBST(PUGIXML_INTERNAL)
