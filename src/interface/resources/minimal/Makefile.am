resourcesdir = $(pkgdatadir)/resources/minimal
resources16dir = $(pkgdatadir)/resources/minimal/16x16
resources32dir = $(pkgdatadir)/resources/minimal/32x32

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
		16x16/find.png \
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
		16x16/server.png \
		16x16/showhidden.png \
		16x16/sitemanager.png \
		16x16/speedlimits.png \
		16x16/synchronize.png \
		16x16/upload.png \
		16x16/uploadadd.png

dist_resources32_DATA = 32x32/file.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/minimal/16x16";
	for i in $(dist_resources16_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/minimal/16x16"; \
	done
	mkdir -p "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/minimal/32x32";
	for i in $(dist_resources32_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/minimal/32x32"; \
	done
	for i in $(dist_resources_DATA); do \
	  cp -f "$(srcdir)/$$i" "$(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/minimal"; \
	done
endif

