# these are the headers for your project

CLEANFILES = FileZilla.h.gch

noinst_HEADERS = compatibility.h \
		 commands.h directorylisting.h \
		 engineprivate.h externalipresolver.h FileZilla.h \
		 FileZillaEngine.h \
		 local_filesys.h \
		 logging.h misc.h notification.h optionsbase.h server.h \
		 serverpath.h setup.h threadex.h timeex.h

if ENABLE_PRECOMP

noinst_DATA = FileZilla.h.gch

noinst_PROGRAMS = dummy
dummy_SOURCES = dummy.cpp

dummy_CPPFLAGS = $(WX_CPPFLAGS)
dummy_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
dummy_CFLAGS = $(WX_CFLAGS_ONLY)

include ./$(DEPDIR)/FileZilla.Po

%.h.gch : %.h
	if $(CXXCOMPILE) $(WX_CPPFLAGS) $(WX_CXXFLAGS_ONLY) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ $<; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

endif
