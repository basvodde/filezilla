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

version_check()
{
	PACKAGE=$1
	MAJOR=$2
	MINOR=$3
	MICRO=$4
	SILENT=$5

	VERSION=$MAJOR

	if [ ! -z "$MINOR" ]; then VERSION=$VERSION.$MINOR; else MINOR=0; fi
	if [ ! -z "$MICRO" ]; then VERSION=$VERSION.$MICRO; else MICRO=0; fi
	
	if [ x$SILENT != x2 ]; then
		if [ ! -z $VERSION ]; then
			echo -n "Checking for $PACKAGE >= $VERSION ... "
		else
			echo -n "Checking for $PACKAGE ... "
		fi
	fi
	
	($PACKAGE --version) < /dev/null > /dev/null 2>&1 ||
	{
		echo -e "\033[1;31mnot found\033[0m\n"
		echo -e "\033[1m$PACKAGE\033[0m could not be found. On some systems \033[1m$PACKAGE\033[0m might be installed but"
		echo -e "has a suffix with the version number, for example \033[1m$PACKAGE-$VERSION\033[0m."
		echo -e "If that is the case, create a symlink to \033[1m$PACKAGE\033[0m"
		exit 1
	}

	# the following line is carefully crafted sed magic
	pkg_version=`$PACKAGE --version|head -n 1|sed 's/([^)]*)//g;s/^[a-zA-Z\.\ \-]*//;s/ .*$//'`
	pkg_major=`echo $pkg_version | cut -d. -f1`
	pkg_minor=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
	pkg_micro=`echo $pkg_version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
	[ -z "$pkg_minor" ] && pkg_minor=0
	[ -z "$pkg_micro" ] && pkg_micro=0

	WRONG=
	if [ -z $MAJOR ]; then
		echo "found $pkg_version, ok."
		return 0
	fi
	
	# start checking the version
	if [ "$pkg_major" -lt "$MAJOR" ]; then
		WRONG=1
	elif [ "$pkg_major" -eq "$MAJOR" ]; then
		if [ "$pkg_minor" -lt "$MINOR" ]; then
			WRONG=1
		elif [ "$pkg_minor" -eq "$MINOR" -a "$pkg_micro" -lt "$MICRO" ]; then
			WRONG=1
		fi
	fi

	if [ ! -z "$WRONG" ]; then
		if [ x$SILENT = x1 ]; then
			return 2;
		fi
		echo -e "\033[1mfound $pkg_version, not ok !\033[0m"
		echo "You must have $PACKAGE $VERSION or greater to generate the configure script."
		echo -e "If \033[1m$PACKAGE\033[0m is a symlink, make sure it points to the correct version."
		echo -e "Please update your installation or use the provides \033[1mupdate-configure.sh\033[0m script."
		exit 2
	else
		echo "found $pkg_version, ok."
		return 0
	fi
}

echo "2 Checking required tools... "

echo -n "2.1 "; version_check automake 1 7 0 1
if [ x$? = x2 ]; then
	WANT_AUTOMAKE=1.7
	export WANT_AUTOMAKE
	version_check automake 1 7 0 2
fi

echo -n "2.2 "; version_check aclocal
echo -n "2.3 "; version_check autoconf 2 5
echo -n "2.4 "; version_check libtoolize 1 4

echo "3. Creating configure and friends... "
if [ ! -e config ]
then
  mkdir config
fi

echo "3.1 Running aclocal... "
aclocal -I . || failedAclocal

echo "3.2 Runnig libtoolize... "
libtoolize --automake -c -f

echo "3.3 Running autoconf... "
autoconf

echo "3.4 Runing automake... "
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

