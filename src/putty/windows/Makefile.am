noinst_LIBRARIES = libfzsftp_win.a libfzputtycommon_win.a

INCLUDES = -I$(srcdir)/..

libfzputtycommon_win_a_CFLAGS = $(AM_CFLAGS) \
				-D_WINDOWS -D_WIN32_IE=0x0500 -DWINVER=0x0500 -D_WIN32_WINDOWS=0x0410 -D_WIN32_WINNT=0x0500 \
				-DNO_IPV6

libfzputtycommon_win_a_SOURCES = wincons.c \
				 winmisc.c \
				 winnoise.c \
				 winstore.c

libfzsftp_win_a_CFLAGS = $(AM_CFLAGS) \
			-D_WINDOWS -D_WIN32_IE=0x0500 -DWINVER=0x0500 -D_WIN32_WINDOWS=0x0410 -D_WIN32_WINNT=0x0500 \
			-DNO_IPV6

libfzsftp_win_a_SOURCES = winpgntc.c \
		       winnet.c \
		       windefs.c \
		       wintime.c \
		       winsftp.c \
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