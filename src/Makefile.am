if MINGW
  MAYBE_FZSHELLEXT = fzshellext
endif

if HAVE_LIBTINYXML
else
  MAYBE_TINYXML = tinyxml
endif

SUBDIRS = include engine $(MAYBE_TINYXML) interface putty $(MAYBE_FZSHELLEXT) .



dist_noinst_DATA = FileZilla.sln
