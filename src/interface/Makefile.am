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
			state.cpp

noinst_HEADERS = 	LocalListView.h Mainfrm.h QueueView.h \
			RemoteTreeView.h LocalTreeView.h Options.h \
			RemoteListView.h StatusView.h state.h

filezilla_LDFLAGS = ../engine/libengine.a
filezilla_LDFLAGS += $(WX_LIBS) $(WX_XRC_LIBS)
filezilla_LDFLAGS += $(EXPAT_LIB)
filezilla_LDFLAGS += $(PNG_LIB)
filezilla_LDFLAGS += $(ZLIB_LIB)
filezilla_LDFLAGS += $(IDN_LIB)
if USE_RESOURCEFILE
  filezilla_LDFLAGS += resources/filezilla.o
endif

dist_noinst_DATA = interface.vcproj
