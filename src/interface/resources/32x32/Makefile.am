resourcesdir = $(pkgdatadir)/resources/32x32

dist_resources_DATA = file.png

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	mkdir -p ../../FileZilla.app/Contents/MacOS/resources/32x32
	cp -f $(dist_resources_DATA) ../../FileZilla.app/Contents/MacOS/resources/32x32
endif
