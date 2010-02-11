SUBDIRS = resources .
INCLUDES = -I$(srcdir)/../include
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = filezilla

# the application source, library search path, and link libraries
filezilla_SOURCES = aboutdialog.cpp \
		asyncrequestqueue.cpp \
		aui_notebook_ex.cpp \
		auto_ascii_files.cpp \
		bookmarks_dialog.cpp \
		buildinfo.cpp \
		chmoddialog.cpp \
		clearprivatedata.cpp \
		cmdline.cpp \
		commandqueue.cpp \
		conditionaldialog.cpp \
		context_control.cpp \
		customheightlistctrl.cpp \
		defaultfileexistsdlg.cpp \
		dialogex.cpp \
		dndobjects.cpp \
		dragdropmanager.cpp \
		edithandler.cpp \
		export.cpp \
		fileexistsdlg.cpp \
		filelistctrl.cpp \
		filelist_statusbar.cpp \
		FileZilla.cpp \
		filter.cpp \
		filter_conditions_dialog.cpp \
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
		manual_transfer.cpp \
		menu_bar.cpp \
		netconfwizard.cpp \
		option_change_event_handler.cpp \
		Options.cpp \
		power_management.cpp \
		queue.cpp \
		QueueView.cpp \
		queueview_failed.cpp \
		queueview_successful.cpp \
		quickconnectbar.cpp \
		recentserverlist.cpp \
		recursive_operation.cpp \
		RemoteListView.cpp \
		RemoteTreeView.cpp \
		search.cpp \
		settings/optionspage.cpp \
		settings/optionspage_connection.cpp \
		settings/optionspage_connection_active.cpp \
		settings/optionspage_connection_ftp.cpp \
		settings/optionspage_connection_passive.cpp \
		settings/optionspage_connection_sftp.cpp \
		settings/optionspage_dateformatting.cpp \
		settings/optionspage_debug.cpp \
		settings/optionspage_edit.cpp \
		settings/optionspage_edit_associations.cpp \
		settings/optionspage_fileexists.cpp \
		settings/optionspage_filelists.cpp \
		settings/optionspage_filetype.cpp \
		settings/optionspage_ftpproxy.cpp \
		settings/optionspage_interface.cpp \
		settings/optionspage_language.cpp \
		settings/optionspage_logging.cpp \
		settings/optionspage_proxy.cpp \
		settings/optionspage_sizeformatting.cpp \
		settings/optionspage_themes.cpp \
		settings/optionspage_transfer.cpp \
		settings/optionspage_updatecheck.cpp \
		settings/settingsdialog.cpp \
		sftp_crypt_info_dlg.cpp \
		sitemanager.cpp \
		sizeformatting.cpp \
		speedlimits_dialog.cpp \
		splitter.cpp \
		state.cpp \
		statusbar.cpp \
		statuslinectrl.cpp \
		StatusView.cpp \
		systemimagelist.cpp \
		themeprovider.cpp \
		toolbar.cpp \
		treectrlex.cpp \
		updatewizard.cpp \
		verifycertdialog.cpp \
		verifyhostkeydialog.cpp \
		view.cpp \
		viewheader.cpp \
		volume_enumerator.cpp \
		welcome_dialog.cpp \
		window_state_manager.cpp \
		wrapengine.cpp \
		xh_menu_ex.cpp \
		xh_toolb_ex.cpp \
		xmlfunctions.cpp
 
if USE_BINRELOC
filezilla_SOURCES += prefix.cpp
endif

