resourcesdir = $(pkgdatadir)/resources/16x16

dist_resources_DATA = file.png folder.png folderclosed.png server.png \
		sitemanager.png processqueue.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/16x16
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/16x16; \
	done
endif
