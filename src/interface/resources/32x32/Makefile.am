resourcesdir = $(pkgdatadir)/resources/32x32

dist_resources_DATA = file.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/32x32
	cp -f $(dist_resources_DATA) $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/32x32
endif
