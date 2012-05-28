
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "IOKit/pwr_mgt/IOPMLib.h"

IOReturn IOPMAssertionCreate(CFStringRef AssertionType,  IOPMAssertionLevel AssertionLevel,
		IOPMAssertionID *AssertionID)
{
	FAIL("IOPMAssertionCreate");
	return 0;
}

IOReturn IOPMAssertionRelease(IOPMAssertionID AssertionID)
{
	FAIL("IOPMAssertionRelease");
	return 0;
}
