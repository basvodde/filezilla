#include "FileZilla.h"
#include "asynchostresolver.h"
#include "engine_private.h"

CAsyncHostResolver::CAsyncHostResolver(CFileZillaEnginePrivate *pOwner, wxString hostname) : wxThread(wxTHREAD_JOINABLE)
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
	{
		m_pOwner->SendEvent(engineHostresolve);
	}
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
