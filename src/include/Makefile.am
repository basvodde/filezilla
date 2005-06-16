# these are the headers for your project

CLEANFILES = FileZilla.h.gch

noinst_HEADERS = commands.h directorylisting.h engineprivate.h FileZilla.h \
		 FileZillaEngine.h \
		 logging.h notification.h optionsbase.h server.h \
		 serverpath.h setup.h timeex.h

if ENABLE_PRECOMP

noinst_DATA = FileZilla.h.gch

noinst_PROGRAMS = dummy
dummy_SOURCES = dummy.cpp

include ./$(DEPDIR)/FileZilla.Po

%.h.gch : %.h
	if $(CXXCOMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ $<; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

endif
