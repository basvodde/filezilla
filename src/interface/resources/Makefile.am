SUBDIRS = 16x16 32x32

CLEANFILES = filezilla.o

resourcesdir = $(pkgdatadir)/resources

dist_resources_DATA = menus.xrc quickconnectbar.xrc dropdown.png \
		disconnect.png toolbar.xrc cancel.png \
		empty.xpm up.xpm down.xpm filezilla.xpm \
		dialogs.xrc reconnect.png \
		refresh.png leds.png

if USE_RESOURCEFILE
noinst_DATA = filezilla.o

filezilla.o:
	windres $(WINDRESFLAGS) filezilla.rc filezilla.o
endif


dist_noinst_DATA = filezilla.rc FileZilla.ico \
		FileZilla.icns MacInfo.plist

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	cp -f `$(dist_resources_DATA)` ../FileZilla.app/Contents/MacOS/resources
endif
