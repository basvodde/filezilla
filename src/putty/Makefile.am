if SFTP_UNIX
  FRONTEND = unix
endif
if SFTP_MINGW
  FRONTEND = windows
endif

SUBDIRS = $(FRONTEND) .

bin_PROGRAMS = fzsftp

fzsftp_SOURCES = ssh.c sshcrc.c sshdes.c sshmd5.c sshrsa.c sshrand.c \
		 sshsha.c sshblowf.c sshdh.c sshcrcda.c sshpubk.c sshzlib.c \
		 sshdss.c x11fwd.c portfwd.c sshaes.c sshsh512.c sshbn.c \
		 wildcard.c pinger.c ssharcf.c \
		 sftp.c int64.c logging.c \
		 be_none.c cproxy.c \
		 psftp.c cmdline.c \
		 timing.c misc.c version.c settings.c tree234.c proxy.c \
		 fzsftp.c

noinst_HEADERS = fzsftp.h int64.h misc.h network.h proxy.h psftp.h putty.h \
		 puttymem.h puttyps.h sftp.h ssh.h storage.h tree234.h

if USE_RESOURCEFILE
  RESOURCEFILE = windows/pscp.o
endif

INCLUDES = -I$(srcdir)/$(FRONTEND)

if SFTP_UNIX
  fzsftp_SOURCES += time.c
  fzsftp_LDADD = unix/libfzsftp_ux.a
endif

if SFTP_MINGW
  fzsftp_SOURCES += pproxy.c
  fzsftp_CFLAGS = $(AM_CFLAGS) -D_WINDOWS
  fzsftp_LDADD = windows/libfzsftp_win.a $(RESOURCEFILE)
endif