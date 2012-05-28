
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "option_change_event_handler.h"

void COptionChangeEventHandler::DoNotify(int option)
{
	FAIL("COptionChangeEventHandler::DoNotify");
}

void COptionChangeEventHandler::RegisterOption(int option)
{
	FAIL("COptionChangeEventHandler::RegisterOption");
}

COptionChangeEventHandler::COptionChangeEventHandler()
{
	FAIL("COptionChangeEventHandler::::COptionChangeEventHandler::");
}

COptionChangeEventHandler::~COptionChangeEventHandler()
{
	FAIL("COptionChangeEventHandler::~COptionChangeEventHandler::");
}

void COptionChangeEventHandler::UnregisterAll()
{
	FAIL("COptionChangeEventHandler::UnregisterAll");
}
