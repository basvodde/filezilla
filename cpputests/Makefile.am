
bin_PROGRAMS = filezilla_tests

INCLUDES = -I$(srcdir)/../src/engine

filezilla_tests_CPPFLAGS = $(WX_CPPFLAGS)
filezilla_tests_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_tests_CFLAGS = $(WX_CFLAGS_ONLY)

filezilla_tests_LDFLAGS = ../src/engine/libengine.a -lwx_base_carbonu-2.8

filezilla_tests_SOURCES = main.cpp TestEngineDirectoryCache.cpp stubWxMsgdlg.cpp stubWxGeometry.cpp

TESTS = $(bin_PROGRAMS)
