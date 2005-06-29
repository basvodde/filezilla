noinst_LIBRARIES = libfzsftp_win.a

INCLUDES = -I$(srcdir)/..

libfzsftp_win_a_CFLAGS = $(AM_CFLAGS) -D_WINDOWS -DNO_IPV6
libfzsftp_win_a_SOURCES = winnoise.c winpgntc.c \
		       winstore.c winnet.c \
		       windefs.c winmisc.c \
		       wintime.c \
		       winsftp.c wincons.c

noinst_HEADERS = winstuff.h

dist_noinst_DATA = pscp.rc pscp.ico

CLEANFILES = pscp.o

if USE_RESOURCEFILE

nodist_noinst_DATA = pscp.o

pscp.o: $(srcdir)/pscp.rc
	@if [ -f $@~ ]; then rm $@~; fi
	@WINDRES@ --include-dir=$(srcdir) $(WINDRESFLAGS) $(srcdir)/pscp.rc $@~
	@if [ -f $@ ]; then rm $@; fi
	mv $@~ $@
endif