#include "FileZilla.h"
#include "backend.h"

int CBackend::m_nextId = 0;

CBackend::CBackend(wxEvtHandler* pEvtHandler) : m_pEvtHandler(pEvtHandler)
{
	m_Id = m_nextId++;
	if (m_nextId < 0)
		m_nextId = 0;
}

CSocketBackend::CSocketBackend(wxEvtHandler* pEvtHandler, wxSocketBase* pSocket) : CBackend(pEvtHandler), m_pSocket(pSocket)
{
	m_error = false;
	m_lastCount = 0;
	m_lastError = 0;

	m_pSocket->SetEventHandler(*pEvtHandler, GetId());
	m_pSocket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
	m_pSocket->Notify(true);

	CRateLimiter* pRateLimiter = CRateLimiter::Get();
	if (pRateLimiter)
		pRateLimiter->AddObject(this);
}

CSocketBackend::~CSocketBackend()
{
	m_pSocket->Notify(false);

	CRateLimiter* pRateLimiter = CRateLimiter::Get();
	if (pRateLimiter)
		pRateLimiter->RemoveObject(this);
}

void CSocketBackend::UpdateResults()
{
	if ((m_error = m_pSocket->Error()))
		m_lastError = m_pSocket->LastError();
	else
		m_lastCount = m_pSocket->LastCount();
}

void CSocketBackend::Write(const void *buffer, unsigned int len)
{
	int max = GetAvailableBytes(CRateLimiter::outbound);
	if (!max)
	{
		Wait(CRateLimiter::outbound);
		m_error = true;
		m_lastError = wxSOCKET_WOULDBLOCK;
		return;
	}
	else if (max > 0 && (unsigned int)max < len)
		len = max;

	m_pSocket->Write(buffer, len);
	UpdateResults();

	if (!m_error && max != -1)
		UpdateUsage(CRateLimiter::outbound, m_lastCount);
}

void CSocketBackend::Read(void *buffer, unsigned int len)
{
	int max = GetAvailableBytes(CRateLimiter::inbound);
	if (!max)
	{
		Wait(CRateLimiter::inbound);
		m_error = true;
		m_lastError = wxSOCKET_WOULDBLOCK;
		return;
	}
	else if (max > 0 && (unsigned int)max < len)
		len = max;

	m_pSocket->Read(buffer, len);
	UpdateResults();

	if (!m_error && max != -1)
		UpdateUsage(CRateLimiter::inbound, m_lastCount);
}

void CSocketBackend::Peek(void *buffer, unsigned int len)
{
	m_pSocket->Peek(buffer, len);
	UpdateResults();
}

void CSocketBackend::OnRateAvailable(enum CRateLimiter::rate_direction direction)
{
	wxSocketEvent evt;
	evt.SetId(GetId());
	if (direction == CRateLimiter::outbound)
		evt.m_event = wxSOCKET_OUTPUT;
	else
		evt.m_event = wxSOCKET_INPUT;
	wxPostEvent(m_pEvtHandler, evt);
}
