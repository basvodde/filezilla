#!/bin/sh
echo "HINT: If this script fails, try './update-configure.sh'"
echo "** Creating configure and friends"
if [ ! -e config ]
then
  mkdir config
fi
set -x
aclocal
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

   
