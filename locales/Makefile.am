localesdir = $(pkgdatadir)/locales

WXWIN_LINGUAS  =    de

CLEANFILES = *.mo resources.h

dist_noinst_DATA = filezilla.pot @FILEZILLA_LINGUAS_PO@

install-data-local: @CATALOGS_TO_INSTALL@

install-filezilla-catalogs: allmo
	for i in @FILEZILLA_LINGUAS@ ; do \
	    $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i.mo $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES/filezilla.mo ; \
	done

install-wxstd-catalogs:
	for i in $(WXWIN_LINGUAS) ; do \
	    $(mkinstalldirs) $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/wxwin/$$i.mo $(DESTDIR)$(localesdir)/$$i/LC_MESSAGES/filezilla-wxstd.mo ; \
	done



# ----------------------------------------------------------------------------
# Logic for catalogs updating follows
# Try to not touch catalog files if nothing has changed
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
	if [ -f $@ ]; then $(MSGMERGE) -U $@ $(srcdir)/filezilla.pot; else cp $(srcdir)/filezilla.pot $@; fi
	@if [ -f $@~ ]; then rm $@~; fi

resources.h: ../src/interface/resources/*.xrc
	@if [ -f $@~ ]; then rm $@~; fi
	wxrc -g -e $(srcdir)/../src/interface/resources/*.xrc > $@~ || exit 1
	cat $@~ | sed "s/_(\"<.*>\")\;//" > $@
	@if [ -f $@~ ]; then rm $@~; fi

filezilla.pot: resources.h ../src/include/*.h ../src/engine/*.h ../src/interface/*.h  ../src/engine/*.cpp ../src/interface/*.cpp 
	@rm -f filezilla_new.pot
	@touch filezilla_new.pot

	(cd $(srcdir) ; $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla_new.pot resources.h)
	(cd $(srcdir) ; find ../src -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla_new.pot)
	(cd $(srcdir) ; find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla_new.pot)

	if [ -f $@ ]; then \
		touch $@; \
		$(MSGMERGE) -U $@ filezilla_new.pot; \
		if [ -f $@~ ]; then rm $@~; fi; \
		rm filezilla_new.pot; \
	else \
		mv filezilla_new.pot $@; \
	fi;

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
