CLEANFILES = fzshellext.o

lib_LTLIBRARIES = libfzshellext.la

libfzshellext_la_SOURCES = shellext.cpp

if USE_RESOURCEFILE
RESOURCEFILE = -Wl,fzshellext.o
libfzshellext_la_DEPENCENCIES = fzshellext.o
endif

libfzshellext_la_LDFLAGS = -Wl,--kill-at -lole32 -luuid -lversion -no-undefined -version-info 0:0:0 $(RESOURCEFILE)

noinst_HEADERS = shellext.h resource.h

dist_noinst_DATA = fzshellext.def fzshellext.vcproj fzshellext.rc version.rc.in

if USE_RESOURCEFILE

fzshellext.o: $(srcdir)/fzshellext.rc
	@if [ -f $@~ ]; then rm $@~; fi
	@WINDRES@ --include-dir=. --include-dir=$(srcdir) $(WINDRESFLAGS) $(srcdir)/fzshellext.rc -D HAS_VERSIONRC $@~ || rm $@~
	@if [ -f $@ ]; then rm $@; fi
	mv $@~ $@

BUILT_SOURCES = fzshellext.o

endif
