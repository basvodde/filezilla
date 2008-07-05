# Rules for the test code (use `make check` to execute)

INCLUDES = -I$(top_srcdir)/src/include
INCLUDES += -I$(top_srcdir)/src/engine

TESTS = test
check_PROGRAMS = $(TESTS)

test_SOURCES =  test.cpp \
		ipaddress.cpp \
		dirparsertest.cpp \
		serverpathtest.cpp

test_CPPFLAGS = $(WX_CPPFLAGS)
test_CXXFLAGS = $(WX_CXXFLAGS_ONLY) $(CPPUNIT_CFLAGS)

test_LDFLAGS = $(CPPUNIT_LIBS)
test_LDFLAGS += ../src/engine/libengine.a 
test_LDFLAGS += $(LIBGNUTLS_LIBS)
test_LDFLAGS += $(WX_LIBS)
test_LDFLAGS += $(IDN_LIB)

test_DEPENDENCIES = ../src/engine/libengine.a
