# this is the makefile for generating filezilla.pot message catalog file and
# building lang.mo files from the translated lang.po catalogs

# this makefile may be invoked to build either filezilla.pot or any lang.mo

# Autodetect the languages we support.  Currently this relies on make
# being called with this dir as the cwd, but if we generate this file
# with configure an explicit path should be specified -- RL.


# the programs we use (TODO: use configure to detect them)
MSGFMT=msgfmt --verbose
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

# used if running under cygwin due to the windows find conflicting
FIND="/bin/find"

FZ_LINGUAS := `$(FIND) . -name *.po 2> /dev/null | sed -n 's/\(wxstd\)\?\.po//p'`

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -kwxGetTranslation -kwxTRANSLATE -s -j

# implicit rules
%.mo: %.po
	$(MSGFMT) -o $@ $<

# a PO file must be updated from filezilla.pot to include new translations
%.po: filezilla.pot
	if [ -f $@ ]; then $(MSGMERGE) $@ filezilla.pot > $@.new && mv $@.new $@; else cp filezilla.pot $@; fi

#	If running under Windows, restore windows line endings to not confuse CVS
	if echo $(OS) | grep Windows; then dos2unix $@; unix2dos $@; fi

ressources:
	touch ../src/interface/ressources/ressources.h
	rm ../src/interface/ressources/ressources.h
	$(FIND) ../src/interface/ressources -name "*.xrc" | $(XARGS) wxrc -g >> ../src/interface/ressources/ressources.h

filezilla.pot: ressources
	touch $@
	$(FIND) ../src/include -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot
	$(FIND) ../src/engine -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot
	$(FIND) ../src/engine -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot
	$(FIND) ../src/interface -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot
	$(FIND) ../src/interface -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot
	$(FIND) ../src/interface/ressources -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o filezilla.pot

allpo: force-update
	@-for t in $(FZ_LINGUAS); do $(MAKE) $$t.po; done

allmo:
	@for t in $(FZ_LINGUAS); do $(MAKE) $$t.mo; done

force-update:
	$(RM) filezilla.pot

# print out the percentage of the translated strings
stats:
	@for i in $(FZ_LINGUAS); do \
		x=`$(MSGFMT) -o /dev/null "./$$i.po" 2>&1 | sed -e 's/[,\.]//g' \
			-e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
			-e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
			-e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`; \
		TR=0 FZ=0 UT=0; \
		eval $$x; \
		TOTAL=`expr $$TR + $$FZ + $$UT`; \
		echo "\"$$i\" => \"`expr 100 "*" $$TR / $$TOTAL`\", /* $$TOTAL strings */"; \
	done

all: allpo allmo force-update percentage FORCE

.PHONY: allpo allmo force-update percentage FORCE

# $Id$
