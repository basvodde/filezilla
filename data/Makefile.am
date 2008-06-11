dist_noinst_DATA = install.nsi.in makezip.sh.in mingwm10.dll uninstall.ico

if MAKENSISSCRIPT

noinst_DATA = installlangfiles.nsh uninstalllangfiles.nsh libtoolexecutablesubdir.nsh

installlangfiles.nsh: Makefile
	rm -f $@
	for i in @FILEZILLA_LINGUAS@; do\
	  echo !insertmacro INSTALLLANGFILE $$i >> $@; \
	done

uninstalllangfiles.nsh: Makefile
	rm -f $@
	for i in @FILEZILLA_LINGUAS@; do\
	  echo !insertmacro UNINSTALLLANGFILE $$i >> $@; \
	done

# Some versions of libtool create executables as foo.exe
# while otheres create executables as .libs/foo.exe
# NSIS needs to know the actual path.

libtoolexecutablesubdir.nsh: Makefile
	rm -f $@
	touch test_libtool_exedir.c
	$(CC) -c -o test_libtool_exedir.o test_libtool_exedir.c
	exedir=`$(LIBTOOL) -n --mode=link $(CC) -o test_libtool_exedir$(EXEEXT) test_libtool_exedir.o`; \
	if echo "$$exedir" | grep '.libs' > /dev/null; then \
	  echo "!define LT_EXEDIR \".libs\\\"" > libtoolexecutablesubdir.nsh; \
	else \
	  touch libtoolexecutablesubdir.nsh; \
	fi
	rm -f test_libtool_exedir.o test_libtool_exedir.c

CLEANFILES = installlangfiles.nsh uninstalllangfiles.nsh libtoolexecutablesubdir.nsh

endif

desktopentrydir = $(datadir)/applications
dist_desktopentry_DATA = filezilla.desktop

