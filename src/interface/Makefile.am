SUBDIRS = resources .
INCLUDES = -I../include
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
                        asyncrequestqueue.cpp

noinst_HEADERS = 	LocalListView.h Mainfrm.h QueueView.h \
			RemoteTreeView.h LocalTreeView.h Options.h \
			RemoteListView.h StatusView.h state.h commandqueue.h \
			fileexistsdlg.h asyncrequestqueue.h

if USE_RESOURCEFILE
RESOURCEFILE = resources/filezilla.o
endif

filezilla_LDFLAGS = ../engine/libengine.a
filezilla_LDFLAGS = ../tinyxml/libtinyxml.a
filezilla_LDFLAGS += $(WX_LIBS)
filezilla_LDFLAGS += $(EXPAT_LIB)
filezilla_LDFLAGS += $(PNG_LIB)
filezilla_LDFLAGS += $(ZLIB_LIB)
filezilla_LDFLAGS += $(IDN_LIB)
filezilla_LDFLAGS += $(RESOURCEFILE)

filezilla_DEPENDENCIES = ../engine/libengine.a
filezilla_DEPENDENCIES = ../tinyxml/libtinyxml.a

dist_noinst_DATA = interface.vcproj

if MACAPPBUNDLE
noinst_DATA = FileZilla.app/Contents/MacOS/filezilla$(EXEEXT)
endif

FileZilla.app/Contents/MacOS/filezilla$(EXEEXT): filezilla
	mkdir -p FileZilla.app/Contents/MacOS/resources
	mkdir -p FileZilla.app/Contents/Resources
	sed s/VERSION/$(VERSION)/ resources/MacInfo.plist >FileZilla.app/Contents/Info.plist
	echo -n "APPL????" >FileZilla.app/Contents/PkgInfo
	$(shell wx-config --rezflags) FileZilla.app/Contents/Resources/filezilla.rsrc
	cp -f resources/FileZilla.icns FileZilla.app/Contents/Resources/FileZilla.icns
	$(MAKE) -C resources mac-copy-resources
	cp -f filezilla FileZilla.app/Contents/MacOS/filezilla

clean-local:
	rm -rf FileZilla.app

