localesdir = $(pkgdatadir)/locales

CLEANFILES = *.mo resources.h

dist_noinst_DATA = filezilla.pot @FILEZILLA_LINGUAS_PO@

install-data-local: @CATALOGS_TO_INSTALL@

install-filezilla-catalogs: allmo
	for i in @FILEZILLA_LINGUAS@ ; do \
	    $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i.mo $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES/filezilla.mo ; \
	done

# Unused code
#WXWIN_LINGUAS  =    de

#install-wxstd-catalogs:
#	for i in $(WXWIN_LINGUAS) ; do \
#	    $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES ; \
#	    $(INSTALL_DATA) $(srcdir)/wxwin/$$i.mo $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES/filezilla-wxstd.mo ; \
#	done



# ----------------------------------------------------------------------------
# Logic for catalogs updating follows
# Try to not modify catalog files if nothing has changed
# But touch them to make sure the timestamps are correct
# so that the uptodate checks are only made if neccessary.
# ----------------------------------------------------------------------------

# the programs we use (TODO: use configure to detect them)
MSGFMT=msgfmt --verbose
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -kwxGetTranslation -kwxTRANSLATE -s -j

# implicit rules
%.mo: %.po
	$(MSGFMT) -o $@ $<

# a PO file must be updated from wxstd.po to include new translations
%.po: $(srcdir)/filezilla.pot
	if [ -f $@ ]; then \
		$(MSGMERGE) -U $@ $(srcdir)/filezilla.pot; \
	else \
		cp $(srcdir)/filezilla.pot $@; \
	fi
	@if [ -f $@~ ]; then rm $@~; fi

# touch file so that timestamp is newer than that of filezilla.pot
	@touch $@;

# generate resources.h from the xrc files. We have to do this since
# xgettext cannot handle xrc files. So export all translatable strings from
# the resource files into resources.h using wxrc.
resources.h: ../src/interface/resources/*.xrc
	@if [ -f $@~ ]; then rm $@~; fi

	wxrc -g -e $(srcdir)/../src/interface/resources/*.xrc > $@~ || exit 1

# strip strings not marked for translation (enclosed in <>)
	cat $@~ | sed "s/_(\"<.*>\")\;//" > $@

	@if [ -f $@~ ]; then rm $@~; fi

filezilla.pot: resources.h ../src/include/*.h ../src/engine/*.h ../src/interface/*.h  ../src/engine/*.cpp ../src/interface/*.cpp 
	@rm -f $@.new
	@touch $@.new

# create filezilla.pot.new from the source files
	(cd $(srcdir) ; $(XGETTEXT) $(XGETTEXT_ARGS) -o $@.new resources.h)
	(cd $(srcdir) ; find ../src -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@.new)
	(cd $(srcdir) ; find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@.new)

	if [ -f $@ ]; then \
		touch $@; \
		$(MSGMERGE) -U $@ $@.new; \
	else \
		mv $@.new $@; \
	fi;

	@if [ -f $@~ ]; then rm $@~; fi
	@if [ -f $@.new ]; then rm $@.new; fi

allpo: @FILEZILLA_LINGUAS_PO@

allmo: allpo
	@for t in @FILEZILLA_LINGUAS@; do $(MAKE) $(srcdir)/$$t.mo; done

# print out the percentage of the translated strings
stats: allpo
	@ALLTOTAL=0 ALLTR=0 ALLFZ=0 ALLUT=0; \
	for i in @FILEZILLA_LINGUAS@; do \
		if [ -f $$i.mo~ ]; then rm $$i.mo~; fi; \
		res=`LC_ALL="en_EN" $(MSGFMT) -o $$i.mo~ "$(srcdir)/$$i.po" 2>&1`; \
		if [ -f $$i.mo~ ]; then rm $$i.mo~; fi; \
		TR=`echo $$res | sed -e 's/^[^0-9]*\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+/\\1/'`; \
		FZ=`echo $$res | sed -e 's/^[^0-9]*\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+/\\2/'`; \
		UT=`echo $$res | sed -e 's/^[^0-9]*\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+\([0-9]\+\)[^0-9]\+/\\3/'`; \
		ALLTR=`expr $$ALLTR + $$TR`; \
		ALLFZ=`expr $$ALLFZ + $$FZ`; \
		ALLUT=`expr $$ALLUT + $$UT`; \
		TOTAL=`expr $$TR + $$FZ + $$UT`; \
		echo "\"$$i\" => \"`expr 100 "*" $$TR / $$TOTAL`%\", $$TR of $$TOTAL strings translated, $$FZ fuzzy, $$UT untranslated"; \
	done; \
	ALLTOTAL=`expr $$ALLTR + $$ALLFZ + $$ALLUT`; \
	echo "---------------------------------------"; \
	echo " ALL => \"`expr 100 "*" $$ALLTR / $$ALLTOTAL`%\", $$ALLTR of $$ALLTOTAL strings translated, $$ALLFZ fuzzy, $$ALLUT untranslated"; \
	echo $$TOTAL;

all: allmo stats

.PHONY: allpo allmo force-update stats FORCE
