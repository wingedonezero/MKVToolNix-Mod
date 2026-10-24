dnl
dnl Check for libFLAC
dnl

  AC_ARG_WITH([flac],
              AS_HELP_STRING([--without-flac],[do not build with flac support]),
              [ with_flac=${withval} ], [ with_flac=yes ])

  if test "$with_flac" != "no"; then
    PKG_CHECK_EXISTS([flac],[flac_found=yes],[flac_found=no])
    if test x"$flac_found" = xyes; then
      PKG_CHECK_MODULES([flac],[flac],[flac_found=yes])
      FLAC_CFLAGS="`$PKG_CONFIG --cflags flac`"
      FLAC_LIBS="`$PKG_CONFIG --libs flac`"
    fi
  else
    flac_found=no
  fi

  if test x"$flac_found" = xyes ; then
    AC_CHECK_LIB(FLAC, FLAC__stream_decoder_skip_single_frame,
                 [ flac_decoder_skip_found=yes ],
                 [ flac_decoder_skip_found=no ],
                 [ $FLAC_LIBS $OGG_LIBS ])
    if test x"$flac_decoder_skip_found" = xyes; then
      opt_features_yes="$opt_features_yes\n   * FLAC audio"
      AC_DEFINE(HAVE_FLAC_DECODER_SKIP, [1], [Define if FLAC__stream_decoder_skip_single_frame exists])
      AC_DEFINE(HAVE_FLAC_FORMAT_H, [1], [Define if the FLAC headers are present])
    else
      dnl MKVToolNix-Mod: never quietly without FLAC
      AC_MSG_ERROR([libFLAC too old: no FLAC__stream_decoder_skip_single_frame (libs/flac; or configure --without-flac)])
    fi
  elif test "$with_flac" != "no"; then
    dnl MKVToolNix-Mod: never quietly without FLAC
    AC_MSG_ERROR([libFLAC not found (libs/flac, built by build.sh; or configure --without-flac)])
  else
    FLAC_CFLAGS=""
    FLAC_LIBS=""
    opt_features_no="$opt_features_no\n   * FLAC audio"
  fi

AC_SUBST(FLAC_CFLAGS)
AC_SUBST(FLAC_LIBS)
