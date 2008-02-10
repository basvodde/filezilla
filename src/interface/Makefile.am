SUBDIRS = resources .
INCLUDES = -I$(srcdir)/../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

bin_PROGRAMS = filezilla

# the application source, library search path, and link libraries
filezilla_SOURCES = aboutdialog.cpp \
		asyncrequestqueue.cpp \
		aui_notebook_ex.cpp \
		buildinfo.cpp \
		chmoddialog.cpp \
		clearprivatedata.cpp \
		commandqueue.cpp \
		conditionaldialog.cpp \
		customheightlistctrl.cpp \
		defaultfileexistsdlg.cpp \
		dialogex.cpp \
		dndobjects.cpp \
		dragdropmanager.cpp \
		edithandler.cpp \
		export.cpp \
		fileexistsdlg.cpp \
		filelistctrl.cpp \
		FileZilla.cpp filter.cpp \
		filteredit.cpp \
		import.cpp \
		inputdialog.cpp \
		ipcmutex.cpp \
		led.cpp \
		listctrlex.cpp \
		listingcomparison.cpp \
		locale_initializer.cpp \
		LocalListView.cpp \
		LocalTreeView.cpp \
		loginmanager.cpp \
		Mainfrm.cpp \
		netconfwizard.cpp \
		Options.cpp \
		optionspage.cpp \
		optionspage_connection.cpp \
		optionspage_connection_active.cpp \
		optionspage_connection_passive.cpp \
		optionspage_dateformatting.cpp \
		optionspage_debug.cpp \
		optionspage_edit.cpp \
		optionspage_fileexists.cpp \
		optionspage_filetype.cpp \
		optionspage_ftpproxy.cpp \
		optionspage_interface.cpp \
		optionspage_language.cpp \
		optionspage_logging.cpp \
		optionspage_sizeformatting.cpp \
		optionspage_themes.cpp \
		optionspage_transfer.cpp \
		optionspage_updatecheck.cpp \
		queue.cpp \
		QueueView.cpp \
		queueview_failed.cpp \
		queueview_successful.cpp \
		quickconnectbar.cpp \
		recentserverlist.cpp \
		recursive_operation.cpp \
		RemoteListView.cpp \
		RemoteTreeView.cpp \
		settingsdialog.cpp \
		sitemanager.cpp \
		state.cpp \
		statuslinectrl.cpp \
		StatusView.cpp \
		systemimagelist.cpp \
		themeprovider.cpp \
		updatewizard.cpp \
		verifycertdialog.cpp \
		view.cpp \
		viewheader.cpp \
		window_state_manager.cpp \
		wrapengine.cpp \
		xh_toolb_ex.cpp \
		xmlfunctions.cpp
 
if USE_BINRELOC
filezilla_SOURCES += prefix.cpp
endif

noinst_HEADERS = aboutdialog.h \
		 asyncrequestqueue.h \
		 aui_notebook_ex.h \
		 buildinfo.h \
		 chmoddialog.h \
		 clearprivatedata.h \
		 commandqueue.h \
		 conditionaldialog.h \
		 customheightlistctrl.h \
		 defaultfileexistsdlg.h \
		 dialogex.h \
		 dndobjects.h \
		 dragdropmanager.h \
		 edithandler.h \
		 export.h \
		 fileexistsdlg.h \
		 filelistctrl.h \
		 filezillaapp.h \
		 filter.h \
		 filteredit.h \
		 import.h \
		 inputdialog.h \
		 ipcmutex.h \
		 led.h \
		 listctrlex.h \
		 listingcomparison.h \
		 locale_initializer.h \
		 LocalListView.h \
		 LocalTreeView.h \
		 loginmanager.h \
		 Mainfrm.h \
		 netconfwizard.h \
		 Options.h \
		 optionspage.h \
		 optionspage_connection.h \
		 optionspage_connection_active.h \
		 optionspage_connection_passive.h \
		 optionspage_dateformatting.h \
		 optionspage_debug.h \
		 optionspage_edit.h \
		 optionspage_fileexists.h \
		 optionspage_filetype.h \
		 optionspage_ftpproxy.h \
		 optionspage_interface.h \
		 optionspage_language.h \
		 optionspage_logging.h \
		 optionspage_sizeformatting.h \
		 optionspage_themes.h \
		 optionspage_transfer.h \
		 optionspage_updatecheck.h \
		 prefix.h \
		 queue.h \
		 QueueView.h \
		 queueview_failed.h \
		 queueview_successful.h \
		 quickconnectbar.h \
		 recentserverlist.h \
		 recursive_operation.h \
		 RemoteListView.h \
		 RemoteTreeView.h \
		 settingsdialog.h \
		 sitemanager.h \
		 state.h \
		 statuslinectrl.h \
		 StatusView.h \
		 systemimagelist.h \
		 themeprovider.h \
		 updatewizard.h \
		 verifycertdialog.h \
		 view.h \
		 viewheader.h \
		 window_state_manager.h \
		 wrapengine.h \
		 xh_toolb_ex.h \
		 xmlfunctions.h

if USE_RESOURCEFILE
RESOURCEFILE = resources/filezilla.o
endif

filezilla_CPPFLAGS = $(WX_CPPFLAGS)
filezilla_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_CFLAGS = $(WX_CFLAGS_ONLY)

filezilla_LDFLAGS = ../engine/libengine.a
filezilla_LDFLAGS += ../tinyxml/libtinyxml.a
filezilla_LDFLAGS += $(LIBGNUTLS_LIBS)
filezilla_LDFLAGS += $(WX_LIBS)
filezilla_LDFLAGS += $(RESOURCEFILE)
filezilla_LDFLAGS += $(IDN_LIB)

if MINGW
filezilla_LDFLAGS += -lole32 -luuid -lnetapi32
endif

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
	$(MAKE) -C resources mac-copy-resources
	cp -f filezilla $(top_builddir)/FileZilla.app/Contents/MacOS/filezilla
