# these are the headers for your project

noinst_LIBRARIES = libfzdbus.a

libfzdbus_a_CPPFLAGS = $(LIBDBUS_CFLAGS) $(WX_CPPFLAGS)
libfzdbus_a_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
libfzdbus_a_CFLAGS = $(WX_CFLAGS_ONLY)


libfzdbus_a_SOURCES = \
		wxdbusconnection.cpp \
		wxdbusmessage.cpp \
		power_management_inhibitor.cpp \
		session_manager.cpp

noinst_HEADERS = session_manager.h \
		wxdbusconnection.h \
		power_management_inhibitor.h \
		wxdbusmessage.h

