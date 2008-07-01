# these are the headers for your project

noinst_LIBRARIES = libengine.a

INCLUDES = -I$(srcdir)/../include

libengine_a_CPPFLAGS = $(LIBGNUTLS_CFLAGS) $(WX_CPPFLAGS)
libengine_a_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
libengine_a_CFLAGS = $(WX_CFLAGS_ONLY)


libengine_a_SOURCES = \
		backend.cpp \
		commands.cpp ControlSocket.cpp \
		directorycache.cpp directorylisting.cpp directorylistingparser.cpp \
		engineprivate.cpp externalipresolver.cpp \
		FileZillaEngine.cpp ftpcontrolsocket.cpp \
		httpcontrolsocket.cpp \
		iothread.cpp \
		local_filesys.cpp \
		logging.cpp \
		misc.cpp notification.cpp \
		pathcache.cpp \
		ratelimiter.cpp \
		server.cpp serverpath.cpp\
		servercapabilities.cpp \
		sftpcontrolsocket.cpp \
		socket.cpp \
		tlssocket.cpp \
		threadex.cpp timeex.cpp transfersocket.cpp

noinst_HEADERS = backend.h \
		ControlSocket.h directorycache.h \
		directorylistingparser.h ftpcontrolsocket.h \
		httpcontrolsocket.h iothread.h \
		logging_private.h \
		pathcache.h \
		ratelimiter.h \
		servercapabilities.h \
		sftpcontrolsocket.h \
		socket.h \
		tlssocket.h \
		transfersocket.h

dist_noinst_DATA = engine.vcproj
