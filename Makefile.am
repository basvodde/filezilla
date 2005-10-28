ACLOCAL_AMFLAGS = -I . 

AUTOMAKE_OPTIONS = 1.6

SUBDIRS = . src locales data

dist_noinst_SCRIPTS = autogen.sh update-configure.sh

dist_noinst_DATA = filezilla.kdevelop

if MACAPPBUNDLE
clean-local:
	rm -rf FileZilla.app
endif
