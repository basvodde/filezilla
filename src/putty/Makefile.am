if SFTP_UNIX
  FRONTEND = unix
endif
if SFTP_MINGW
  FRONTEND = windows
endif

SUBDIRS = $(FRONTEND) .

noinst_LIBRARIES = libfzputtycommon.a

libfzputtycommon_a_SOURCES = misc.c \
			     sshaes.c \
			     sshdes.c \
			     sshdss.c \
			     sshbn.c \
			     sshmd5.c \
			     sshpubk.c \
			     sshrand.c \
			     sshrsa.c \
			     sshsh256.c \
			     sshsh512.c \
			     sshsha.c \
			     fzprintf.c

bin_PROGRAMS = fzsftp fzputtygen

fzsftp_SOURCES = ssh.c sshcrc.c \
		 sshsha.c sshblowf.c sshdh.c sshcrcda.c sshzlib.c \
		 sshdss.c x11fwd.c portfwd.c \
		 wildcard.c pinger.c ssharcf.c \
		 sftp.c int64.c logging.c \
		 be_none.c cproxy.c \
		 psftp.c cmdline.c \
		 timing.c version.c settings.c tree234.c proxy.c \
		 fzsftp.c

fzputtygen_SOURCES = cmdgen.c \
		     import.c \
		     notiming.c

noinst_HEADERS = fzprintf.h \
		 fzsftp.h \
		 int64.h misc.h network.h proxy.h psftp.h putty.h \
		 puttymem.h puttyps.h sftp.h ssh.h storage.h tree234.h

if USE_RESOURCEFILE
  RESOURCEFILE = windows/psftp.o
endif

INCLUDES = -I$(srcdir)/$(FRONTEND) -I../../config.h

fzsftp_LDADD = libfzputtycommon.a

if SFTP_UNIX
  libfzputtycommon_a_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64

  fzsftp_SOURCES += time.c
  fzsftp_LDADD += unix/libfzsftp_ux.a unix/libfzputtycommon_ux.a
  fzsftp_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64

  fzputtygen_SOURCES += tree234.c
  fzputtygen_CPPFLAGS = $(AM_CPPFLAGS)
  fzputtygen_LDADD = unix/libfzputtycommon_ux.a libfzputtycommon.a
endif

if SFTP_MINGW
  libfzputtycommon_a_CFLAGS = $(AM_CFLAGS) -D_WINDOWS

  fzsftp_SOURCES += pproxy.c
  fzsftp_CPPFLAGS = $(AM_CPPFLAGS) -D_WINDOWS
  fzsftp_LDADD += windows/libfzsftp_win.a windows/libfzputtycommon_win.a $(RESOURCEFILE)

  fzputtygen_CPPFLAGS = $(AM_CPPFLAGS) -D_WINDOWS
  fzputtygen_LDADD = windows/libfzputtycommon_win.a libfzputtycommon.a
endif

if MACAPPBUNDLE
noinst_DATA = $(top_builddir)/FileZilla.app/Contents/MacOS/fzsftp$(EXEEXT)
endif
 
$(top_builddir)/FileZilla.app/Contents/MacOS/fzsftp$(EXEEXT): fzsftp
	mkdir -p $(top_builddir)/FileZilla.app/Contents/MacOS
	cp -f fzsftp $(top_builddir)/FileZilla.app/Contents/MacOS/fzsftp
	cp -f fzputtygen $(top_builddir)/FileZilla.app/Contents/MacOS/fzputtygen
