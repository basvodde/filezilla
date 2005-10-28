dnl FZ_PATH_PROG_VER does basically the same as AC_PATH_PROG, but
dnl wants the minimum acceptable version of the program as 3rd parameter.

AC_DEFUN([FZ_PATH_PROG_VER],
[
  AC_PATH_PROG([$1], [$2], [$4], [$5])

  if ! test -z "@S|@$1" && ! test -z "$3"; then
    fz_req_version=$3
    fz_req_major=`echo $fz_req_version | cut -d. -f1`
    fz_req_minor=`echo $fz_req_version | sed s/@<:@-,a-z,A-Z@:>@.*// | cut -d. -f2`
    fz_req_micro=`echo $fz_req_version | sed s/@<:@-,a-z,A-Z@:>@.*// | cut -d. -f3`
    if test -z "$fz_req_minor"; then fz_req_minor=0; fi
    if test -z "$fz_req_micro"; then fz_req_micro=0; fi

    AC_MSG_CHECKING([whether $2 version >= $fz_req_major.$fz_req_minor.$fz_req_micro])

    fz_pkg_version=`@S|@$1 --version|head -n 1|sed 's/(@<:@^)@:>@*)//g;s/^@<:@a-zA-Z\.\ \-@:>@*//;s/ .*$//'`
    fz_pkg_major=`echo $fz_pkg_version | cut -d. -f1`
    fz_pkg_minor=`echo $fz_pkg_version | sed s/@<:@-,a-z,A-Z@:>@.*// | cut -d. -f2`
    fz_pkg_micro=`echo $fz_pkg_version | sed s/@<:@-,a-z,A-Z@:>@.*// | cut -d. -f3`
    if test -z "$fz_pkg_minor"; then fz_pkg_minor=0; fi
    if test -z "$fz_pkg_micro"; then fz_pkg_micro=0; fi

    if test "$fz_pkg_major" -lt "$fz_req_major"; then
      $1=
    elif test "$fz_pkg_major" -eq "$fz_req_major"; then
      if test "$fz_pkg_minor" -lt "$fz_req_minor"; then
        $1=
      elif test "$fz_pkg_minor" -eq "$fz_req_minor"; then
        if test "$fz_pkg_micro" -lt "$fz_req_micro"; then
          $1=
        fi
      fi
    fi

    if test -z "@S|@$1"; then
      AC_MSG_RESULT([no, $fz_pkg_version])
    else
      AC_MSG_RESULT([yes, $fz_pkg_version])
    fi

  fi
])
