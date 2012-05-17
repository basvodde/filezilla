
bin_PROGRAMS = filezilla_engine_tests filezilla_interface_tests

TESTS = $(bin_PROGRAMS)

WX_STUBS = stubs/wx/stubWxMsgdlg.cpp stubs/wx/stubWxGeometry.cpp stubs/wx/stubWxCommandEvent.cpp stubs/wx/stubWxWindow.cpp stubs/wx/stubWxFrame.cpp stubs/wx/stubWxColour.cpp stubs/wx/stubWxFont.cpp stubs/wx/stubWxGDIObject.cpp stubs/wx/stubWxRegion.cpp stubs/wx/stubWxPalette.cpp stubs/wx/stubWxAccel.cpp stubs/wx/stubWxBrush.cpp stubs/wx/stubWxCursor.cpp stubs/wx/stubWxBitmap.cpp stubs/wx/stubWxImage.cpp 
MAIN = main/main.cpp

filezilla_engine_tests_CPPFLAGS = $(WX_CPPFLAGS) -I$(srcdir)/../src/engine
filezilla_engine_tests_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_engine_tests_CFLAGS = $(WX_CFLAGS_ONLY)
filezilla_engine_tests_LDFLAGS = ../src/engine/libengine.a -lwx_base_carbonu-2.8
filezilla_engine_tests_SOURCES = $(MAIN) engine/TestEngineDirectoryCache.cpp engine/TestIOThread.cpp engine/TestSocket.cpp $(WX_STUBS) 

filezilla_interface_tests_CPPFLAGS = $(WX_CPPFLAGS) -I$(srcdir)/../src/interface -I$(srcdir)/../src/engine
filezilla_interface_tests_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_interface_tests_CFLAGS = $(WX_CFLAGS_ONLY)
filezilla_interface_tests_LDFLAGS = ../src/interface/libinterface.a -lwx_base_carbonu-2.8
filezilla_interface_tests_SOURCES = $(MAIN) interface/TestAboutDialog.cpp $(WX_STUBS)