noinst_HEADERS = aboutdialog.h \
		 asyncrequestqueue.h \
		 aui_notebook_ex.h \
		 auto_ascii_files.h \
		 bookmarks_dialog.h \
		 buildinfo.h \
		 chmoddialog.h \
		 clearprivatedata.h \
		 cmdline.h \
		 commandqueue.h \
		 conditionaldialog.h \
		 context_control.h \
		 customheightlistctrl.h \
		 defaultfileexistsdlg.h \
		 dialogex.h \
		 dndobjects.h \
		 dragdropmanager.h \
		 edithandler.h \
		 export.h \
		 fileexistsdlg.h \
		 filelistctrl.h \
		 filelist_statusbar.h \
		 filezilla.h \
		 filezillaapp.h \
		 filter.h \
		 filter_conditions_dialog.h \
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
		 manual_transfer.h \
		 menu_bar.h \
		 netconfwizard.h \
		 option_change_event_handler.h \
		 Options.h \
		 power_management.h \
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
		 search.h \
		 settings/optionspage.h \
		 settings/optionspage_connection.h \
		 settings/optionspage_connection_active.h \
		 settings/optionspage_connection_ftp.h \
		 settings/optionspage_connection_passive.h \
		 settings/optionspage_connection_sftp.h \
		 settings/optionspage_dateformatting.h \
		 settings/optionspage_debug.h \
		 settings/optionspage_edit.h \
		 settings/optionspage_edit_associations.h \
		 settings/optionspage_fileexists.h \
		 settings/optionspage_filelists.h \
		 settings/optionspage_filetype.h \
		 settings/optionspage_ftpproxy.h \
		 settings/optionspage_interface.h \
		 settings/optionspage_language.h \
		 settings/optionspage_logging.h \
		 settings/optionspage_proxy.h \
		 settings/optionspage_sizeformatting.h \
		 settings/optionspage_themes.h \
		 settings/optionspage_transfer.h \
		 settings/optionspage_updatecheck.h \
		 settings/settingsdialog.h \
		 sftp_crypt_info_dlg.h \
		 sitemanager.h \
		 sizeformatting.h \
		 speedlimits_dialog.h \
		 splitter.h \
		 state.h \
		 statuslinectrl.h \
		 statusbar.h \
		 StatusView.h \
		 systemimagelist.h \
		 themeprovider.h \
		 toolbar.h \
		 treectrlex.h \
		 updatewizard.h \
		 verifycertdialog.h \
		 verifyhostkeydialog.h \
		 view.h \
		 viewheader.h \
		 volume_enumerator.h \
		 welcome_dialog.h \
		 window_state_manager.h \
		 wrapengine.h \
		 xh_menu_ex.h \
		 xh_toolb_ex.h \
		 xmlfunctions.h

if USE_RESOURCEFILE
RESOURCEFILE = resources/filezilla.o
endif

filezilla_DEPENDENCIES = ../engine/libengine.a

filezilla_CPPFLAGS = $(WX_CPPFLAGS)
filezilla_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_CFLAGS = $(WX_CFLAGS_ONLY)

filezilla_LDFLAGS = ../engine/libengine.a
filezilla_LDFLAGS += $(TINYXML_LIBS)
filezilla_LDFLAGS += $(LIBGNUTLS_LIBS)

if HAVE_DBUS
filezilla_DEPENDENCIES += ../dbus/libfzdbus.a
filezilla_LDFLAGS += ../dbus/libfzdbus.a $(LIBDBUS_LIBS)
endif

filezilla_LDFLAGS += $(WX_LIBS)
filezilla_LDFLAGS += $(RESOURCEFILE)
filezilla_LDFLAGS += $(IDN_LIB)

if MINGW
filezilla_LDFLAGS += -lole32 -luuid -lnetapi32 -lmpr
endif

if MINGW
filezilla_DEPENDENCIES += resources/filezilla.o
endif

if HAVE_LIBTINYXML
else
filezilla_DEPENDENCIES += $(TINYXML_LIBS)
endif

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

CLEANFILES = filezilla.h.gch

if ENABLE_PRECOMP

BUILT_SOURCES = filezilla.h.gch

include ./$(DEPDIR)/filezilla.Po

filezilla.h.gch: filezilla.h
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(filezilla_CPPFLAGS) $(CPPFLAGS) $(filezilla_CXXFLAGS) $(CXXFLAGS) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o "$@" $<; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

endif

