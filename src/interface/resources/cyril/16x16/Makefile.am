resourcesdir = $(pkgdatadir)/resources/cyril/16x16

dist_resources_DATA = download.png localtreeview.png server.png
		ascii.png downloadadd.png logview.png showhidden.png
		auto.png file.png processqueue.png sitemanager.png
		binary.png folder.png queueview.png speedlimits.png
		bookmark.png folderback.png reconnect.png synchronize.png
		cancel.png folderclosed.png refresh.png upload.png
		compare.png folderup.png uploadadd.png
		disconnect.png help.png remotetreeview.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/cyril/16x16
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/cyril/16x16; \
	done
endif
