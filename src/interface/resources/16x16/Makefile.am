resourcesdir = $(pkgdatadir)/resources/16x16

dist_resources_DATA = file.png folder.png folderclosed.png server.png \
		sitemanager.png processqueue.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p ../../FileZilla.app/Contents/MacOS/resources/16x16
	cp -f $(dist_resources_DATA) ../../FileZilla.app/Contents/MacOS/resources/16x16
endif
