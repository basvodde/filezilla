lib_LTLIBRARIES = libfzshellext.la

libfzshellext_la_SOURCES = shellext.cpp
libfzshellext_la_LDFLAGS = -Wl,--kill-at -lole32 -luuid -lversion -no-undefined -version-info 0:0:0

noinst_HEADERS = shellext.h resource.h

dist_noinst_DATA = fzshellext.def fzshellext.vcproj fzshellext.rc


