noinst_LIBRARIES = libfzsftp_win.a

INCLUDES = -I$(srcdir)/..

libfzsftp_win_a_CFLAGS = $(AM_CFLAGS) -D_WINDOWS -DNO_IPV6
libfzsftp_win_a_SOURCES = winnoise.c winpgntc.c \
		       winstore.c winnet.c \
		       windefs.c winmisc.c \
		       wintime.c \
		       winsftp.c wincons.c \
                       winhandl.c

noinst_HEADERS = winstuff.h rcstuff.h

dist_noinst_DATA = psftp.rc version.rc2 pscp.ico

CLEANFILES = pscp.o

if USE_RESOURCEFILE

nodist_noinst_DATA = psftp.o

psftp.o: $(srcdir)/psftp.rc
	@if [ -f $@~ ]; then rm $@~; fi
	@WINDRES@ --include-dir=$(srcdir) $(WINDRESFLAGS) $(srcdir)/psftp.rc $@~
	@if [ -f $@ ]; then rm $@; fi
	mv $@~ $@
endif