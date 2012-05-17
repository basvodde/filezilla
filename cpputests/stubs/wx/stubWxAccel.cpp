
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/accel.h"

wxAcceleratorTable::wxAcceleratorTable()
{
	FAIL("wxAcceleratorTable::wxAcceleratorTable");
}

wxAcceleratorTable::~wxAcceleratorTable()
{
	FAIL("wxAcceleratorTable::~wxAcceleratorTable");
}

wxClassInfo *wxAcceleratorTable::GetClassInfo() const
{
	FAIL("wxAcceleratorTable::GetClassInfo");
	return NULL;
}
