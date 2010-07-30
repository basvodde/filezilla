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
])
AC_DEFUN([FZ_CHECK_TINYXML], [
  AC_LANG_PUSH(C++)

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

  if test "x$with_tinyxml" != "xbuiltin"; then
    AC_HAVE_LIBRARY(tinyxml,
      [
         with_tinyxml=system
      ],
      [
        if test "x$with_tinyxml" = "xsystem"; then
          AC_MSG_ERROR([tinyxml sytem library not found but requested. If you do not have TinyXML installed as system library, you can use the copy of TinyXML distributed with FileZilla by passing --with-tinyxml=builtin as argument to configure.])
        else
          with_tinyxml=builtin
        fi
      ])
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

