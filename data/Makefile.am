dist_noinst_DATA = install.nsi.in mingwm10.dll reinstall.ini uninstall.ico

if MAKENSISSCRIPT

noinst_DATA = installlangfiles.nsh uninstalllangfiles.nsh

installlangfiles.nsh: Makefile
	rm -f $@
	for i in $(top_srcdir)/locales/*.po; do\
	  i=$${i%.po};\
	  i=$${i##*/};\
	  echo !insertmacro INSTALLLANGFILE $$i >> $@; \
	done

uninstalllangfiles.nsh: Makefile
	rm -f $@
	for i in $(top_srcdir)/locales/*.po; do\
	  i=$${i%.po};\
	  i=$${i##*/};\
	  echo !insertmacro UNINSTALLLANGFILE $$i >> $@; \
	done

CLEANFILES = installlangfiles.nsh uninstalllangfiles.nsh

endif