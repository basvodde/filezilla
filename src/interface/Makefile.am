SUBDIRS = resources
INCLUDES = -I../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = filezilla

# the application source, library search path, and link libraries
filezilla_SOURCES = FileZilla.cpp LocalListView.cpp LocalListView.h \
			LocalTreeView.cpp LocalTreeView.h Mainfrm.cpp \
			Mainfrm.h Options.cpp Options.h QueueView.cpp \
			QueueView.h RemoteListView.cpp RemoteListView.h \
			RemoteTreeView.cpp RemoteTreeView.h \
			StatusView.cpp StatusView.h
filezilla_LDFLAGS = $(WX_LIBS) $(WX_XRC_LIBS)
filezilla_LDADD = ../engine/libengine.a
filezilla_LDFLAGS += -lexpat
