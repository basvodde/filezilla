

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/choice.h"

const wxChar wxChoiceNameStr[100] = L"";

wxChoiceBase::~wxChoiceBase()
{
	FAIL("wxChoiceBase::~wxChoiceBase");
}

void wxChoiceBase::Command(wxCommandEvent& event)
{
	FAIL("wxChoiceBase::Command");
}

wxChoice::~wxChoice()
{
	FAIL("wxChoice::~wxChoice");
}


wxClassInfo wxChoice::ms_classInfo(NULL, NULL, NULL, 0, NULL);

wxClassInfo *wxChoice::GetClassInfo() const
{
	FAIL("wxChoice::GetClassInfo");
	return NULL;
}

void wxChoice:: Delete(unsigned int n)
{
	FAIL("wxChoice::Delete");
}

void wxChoice:: Clear()
{
	FAIL("wxChoice::Clear");
}

unsigned int wxChoice::GetCount() const
{
	FAIL("wxChoice::GetCount");
	return 0;
}

int wxChoice:: GetSelection() const
{
	FAIL("wxChoice::GetSelection");
	return 0;
}

void wxChoice:: SetSelection(int n)
{
	FAIL("wxChoice::SetSelection");
}

int wxChoice:: FindString(const wxString& s, bool bCase) const
{
	FAIL("wxChoice::FindString");
	return 0;
}

wxString wxChoice::GetString(unsigned int n) const
{
	FAIL("wxChoice::GetString");
	return L"";
}

void wxChoice:: SetString(unsigned int pos, const wxString& s)
{
	FAIL("wxChoice::SetString");
}

wxInt32 wxChoice::MacControlHit( WXEVENTHANDLERREF handler , WXEVENTREF event )
{
	FAIL("wxChoice::MacControlHit");
	return 0;
}

wxSize wxChoice::DoGetBestSize() const
{
	FAIL("wxChoice::DoGetBestSize");
	return wxSize();
}

int wxChoice:: DoAppend(const wxString& item)
{
	FAIL("wxChoice::DoAppend");
	return 0;
}

int wxChoice:: DoInsert(const wxString& item, unsigned int pos)
{
	FAIL("wxChoice::DoInsert");
	return 0;
}

void wxChoice:: DoSetItemClientData(unsigned int n, void* clientData)
{
	FAIL("wxChoice::DoSetItemClientData");
}

void* wxChoice::DoGetItemClientData(unsigned int n) const
{
	FAIL("wxChoice::DoGetItemClientData");
	return NULL;
}

void wxChoice:: DoSetItemClientObject(unsigned int n, wxClientData* clientData)
{
	FAIL("wxChoice::DoSetItemClientObject");
}

wxClientData* wxChoice::DoGetItemClientObject(unsigned int n) const
{
	FAIL("wxChoice::DoGetItemClientObject");
	return NULL;
}

bool wxChoice::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, const wxArrayString& choices,
		long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxChoice::Create");
	return true;
}

