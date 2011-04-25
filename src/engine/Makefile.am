# these are the headers for your project

noinst_LIBRARIES = libengine.a

INCLUDES = -I$(srcdir)/../include

libengine_a_CPPFLAGS = $(LIBGNUTLS_CFLAGS) $(WX_CPPFLAGS)
libengine_a_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
libengine_a_CFLAGS = $(WX_CFLAGS_ONLY)

libengine_a_SOURCES = \
		backend.cpp \
		commands.cpp \
		ControlSocket.cpp \
		directorycache.cpp \
		directorylisting.cpp \
		directorylistingparser.cpp \
		engineprivate.cpp \
		externalipresolver.cpp \
		FileZillaEngine.cpp \
		ftpcontrolsocket.cpp \
		httpcontrolsocket.cpp \
		iothread.cpp \
		local_filesys.cpp \
		local_path.cpp \
		logging.cpp \
		misc.cpp notification.cpp \
		pathcache.cpp \
		proxy.cpp \
		ratelimiter.cpp \
		server.cpp serverpath.cpp\
		servercapabilities.cpp \
		sftpcontrolsocket.cpp \
		sizeformatting_base.cpp \
		socket.cpp \
		string_coalescer.cpp \
		tlssocket.cpp \
		threadex.cpp \
		timeex.cpp \
		transfersocket.cpp

noinst_HEADERS = backend.h \
		ControlSocket.h \
		directorycache.h \
		directorylistingparser.h \
		filezilla.h \
		ftpcontrolsocket.h \
		httpcontrolsocket.h iothread.h \
		logging_private.h \
		pathcache.h \
		proxy.h \
		ratelimiter.h \
		servercapabilities.h \
		sftpcontrolsocket.h \
		tlssocket.h \
		transfersocket.h

dist_noinst_DATA = engine.vcproj

CLEANFILES = filezilla.h.gch

if ENABLE_PRECOMP

BUILT_SOURCES = filezilla.h.gch

include ./$(DEPDIR)/filezilla.Po

filezilla.h.gch: filezilla.h
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libengine_a_CPPFLAGS) $(CPPFLAGS) $(libengine_a_CXXFLAGS) $(CXXFLAGS) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ $<; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

endif

