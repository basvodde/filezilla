#!/bin/sh
echo "HINT: If this script fails, try './update-configure.sh'"
echo "** Creating configure and friends"
if [ ! -e config ]
then
  mkdir config
fi

am_version=`automake --version`
am_version_major=`echo $am_version | sed 's/.*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
am_version_minor=`echo $am_version | sed 's/.*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`

if test $am_version_major -lt 1; then
  export WANT_AUTOMAKE="1.6"
elif test $am_version_major -eq 1 && test $am_version_minor -lt 6; then
  export WANT_AUTOMAKE="1.6"
fi

set -x
  aclocal -I .
  libtoolize --automake -c -f
  autoconf
  automake -a -c -f
set +x
if test ! -f configure || test ! -f config/ltmain.sh || test ! -f Makefile.in; then
  cat<<EOT
** Unable to generate all required files!
** you'll need autoconf 2.5, automake 1.7, libtool 1.5, autoheader and aclocal installed
** If you don't have access to these tools, you can use the
** update-configure.sh script which will download the generated files, 
** just remember to call it from time to time, it will only download 
** if some files have been changed
EOT
  exit 1
fi
echo "Now run ./configure, see ./configure --help for more information"

   
