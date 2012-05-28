
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "socket.h"


CSocketEventHandler::~CSocketEventHandler()
{
	FAIL("CSocketEventHandler::~CSocketEventHandler");
}
