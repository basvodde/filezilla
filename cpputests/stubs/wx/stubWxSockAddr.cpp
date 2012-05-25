
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/socket.h"
#include "wx/sckaddr.h"

wxSockAddress::wxSockAddress()
{
	FAIL("wxSockAddress::wxSockAddress");
}

wxSockAddress::~wxSockAddress()
{
	FAIL("wxSockAddress::~wxSockAddress");
}

wxClassInfo *wxSockAddress::GetClassInfo() const
{
	FAIL("wxSockAddress::GetClassInfo");
	return NULL;
}

void wxSockAddress::Clear()
{
	FAIL("wxSockAddress::Clear");
}

wxIPaddress::wxIPaddress()
{
	FAIL("wxIPaddress::wxIPaddress");
}

wxIPaddress::~wxIPaddress()
{
	FAIL("wxIPaddress::~wxIPaddress");
}

wxClassInfo *wxIPaddress::GetClassInfo() const
{
	FAIL("wxIPaddress::GetClassInfo");
	return NULL;
}

wxIPV4address::wxIPV4address()
{
	FAIL("wxIPV4address::wxIPV4address")
}

wxIPV4address::~wxIPV4address()
{
	FAIL("wxIPV4address::~wxIPV4address");
}

wxClassInfo *wxIPV4address::GetClassInfo() const
{
	FAIL("wxIPV4address::GetClassInfo");
	return NULL;
}

bool wxIPV4address::Hostname(const wxString& name)
{
	FAIL("wxIPV4address::Hostname");
	return true;
}

bool wxIPV4address::Hostname(unsigned long addr)
{
	FAIL("wxIPV4address::Hostname");
	return true;
}

bool wxIPV4address::Service(const wxString& name)
{
	FAIL("wxIPV4address::Service");
	return true;
}

bool wxIPV4address::Service(unsigned short port)
{
	FAIL("wxIPV4address::Service");
	return true;
}

bool wxIPV4address::LocalHost()
{
	FAIL("wxIPV4address::LocalHost");
	return true;
}

bool wxIPV4address::IsLocalHost() const
{
	FAIL("wxIPV4address::IsLocalHost");
	return true;
}

bool wxIPV4address::AnyAddress()
{
	FAIL("wxIPV4address::AnyAddress");
	return true;
}

wxString wxIPV4address::Hostname() const
{
	FAIL("wxIPV4address::Hostname");
	return L"";
}

unsigned short wxIPV4address::Service() const
{
	FAIL("wxIPV4address::Service");
	return 0;
}

wxString wxIPV4address::IPAddress() const
{
	FAIL("wxIPV4address::IPAddress");
	return L"";
}

wxSockAddress *wxIPV4address::Clone() const
{
	FAIL("wxIPV4address::Clone");
	return NULL;
}





