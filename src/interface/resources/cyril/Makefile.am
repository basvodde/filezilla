SUBDIRS = 16x16

CLEANFILES = filezilla.o

resourcesdir = $(pkgdatadir)/resources/cyril

dist_resources_DATA = 

dist_noinst_DATA = 

if MACAPPBUNDLE
mac-copy-resources: $(dist_resources_DATA)
	for i in $(dist_resources_DATA); \
		do cp -f $(srcdir)/$$i $(top_builddir)/FileZilla.app/Contents/SharedSupport/resources/cyril; \
	done
	
	$(MAKE) -C 16x16 mac-copy-resources
	$(MAKE) -C 32x32 mac-copy-resources
endif
