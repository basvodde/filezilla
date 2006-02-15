#include "FileZilla.h"
#include "asynchostresolver.h"

const wxEventType fzEVT_ASYNCHOSTRESOLVE = wxNewEventType();

fzAsyncHostResolveEvent::fzAsyncHostResolveEvent(int id)
	: wxEvent(id, fzEVT_ASYNCHOSTRESOLVE)
{
}

wxEvent* fzAsyncHostResolveEvent::Clone() const
{
	return new fzAsyncHostResolveEvent(GetId());
}

CAsyncHostResolver::CAsyncHostResolver(wxEvtHandler *pOwner, wxString hostname) : wxThread(wxTHREAD_JOINABLE)
{
	m_bObsolete = false;
	m_pOwner = pOwner;
	m_Hostname = hostname;
	m_bSuccessful = false;
	m_bDone = false;
}

CAsyncHostResolver::~CAsyncHostResolver()
{
}

wxThread::ExitCode CAsyncHostResolver::Entry()
{
	m_bSuccessful = m_Address.Hostname(m_Hostname);

	SendReply();
	
	return 0;
}

void CAsyncHostResolver::SetObsolete()
{
	m_bObsolete = true;
}

void CAsyncHostResolver::SendReply()
{
	m_bDone = true;
	if (m_pOwner)
		wxPostEvent(m_pOwner, fzAsyncHostResolveEvent());
}

bool CAsyncHostResolver::Done() const
{
	return m_bDone;
}

bool CAsyncHostResolver::Obsolete() const
{
	return m_bObsolete;
}

bool CAsyncHostResolver::Successful() const
{
	return m_bSuccessful;
}
