# these are the headers for your project

noinst_LIBRARIES = libengine.a

INCLUDES = -I../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
# lib_LIBRARIES = libengine.a
libengine_a_SOURCES = FileZillaEngine.cpp ControlSocket.cpp commands.cpp \
			ftpcontrolsocket.cpp server.cpp serverpath.cpp \
			logging.cpp notification.cpp asynchostresolver.cpp

noinst_HEADERS = asynchostresolver.h ControlSocket.h \
			engine_private.h ftpcontrolsocket.h \
			logging_private.h

