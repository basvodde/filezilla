resourcesdir = $(pkgdatadir)/resources/16x16

dist_resources_DATA = cancel.png disconnect.png download.png downloadadd.png \
		file.png folder.png folderclosed.png processqueue.png \
		reconnect.png refresh.png  server.png sitemanager.png \
		upload.png uploadadd.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/16x16
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/16x16; \
	done
endif
