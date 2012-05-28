

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "externalipresolver.h"

DEFINE_EVENT_TYPE(fzEVT_EXTERNALIPRESOLVE);

wxString CExternalIPResolver::m_ip = L"";;

CExternalIPResolver::CExternalIPResolver(wxEvtHandler* handler, int id)
{
	FAIL("CExternalIPResolver::CExternalIPResolver");
}

CExternalIPResolver::~CExternalIPResolver()
{
	FAIL("CExternalIPResolver::~CExternalIPResolver");
}

void CExternalIPResolver::GetExternalIP(const wxString& address, enum CSocket::address_family protocol, bool force)
{
	FAIL("CExternalIPResolver::GetExternalIP");
}

void CExternalIPResolver::OnSocketEvent(CSocketEvent& event)
{
	FAIL("CExternalIPResolver::OnSocketEvent");
}
