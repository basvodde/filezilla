

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/ctrlsub.h"

wxItemContainer::~wxItemContainer()
{
	FAIL("wxItemContainer::~wxItemContainer");
}

wxItemContainerImmutable::~wxItemContainerImmutable()
{
	FAIL("wxItemContainerImmutable::~wxItemContainerImmutable");
}

wxClassInfo *wxControlWithItems::GetClassInfo() const
{
	FAIL("wxControlWithItems::GetClassInfo");
	return NULL;
}

wxControlWithItems::~wxControlWithItems()
{
	FAIL("wxControlWithItems::~wxControlWithItems");
}
