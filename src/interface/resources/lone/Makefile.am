resourcesdir = $(pkgdatadir)/resources/lone
resources16dir = $(pkgdatadir)/resources/lone/16x16
resources32dir = $(pkgdatadir)/resources/lone/32x32
resources48dir = $(pkgdatadir)/resources/lone/48x48

dist_resources_DATA = theme.xml

dist_resources16_DATA = 16x16/ascii.png \
		16x16/auto.png \
		16x16/binary.png \
		16x16/bookmarks.png \
		16x16/cancel.png \
		16x16/compare.png \
		16x16/disconnect.png \
		16x16/download.png \
		16x16/downloadadd.png \
		16x16/file.png \
		16x16/filter.png \
		16x16/folder.png \
		16x16/folderback.png \
		16x16/folderclosed.png \
		16x16/folderup.png \
		16x16/help.png \
		16x16/localtreeview.png \
		16x16/lock.png \
		16x16/logview.png \
		16x16/processqueue.png \
		16x16/queueview.png \
		16x16/reconnect.png \
		16x16/refresh.png \
		16x16/remotetreeview.png \
		16x16/showhidden.png \
		16x16/sitemanager.png \
		16x16/speedlimits.png \
		16x16/synchronize.png \
		16x16/upload.png \
		16x16/uploadadd.png

dist_resources32_DATA = 32x32/ascii.png \
		32x32/auto.png \
		32x32/binary.png \
		32x32/bookmarks.png \
		32x32/cancel.png \
		32x32/compare.png \
		32x32/disconnect.png \
		32x32/download.png \
		32x32/downloadadd.png \
		32x32/file.png \
		32x32/filter.png \
		32x32/folder.png \
		32x32/folderback.png \
		32x32/folderclosed.png \
		32x32/folderup.png \
		32x32/help.png \
		32x32/localtreeview.png \
		32x32/lock.png \
		32x32/logview.png \
		32x32/processqueue.png \
		32x32/queueview.png \
		32x32/reconnect.png \
		32x32/refresh.png \
		32x32/remotetreeview.png \
		32x32/showhidden.png \
		32x32/sitemanager.png \
		32x32/speedlimits.png \
		32x32/synchronize.png \
		32x32/upload.png \
		32x32/uploadadd.png

dist_resources48_DATA = 48x48/ascii.png \
		48x48/auto.png \
		48x48/binary.png \
		48x48/bookmarks.png \
		48x48/cancel.png \
		48x48/compare.png \
		48x48/disconnect.png \
		48x48/download.png \
		48x48/downloadadd.png \
		48x48/file.png \
		48x48/filter.png \
		48x48/folder.png \
		48x48/folderback.png \
		48x48/folderclosed.png \
		48x48/folderup.png \
		48x48/help.png \
		48x48/localtreeview.png \
		48x48/lock.png \
		48x48/logview.png \
		48x48/processqueue.png \
		48x48/queueview.png \
		48x48/reconnect.png \
		48x48/refresh.png \
		48x48/remotetreeview.png \
		48x48/showhidden.png \
		48x48/sitemanager.png \
		48x48/speedlimits.png \
		48x48/synchronize.png \
		48x48/upload.png \
		48x48/uploadadd.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/16x16";
	for i in $(dist_resources16_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/16x16"; \
	done
	mkdir -p "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/32x32";
	for i in $(dist_resources32_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/32x32"; \
	done
	mkdir -p "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/48x48";
	for i in $(dist_resources48_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone/48x48"; \
	done
	for i in $(dist_resources_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/lone"; \
	done
endif

