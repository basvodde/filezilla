CLEANFILES = filezilla.o

resourcesdir = $(pkgdatadir)/resources

dist_resources_DATA = menus.xrc quickconnectbar.xrc dropdown.png \
		disconnect.png toolbar.xrc cancel.png \
		empty.xpm up.xpm down.xpm filezilla.xpm

if USE_RESOURCEFILE
noinst_DATA = filezilla.o
filezilla.o:
	windres $(CPPFLAGS) filezilla.rc filezilla.o
endif


dist_noinst_DATA = filezilla.rc FileZilla.ico
