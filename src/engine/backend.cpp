#include "FileZilla.h"
#include "backend.h"
#include "socket.h"
#include <errno.h>

int CBackend::m_nextId = 0;

CBackend::CBackend(wxEvtHandler* pEvtHandler) : m_pEvtHandler(pEvtHandler)
{
	m_Id = GetNextId();
}

int CBackend::GetNextId()
{
	const int id = m_nextId++;
	if (m_nextId < 0)
		m_nextId = 0;
	return id;
}

CSocketBackend::CSocketBackend(wxEvtHandler* pEvtHandler, CSocket* pSocket) : CBackend(pEvtHandler), m_pSocket(pSocket)
{
	m_error = false;
	m_lastCount = 0;
	m_lastError = 0;

	m_pSocket->SetEventHandler(pEvtHandler, GetId());

	CRateLimiter* pRateLimiter = CRateLimiter::Get();
	if (pRateLimiter)
		pRateLimiter->AddObject(this);
}

CSocketBackend::~CSocketBackend()
{
	m_pSocket->SetEventHandler(0, -1);

	CRateLimiter* pRateLimiter = CRateLimiter::Get();
	if (pRateLimiter)
		pRateLimiter->RemoveObject(this);	
}

void CSocketBackend::Write(const void *buffer, unsigned int len)
{
	wxLongLong max = GetAvailableBytes(CRateLimiter::outbound);
	if (max == 0)
	{
		Wait(CRateLimiter::outbound);
		m_error = true;
		m_lastError = wxSOCKET_WOULDBLOCK;
		return;
	}
	else if (max > 0 && max < len)
		len = max.GetLo();

	m_lastCount = m_pSocket->Write(buffer, len, m_lastError);
	m_error = m_lastCount == -1;

	// XXX
	if (m_lastError == EAGAIN)
		m_lastError = wxSOCKET_WOULDBLOCK;

	if (!m_error && max != -1)
		UpdateUsage(CRateLimiter::outbound, m_lastCount);
}

void CSocketBackend::Read(void *buffer, unsigned int len)
{
	wxLongLong max = GetAvailableBytes(CRateLimiter::inbound);
	if (max == 0)
	{
		Wait(CRateLimiter::inbound);
		m_error = true;
		m_lastError = wxSOCKET_WOULDBLOCK;
		return;
	}
	else if (max > 0 && max < len)
		len = max.GetLo();

	m_lastCount = m_pSocket->Read(buffer, len, m_lastError);
	m_error = m_lastCount == -1;

	// XXX
	if (m_lastError == EAGAIN)
		m_lastError = wxSOCKET_WOULDBLOCK;

	if (!m_error && max != -1)
		UpdateUsage(CRateLimiter::inbound, m_lastCount);
}

void CSocketBackend::Peek(void *buffer, unsigned int len)
{
	/*XXXm_pSocket->Peek(buffer, len);
	UpdateResults();*/
}

void CSocketBackend::OnRateAvailable(enum CRateLimiter::rate_direction direction)
{
	if (direction == CRateLimiter::outbound)
	{
		CSocketEvent evt(GetId(), CSocketEvent::write);
		m_pEvtHandler->AddPendingEvent(evt);
	}
	else
	{
		CSocketEvent evt(GetId(), CSocketEvent::read);
		m_pEvtHandler->AddPendingEvent(evt);
	}
}
