localesdir = $(pkgdatadir)/locales

FILEZILLA_LINGUAS = de
WXWIN_LINGUAS  =    de

CLEANFILES = *.mo resources.h

dist_noinst_DATA = filezilla.pot de.po

install-data-local: @CATALOGS_TO_INSTALL@

install-filezilla-catalogs: allmo
	for i in $(FILEZILLA_LINGUAS) ; do \
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
# (shamelessly stolen from wxWindows makefile):
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
	if [ -f $@ ]; then $(MSGMERGE) $@ $(srcdir)/filezilla.pot > $@.new && mv $@.new $@; else cp $(srcdir)/filezilla.pot $@; fi

resources.h:
	wxrc -g -e $(srcdir)/../src/interface/resources/*.xrc > $(srcdir)/resources.h

$(srcdir)/filezilla.pot: resources.h
	touch $@
   
	(cd $(srcdir) ; $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot resources.h)
	(cd $(srcdir) ; find ../src -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot)
	(cd $(srcdir) ; find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot)

allpo: force-update
	@-for t in $(FILEZILLA_LINGUAS); do $(MAKE) $(srcdir)/$$t.po; done

allmo:
	@for t in $(FILEZILLA_LINGUAS); do $(MAKE) $(srcdir)/$$t.mo; done

force-update:
	$(RM) $(srcdir)/filezilla.pot

# print out the percentage of the translated strings
stats:
	@for i in $(FILEZILLA_LINGUAS); do \
		x=`$(MSGFMT) -o /dev/null "$(srcdir)/$$i.po" 2>&1 | sed -e 's/[,\.]//g' \
			-e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
			-e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
			-e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`; \
		TR=0 FZ=0 UT=0; \
echo $$x; \
		eval $$x; \
		TOTAL=`expr $$TR + $$FZ + $$UT`; \
		echo "\"$$i\" => \"`expr 100 "*" $$TR / $$TOTAL`%\", $$TR of $$TOTAL strings translated, $$FZ fuzzy"; \
	done

all: allmo

.PHONY: allpo allmo force-update stats FORCE
