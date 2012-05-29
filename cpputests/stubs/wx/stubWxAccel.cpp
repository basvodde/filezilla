
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/accel.h"

wxAcceleratorTable::wxAcceleratorTable(int n, const wxAcceleratorEntry entries[])
{
	FAIL("wxAcceleratorTable::wxAcceleratorTable");
}

wxAcceleratorTable::wxAcceleratorTable()
{
}

wxAcceleratorTable::~wxAcceleratorTable()
{
}

wxClassInfo *wxAcceleratorTable::GetClassInfo() const
{
	FAIL("wxAcceleratorTable::GetClassInfo");
	return NULL;
}
