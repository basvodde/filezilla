# these are the headers for your project

noinst_LIBRARIES = libengine.a

INCLUDES = -I$(srcdir)/../include

libengine_a_CPPFLAGS = $(WX_CPPFLAGS)
libengine_a_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
libengine_a_CFLAGS = $(WX_CFLAGS_ONLY)


libengine_a_SOURCES = asynchostresolver.cpp commands.cpp ControlSocket.cpp \
		directorycache.cpp directorylisting.cpp directorylistingparser.cpp \
		engineprivate.cpp \
		FileZillaEngine.cpp ftpcontrolsocket.cpp logging.cpp \
		misc.cpp notification.cpp server.cpp serverpath.cpp\
		sftpcontrolsocket.cpp timeex.cpp transfersocket.cpp

noinst_HEADERS = asynchostresolver.h ControlSocket.h directorycache.h \
		directorylistingparser.h ftpcontrolsocket.h \
		logging_private.h sftpcontrolsocket.h transfersocket.h

dist_noinst_DATA = engine.vcproj
