resourcesdir = $(pkgdatadir)/resources/cyril/16x16

dist_resources_DATA = ascii.png auto.png binary.png bookmark.png cancel.png \
		compare.png disconnect.png download.png downloadadd.png file.png \
		folder.png folderback.png folderclosed.png folderup.png help.png \
		localtreeview.png logview.png processqueue.png queueview.png \
		reconnect.png refresh.png remotetreeview.png server.png \
		showhidden.png sitemanager.png speedlimits.png synchronize.png \
		upload.png uploadadd.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/cyril/16x16
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/cyril/16x16; \
	done
endif
