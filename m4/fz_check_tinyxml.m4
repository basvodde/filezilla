dnl Checks whether system's TinyXML library exists

AC_DEFUN([FZ_CHECK_TINYXML], [
  AC_ARG_WITH(tinyxml, AC_HELP_STRING([--with-tinyxml=type], [Selects which version of tinyxml to use. Type has to be either system or builtin]),
    [
      if test "x$with_tinyxml" != "xbuiltin"; then
        if test "x$with_tinyxml" != "xsystem"; then
          if test "x$with_tinyxml" != "xauto"; then
            AC_MSG_ERROR([--with-tinyxml has to be set to either system (the default), builtin or auto])
          fi
        fi
      fi
    ],
    [
      if echo $host_os | grep -i "cygwin\|mingw\|mac" > /dev/null 2>&1 ; then
        with_tinyxml=auto
      else
        with_tinyxml=system
      fi
    ])

  AC_LANG_PUSH(C++)

  dnl Check tinyxml.h header
  if test "x$with_tinyxml" != "xbuiltin"; then
    AC_CHECK_HEADER(
      [tinyxml.h],
      [],
      [
        if test "x$with_tinyxml" = "xsystem"; then
          AC_MSG_ERROR([tinyxml.h not found. If you do not have TinyXML installed as system library, you can use the copy of TinyXML distributed with FileZilla by passing --with-tinyxml=builtin as argument to configure.])
        else
          with_tinyxml=builtin
        fi
      ])
  fi

  dnl Check for shared library

  if test "x$with_tinyxml" != "xbuiltin"; then
    dnl Oddity: in AC_CHECK_HEADER I can leave the true case empty, but not in AC_HAVE_LIBRARY
    AC_HAVE_LIBRARY(tinyxml,
      [true],
      [
        if test "x$with_tinyxml" = "xsystem"; then
          AC_MSG_ERROR([tinyxml sytem library not found but requested. If you do not have TinyXML installed as system library, you can use the copy of TinyXML distributed with FileZilla by passing --with-tinyxml=builtin as argument to configure.])
        else
          with_tinyxml=builtin
        fi
      ])
  fi

  dnl Check for known bug in TiXmlBase::EncodeString,
  dnl see http://sourceforge.net/tracker/index.php?func=detail&aid=3031828&group_id=13559&atid=313559
  if test "x$with_tinyxml" != "xbuiltin"; then
    fz_tinyxml_oldlibs=$LIBS
    LIBS="$LIBS -ltinyxml"
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([
          #include <tinyxml.h>
        ], [
          TIXML_STRING input("foo&#xxx;");
          TIXML_STRING output;

          TiXmlBase::EncodeString(input, &output);

          if (output != "foo&amp#xxx;")
            return 1;
          return 0;
        ])
      ], [
         with_tinyxml=system
      ], [
        if test "x$with_tinyxml" = "xsystem"; then
          AC_MSG_ERROR([Broken version TinyXML library detected. See http://sourceforge.net/tracker/index.php?func=detail&aid=3031828&group_id=13559&atid=313559 for details. If you cannot fix the version of TinyXML installed on your system, you can use the copy of TinyXML distributed with FileZilla by passing --with-tinyxml=builtin as argument to configure.])
        else
          with_tinyxml=builtin
        fi
      ], [
        dnl Cannot run script if cross-compiling
        true
      ])
    LIBS="$fz_tinyxml_oldlibs"
  fi

  AC_LANG_POP

  if test "x$with_tinyxml" = "xsystem"; then
    AC_MSG_NOTICE([Using system tinyxml])
    AC_DEFINE(HAVE_LIBTINYXML, 1, [Define to 1 if your system has the `tinyxml' library (-ltinyxml).])
    TINYXML_LIBS="-ltinyxml"
  else
    AC_MSG_NOTICE([Using builtin tinyxml])
    TINYXML_LIBS="../tinyxml/libtinyxml.a"
  fi

  AC_SUBST(TINYXML_LIBS)
])

