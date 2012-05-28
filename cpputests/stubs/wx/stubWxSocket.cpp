
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/socket.h"

GSocketError GSocket::GetError()
{
	FAIL("GSocket::GetError");
	return GSOCK_NOERROR;
}


wxSocketBase::wxSocketBase()
{
	FAIL("wxSocketBase::wxSocketBase");
}

wxSocketBase::~wxSocketBase()
{
 FAIL("wxSocketBase::~wxSocketBase");
}

wxClassInfo *wxSocketBase::GetClassInfo() const
{
	FAIL("wxSocketBase::GetClassInfo");
	return NULL;
}

bool wxSocketBase::GetLocal(wxSockAddress& addr_man) const
{
	FAIL("wxSocketBase::GetLocal");
	return true;
}

bool wxSocketBase::GetPeer(wxSockAddress& addr_man) const
{
	FAIL("wxSocketBase::GetPeer");
	return true;
}

bool wxSocketBase::SetLocal(wxIPV4address& local)
{
	FAIL("wxSocketBase::SetLocal");
	return true;
}

bool wxSocketBase::Close()
{
	FAIL("wxSocketBase::Close");
	return true;
}

wxSocketBase& wxSocketBase::Read(void* buffer, wxUint32 nbytes)
{
	FAIL("wxSocketBase::Read");
	return *this;
}

void wxSocketBase::SetNotify(wxSocketEventFlags flags)
{
	FAIL("wxSocketBase::SetNotify");
}

void wxSocketBase::Notify(bool notify)
{
	FAIL("wxSocketBase::Notify");
}

void wxSocketBase::SetFlags(wxSocketFlags flags)
{
	FAIL("wxSocketBase::SetFlags");
}

wxSocketBase& wxSocketBase::Write(const void *buffer, wxUint32 nbytes)
{
	FAIL("wxSocketBase::Write");
	return *this;
}

void wxSocketBase::SetEventHandler(wxEvtHandler& handler, int id)
{
	FAIL("wxSocketBase::SetEventHandler");
}

wxSocketClient::wxSocketClient(wxSocketFlags flags)
{
	FAIL("wxSocketClient::wxSocketClient");
}

wxClassInfo *wxSocketClient::GetClassInfo() const
{
	FAIL("wxSocketClient::GetClassInfo");
	return NULL;
}

bool wxSocketClient::Connect(wxSockAddress& addr, bool wait)
{
	FAIL("wxSocketClient::Connect");
	return true;
}

wxSocketClient::~wxSocketClient()
{
	FAIL("wxSocketClient::~wxSocketClient");
}

bool wxSocketClient::DoConnect(wxSockAddress& addr, wxSockAddress* local, bool wait)
{
	FAIL("wxSocketClient::DoConnect");
	return true;
}

wxClassInfo *wxSocketServer::GetClassInfo() const
{
	FAIL("wxSocketServer::GetClassInfo");
	return NULL;
}

wxSocketServer::wxSocketServer(const wxSockAddress& addr, wxSocketFlags flags)
{
	FAIL("wxSocketServer::wxSocketServer");
}

wxSocketBase* wxSocketServer::Accept(bool wait)
{
	FAIL("wxSocketServer::Accept");
	return this;
}
