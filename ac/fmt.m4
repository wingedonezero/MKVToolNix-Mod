dnl
dnl Check for the fmt library
dnl


AC_CACHE_CHECK([fmt],[ac_cv_fmt],[
  AC_LANG_PUSH(C++)

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LIBS="$LIBS"
  CXXFLAGS="$STD_CXX $CXXFLAGS"
  LIBS="$LIBS -lfmt"

  dnl MKVToolNix-Mod: linked, not only compiled (the library must be there too)
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #include <fmt/format.h>
    #include <fmt/ostream.h>

    #if !defined(FMT_VERSION) || (FMT_VERSION < 80000)
    #error fmtlib is too old, need 8.0.0 or later
    #endif
  ]], [[
    fmt::format("{0}", fmt::to_string(4254));
    std::string moo{"'{0}' says the cow"};
    fmt::format(fmt::runtime(moo), "moo");
  ]])],[ac_cv_fmt=yes],[ac_cv_fmt=no])

  CXXFLAGS="$ac_save_CXXFLAGS"
  LIBS="$ac_save_LIBS"

  AC_LANG_POP
])

dnl MKVToolNix-Mod: required, from libs/ (there is no internal copy)
if test x"$ac_cv_fmt" != xyes; then
  AC_MSG_ERROR([fmt 8.0.0 or later not found or not usable (libs/fmt, built by build.sh; see config.log)])
fi
FMT_INTERNAL=no

AC_SUBST(FMT_INTERNAL)
