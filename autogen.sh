#! /bin/sh

echo -e " --- FileZilla 3 autogen script ---\n"
echo -e "\033[1mHINT:\033[0m If this script fails, try \033[1m./update-configure.sh\033[0m\n"

failedAclocal()
{
	echo -e "\n\033[1;31m*** Failed to execute aclocal\033[0m"
	echo "" 
	echo '    If you get errors about undefined symbols, make sure'
	echo '    that the corresponding .m4 file is in your aclocal search'
	echo '    directory.'
	echo '    Especially if you get an error about undefined'
	echo '    AC_PROG_LIBTOOL, search for a .m4 file with libtool in'
	echo '    its name and copy the file to the directory,'
	echo -e '    \033[1maclocal --print-ac-dir\033[0m prints.'

	exit 1
}

echo -n "1. Cleaning previous files... "

rm -rf  configure config.log aclocal.m4 \
	config.status config autom4te.cache libtool
find . -name "Makefile.in" | xargs rm -f
find . -name "Makefile" | xargs rm -f
echo done

echo "2. Creating configure and friends... "
if [ ! -e config ]
then
  mkdir config
fi

echo "2.1 Checking automake version... "
amk_version=`automake --version`
am_version_major=`echo $am_version | sed 's/.*\([[0-9]]*\).\([[0-9]]*\)\(\.\|-p\)\([[0-9]]*\).*/\1/'`
am_version_minor=`echo $am_version | sed 's/.*\([[0-9]]*\).\([[0-9]]*\)\(\.\|-p\)\([[0-9]]*\).*/\2/'`

if test "$am_version_major" = "" -o "$am_version_minor" = ""; then
  export WANT_AUTOMAKE="1.7"
elif test $am_version_major -lt 1; then
  export WANT_AUTOMAKE="1.7"
elif test $am_version_major -eq 1 && test $am_version_minor -lt 7; then
  export WANT_AUTOMAKE="1.7"
fi

echo "2.2 Running aclocal... "
aclocal -I . || failedAclocal

echo "2.3 Runnig libtoolize... "
libtoolize --automake -c -f

echo "2.4 Running autoconf... "
autoconf

echo "2.5 Runing automake... "
automake -a -c -f

echo -n "3. Checking generate files... "
if test ! -f configure || test ! -f config/ltmain.sh || test ! -f Makefile.in; then
  echo -e "\033[1;31mfailed"
  echo -e "\nError: Unable to generate all required files!\033[0m\n"
  echo "Please make sure you have you have autoconf 2.5, automake 1.7, libtool 1.5,"
  echo "autoheader and aclocal installed."
  echo -e "If you don't have access to these tools, you can use the \033[1mupdate-configure.sh\033[0m"
  echo "script which will download the generated files, just remember to call it from"
  echo "time to time, it will only download if some files have been changed"

  exit 1
fi
echo done
echo -e "\nScript completed successfully."
echo -e "Now run \033[1m./configure\033[0m, see \033[1m./configure --help\033[0m for more information"

