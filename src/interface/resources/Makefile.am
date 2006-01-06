SUBDIRS = 16x16 32x32 cyril

CLEANFILES = filezilla.o

resourcesdir = $(pkgdatadir)/resources

dist_resources_DATA = dialogs.xrc down.png dropdown.png empty.png \
		filezilla.xpm leds.png menus.xrc netconfwizard.xrc \
		quickconnectbar.xrc \
		themes.xml toolbar.xrc up.png filezilla.png	  

if USE_RESOURCEFILE
nodist_noinst_DATA = filezilla.o

filezilla.o: $(srcdir)/filezilla.rc
	@if [ -f $@~ ]; then rm $@~; fi
	@WINDRES@ --include-dir=$(srcdir) $(WINDRESFLAGS) $(srcdir)/filezilla.rc $@~
	@if [ -f $@ ]; then rm $@; fi
	mv $@~ $@
endif


dist_noinst_DATA = filezilla.rc FileZilla.ico \
		FileZilla.icns MacInfo.plist

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources; \
	done
	
	$(MAKE) -C 16x16 mac-copy-resources
	$(MAKE) -C 32x32 mac-copy-resources
	$(MAKE) -C cyril mac-copy-resources
endif
