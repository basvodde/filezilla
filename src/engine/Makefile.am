# these are the headers for your project

noinst_LIBRARIES = libengine.a

INCLUDES = -I$(srcdir)/../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
# lib_LIBRARIES = libengine.a
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
