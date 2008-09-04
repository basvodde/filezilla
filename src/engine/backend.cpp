#include "FileZilla.h"
#include "backend.h"
#include "socket.h"
#include <errno.h>

int CBackend::m_nextId = 0;

CBackend::CBackend(CSocketEventHandler* pEvtHandler) : m_pEvtHandler(pEvtHandler)
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

CSocketBackend::CSocketBackend(CSocketEventHandler* pEvtHandler, CSocket* pSocket) : CBackend(pEvtHandler), m_pSocket(pSocket)
{
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

int CSocketBackend::Write(const void *buffer, unsigned int len, int& error)
{
	wxLongLong max = GetAvailableBytes(CRateLimiter::outbound);
	if (max == 0)
	{
		Wait(CRateLimiter::outbound);
		error = EAGAIN;
		return -1;
	}
	else if (max > 0 && max < len)
		len = max.GetLo();

	int written = m_pSocket->Write(buffer, len, error);
	
	if (written > 0 && max != -1)
		UpdateUsage(CRateLimiter::outbound, written);

	return written;
}

int CSocketBackend::Read(void *buffer, unsigned int len, int& error)
{
	wxLongLong max = GetAvailableBytes(CRateLimiter::inbound);
	if (max == 0)
	{
		Wait(CRateLimiter::inbound);
		error = EAGAIN;
		return -1;
	}
	else if (max > 0 && max < len)
		len = max.GetLo();

	int read = m_pSocket->Read(buffer, len, error);

	if (read > 0 && max != -1)
		UpdateUsage(CRateLimiter::inbound, read);

	return read;
}

int CSocketBackend::Peek(void *buffer, unsigned int len, int& error)
{
	return m_pSocket->Peek(buffer, len, error);
}

void CSocketBackend::OnRateAvailable(enum CRateLimiter::rate_direction direction)
{
	CSocketEvent *evt;
	if (direction == CRateLimiter::outbound)
		evt = new CSocketEvent(m_pEvtHandler, GetId(), CSocketEvent::write);
	else
		evt = new CSocketEvent(m_pEvtHandler, GetId(), CSocketEvent::read);
	
	CSocketEventDispatcher::Get().SendEvent(evt);
}
