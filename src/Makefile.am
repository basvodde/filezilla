if MINGW
  MAYBE_FZSHELLEXT = fzshellext
endif

if HAVE_LIBTINYXML
else
  MAYBE_TINYXML = tinyxml
endif

if HAVE_DBUS
else
  MAYBE_DBUS = dbus
endif

SUBDIRS = include engine $(MAYBE_TINYXML) $(MAYBE_DBUS) interface putty $(MAYBE_FZSHELLEXT) .



dist_noinst_DATA = FileZilla.sln
