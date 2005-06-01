localesdir = $(pkgdatadir)/locales

CLEANFILES = *.po.new *.mo resources.h filezilla.pot filezilla.pot.new

dist_noinst_DATA = filezilla.pot @FILEZILLA_LINGUAS_PO@ 

install-data-local: @CATALOGS_TO_INSTALL@

install-filezilla-catalogs: allmo
	for i in @FILEZILLA_LINGUAS@ ; do \
	  $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i ; \
	  $(INSTALL_DATA) $$i.mo $(DESTDIR)$(localesdir)/$$i/filezilla.mo ; \
	done

uninstall-local:
	for i in @FILEZILLA_LINGUAS@ ; do \
	  rm $(DESTDIR)$(localesdir)/$$i/filezilla.mo ; \
	done


# Unused code
#WXWIN_LINGUAS  =    de

#install-wxstd-catalogs:
#	for i in $(WXWIN_LINGUAS) ; do \
#	  $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i ; \
#	  $(INSTALL_DATA) wxwin/$$i.mo $(DESTDIR)$(localesdir)/$$i/filezilla-wxstd.mo ; \
#	done



# ----------------------------------------------------------------------------
# Logic for catalogs updating follows
# Try to not modify catalog files if nothing has changed
# But touch them to make sure the timestamps are correct
# so that the uptodate checks are only made if neccessary.
# ----------------------------------------------------------------------------

# the programs we use (TODO: use configure to detect them)
MSGFMT=@msgfmt@ --verbose
MSGMERGE=@msgmerge@
XGETTEXT=@xgettext@

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -kwxGetTranslation -kwxTRANSLATE -s -j

# implicit rules
%.mo: %.po.new
	$(MSGFMT) -o $@ $< || exit 1

# a PO file must be updated from wxstd.po to include new translations
%.po.new: $(srcdir)/%.po filezilla.pot
	file=$@; \
	cat $(srcdir)/$${file%.po.new}.po > $@
	if [ ! -f $@ ]; then \
	  exit 1; \
	fi
	$(MSGMERGE) -U $@ filezilla.pot  || exit 1
	@if [ -f $@~ ]; then rm $@~; fi

# touch file so that timestamp is newer than that of filezilla.pot
	@touch $@;

# generate resources.h from the xrc files. We have to do this since
# xgettext cannot handle xrc files. So export all translatable strings from
# the resource files into resources.h using wxrc.
resources.h: $(srcdir)/../src/interface/resources/*.xrc
	@if [ -f $@~ ]; then rm $@~; fi

	@WXRC@ -g -e $(srcdir)/../src/interface/resources/*.xrc > $@~ || exit 1

# strip strings not marked for translation (enclosed in <>)
	cat $@~ | sed "s/_(\"<.*>\")\;//" > $@  || exit 1

	@if [ -f $@~ ]; then rm $@~; fi

filezilla.pot: resources.h $(srcdir)/../src/include/*.h $(srcdir)/../src/engine/*.h $(srcdir)/../src/interface/*.h  $(srcdir)/../src/engine/*.cpp $(srcdir)/../src/interface/*.cpp 
	@rm -f $@.new
	@touch $@.new

	FILES="resources.h `find $(srcdir)/../src -name '*.h' -o -name '*.cpp'`"; \
	$(XGETTEXT) $(XGETTEXT_ARGS) -o $@.new $$FILES || exit 1

# change charset declaration in template file to UTF-8
	cat $@.new | sed -e "s/^\"Content-Type: .*$$/\"Content-Type: text\\/plain; charset=UTF-8\\\\n\"/" > $@.new~
	@mv $@.new~ $@.new  

	if [ -f $@ ]; then \
	  touch $@; \
	  $(MSGMERGE) -U $@ $@.new || exit 1; \
	else \
	  mv $@.new $@; \
	fi;

	@if [ -f $@.pot~ ]; then rm $@~; fi
	@if [ -f $@.new ]; then rm $@.new; fi

allpo: @FILEZILLA_LINGUAS_PO_NEW@

# this target which will not be called automatically will update the .po files
updatepo: @FILEZILLA_LINGUAS_PO_NEW@
	for t in @FILEZILLA_LINGUAS@; do \
          cp $$t.po.new $(srcdir)/$$t.po || exit 1; \
        done

allmo: allpo
	@for t in @FILEZILLA_LINGUAS@; do $(MAKE) $(AM_MAKEFLAGS) $$t.mo || exit 1; done

# on the Mac copy the .mo files into the application bundle
if MACAPPBUNDLE

noinst_DATA = $(top_builddir)/FileZilla.app/Contents/SharedSupport/locales

$(top_builddir)/FileZilla.app/Contents/SharedSupport/locales: allmo
	mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/locales
	for i in @FILEZILLA_LINGUAS@ ; do \
	  mkdir -p $(top_builddir)/FileZilla.app/Contents/SharedSupport/locales/$$i; \
	  cp -f $$i.mo $(top_builddir)/FileZilla.app/Contents/SharedSupport/locales/$$i/filezilla.mo; \
	done

endif

all: allmo

.PHONY: allpo allmo force-update FORCE
