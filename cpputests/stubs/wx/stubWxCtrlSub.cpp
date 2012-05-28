

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/ctrlsub.h"

IMPLEMENT_ABSTRACT_CLASS(wxControlWithItems, wxControl);


wxItemContainer::~wxItemContainer()
{
	FAIL("wxItemContainer::~wxItemContainer");
}

void wxItemContainer::SetClientObject(unsigned int n, wxClientData* clientData)
{
	FAIL("wxItemContainer::SetClientObject");
}

wxClientData* wxItemContainer::GetClientObject(unsigned int n) const
{
	FAIL("wxItemContainer::GetClientObject");
	return NULL;
}


wxItemContainerImmutable::~wxItemContainerImmutable()
{
	FAIL("wxItemContainerImmutable::~wxItemContainerImmutable");
}

bool wxItemContainerImmutable::SetStringSelection(const wxString& s)
{
	FAIL("wxItemContainerImmutable::SetStringSelection");
	return true;
}

 wxString wxItemContainerImmutable::GetStringSelection() const
 {
	 FAIL("wxItemContainerImmutable::GetStringSelection");
	 return L"";
 }


wxControlWithItems::~wxControlWithItems()
{
	FAIL("wxControlWithItems::~wxControlWithItems");
}
