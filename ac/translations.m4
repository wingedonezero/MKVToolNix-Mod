dnl
dnl gettext support (the programs' translation calls)
dnl
dnl MKVToolNix-Mod: English only, no translations are built or installed;
dnl the gettext check stays so the programs are compiled as before (msgfmt
dnl is no longer needed: it only compiled the translations).
AC_ARG_WITH([gettext], AS_HELP_STRING([--without-gettext],[do not build with gettext support]),
            [ with_gettext=${withval} ], [ with_gettext=yes ])
if test "x$with_gettext" != "xno"; then
  AC_CHECK_FUNCS(gettext, gettext_found=yes, gettext_found=no)
  if test x"$gettext_found" != xyes ; then
    AC_CHECK_LIB(intl, gettext,
                 [ LIBINTL_LIBS="-lintl";
                   gettext_found=yes ],
                 [ gettext_found=no ],
                 [ -liconv ])
  fi
fi

if test x"$gettext_found" = xyes ; then
  AC_CHECK_HEADERS(libintl.h)
  opt_features_yes="$opt_features_yes\n   * gettext"
else
  opt_features_no="$opt_features_no\n   * gettext"
fi

AC_SUBST(LIBINTL_LIBS)
