noinst_LIBRARIES = libfzsftp_ux.a

INCLUDES = -I$(srcdir)/.. -I../../include

libfzsftp_ux_a_SOURCES = uxsftp.c uxcons.c \
			 uxnoise.c uxagentc.c \
			 uxstore.c uxsel.c uxnet.c uxmisc.c uxproxy.c

libfzsftp_ux_a_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64

noinst_HEADERS = unix.h
