

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
