dist_noinst_DATA = install.nsi.in makezip.sh.in mingwm10.dll uninstall.ico

if MAKENSISSCRIPT

noinst_DATA = installlangfiles.nsh uninstalllangfiles.nsh

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

CLEANFILES = installlangfiles.nsh uninstalllangfiles.nsh

endif

desktopentrydir = $(datadir)/applications
dist_desktopentry_DATA = filezilla.desktop

