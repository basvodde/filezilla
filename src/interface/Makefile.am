SUBDIRS = resources .
INCLUDES = -I$(srcdir)/../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

bin_PROGRAMS = filezilla

# the application source, library search path, and link libraries
filezilla_SOURCES = FileZilla.cpp LocalListView.cpp LocalTreeView.cpp \
			Mainfrm.cpp Options.cpp QueueView.cpp \
			RemoteListView.cpp RemoteTreeView.cpp StatusView.cpp \
			state.cpp commandqueue.cpp fileexistsdlg.cpp \
                        asyncrequestqueue.cpp led.cpp sitemanager.cpp \
			statuslinectrl.cpp xmlfunctions.cpp ipcmutex.cpp
if USE_BINRELOC
filezilla_SOURCES += prefix.cpp
endif

noinst_HEADERS = 	LocalListView.h Mainfrm.h QueueView.h \
			RemoteTreeView.h LocalTreeView.h Options.h \
			RemoteListView.h StatusView.h state.h commandqueue.h \
			fileexistsdlg.h asyncrequestqueue.h led.h \
			sitemanager.h statuslinectrl.h prefix.h \
			prefix.h xmlfunctions.h filezillaapp.h ipcmutex.h

if USE_RESOURCEFILE
RESOURCEFILE = resources/filezilla.o
endif

filezilla_LDFLAGS = ../engine/libengine.a
filezilla_LDFLAGS += ../tinyxml/libtinyxml.a
filezilla_LDFLAGS += $(WX_LIBS)
filezilla_LDFLAGS += $(RESOURCEFILE)

filezilla_DEPENDENCIES = ../engine/libengine.a \
			../tinyxml/libtinyxml.a

dist_noinst_DATA = interface.vcproj

if MACAPPBUNDLE
noinst_DATA = $(top_builddir)/FileZilla.app/Contents/MacOS/filezilla$(EXEEXT)
endif
 
$(top_builddir)/FileZilla.app/Contents/MacOS/filezilla$(EXEEXT): filezilla
	mkdir -p $(top_builddir)/FileZilla.app/Contents/MacOS
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources
	mkdir -p $(top_builddir)/FileZilla.app/Contents/Resources
	sed s/VERSION/$(VERSION)/ resources/MacInfo.plist >$(top_builddir)/FileZilla.app/Contents/Info.plist
	echo -n "APPL????" >$(top_builddir)/FileZilla.app/Contents/PkgInfo
	$(shell wx-config --rezflags) $(top_builddir)/FileZilla.app/Contents/Resources/filezilla.rsrc
	cp -f resources/FileZilla.icns $(top_builddir)/FileZilla.app/Contents/Resources/FileZilla.icns
	$(MAKE) -C resources mac-copy-resources
	cp -f filezilla $(top_builddir)/FileZilla.app/Contents/MacOS/filezilla
