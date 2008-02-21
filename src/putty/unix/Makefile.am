noinst_LIBRARIES = libfzsftp_ux.a libfzputtycommon_ux.a

INCLUDES = -I$(srcdir)/.. -I../../include

libfzputtycommon_a_SOURCES = uxcons.c \
			     uxmisc.c \
			     uxnoise.c \
			     uxstore.c

libfzputtycommon_a_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64

libfzsftp_ux_a_SOURCES = uxsftp.c \
			 uxnoise.c \
			 uxagentc.c \
			 uxsel.c \
			 uxnet.c \
			 uxproxy.c

libfzsftp_ux_a_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64

noinst_HEADERS = unix.h
