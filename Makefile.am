ACLOCAL_AMFLAGS = -I . 

AUTOMAKE_OPTIONS = 1.6

if !LOCALES_ONLY
  MAYBE_SRCDIR = src
  MAYBE_DATADIR = data
endif
SUBDIRS = . $(MAYBE_SRCDIR) locales $(MAYBE_DATADIR)

dist_noinst_SCRIPTS = autogen.sh update-configure.sh

dist_noinst_DATA = filezilla.kdevelop

if !LOCALES_ONLY
if MACAPPBUNDLE
clean-local:
	rm -rf FileZilla.app
endif
endif
