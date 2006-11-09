#include "FileZilla.h"
#include "tlssocket.h"
#include "ControlSocket.h"

#include <gnutls/gnutls.h>

BEGIN_EVENT_TABLE(CTlsSocket, wxEvtHandler)
EVT_SOCKET(wxID_ANY, CTlsSocket::OnSocketEvent)
END_EVENT_TABLE()

CTlsSocket::CTlsSocket(CControlSocket* pOwner)
	: m_pOwner(pOwner)
{
	m_session = 0;
	m_initialized = false;
	m_certCredentials = 0;
	
	m_pSocket = 0;
	m_pEvtHandler = 0;

	m_canReadFromSocket = true;
	m_canWriteToSocket = true;

	m_canTriggerRead = true;
	m_canTriggerWrite = true;

	m_tlsState = noconn;

	m_lastError = 0;
	m_lastCount = 0;
	m_lastSuccessful = false;

	m_lastWriteFailed = false;
	m_writeSkip = 0;

	m_peekData = 0;
	m_peekDataLen = 0;

	m_socketClosed = false;
}

CTlsSocket::~CTlsSocket()
{
	Uninit();
}

bool CTlsSocket::Init()
{
	// This function initializes GnuTLS
	int res = gnutls_global_init();
	if (res)
	{
		LogError(res);
		return false;
	}
	m_initialized = true;

	res = gnutls_certificate_allocate_credentials(&m_certCredentials);
	if (res < 0)
	{
		LogError(res);
		Uninit();
		return false;
	}

	res = gnutls_init(&m_session, GNUTLS_CLIENT);
	if (res)
	{
		LogError(res);
		Uninit();
		return false;
	}

	res = gnutls_set_default_priority(m_session);
	if (res)
	{
		LogError(res);
		Uninit();
		return false;
	}
	
	// Set which type of certificates we accept
	const int cert_type_priority[] = { GNUTLS_CRT_X509, 0 };
	gnutls_certificate_type_set_priority(m_session, cert_type_priority);
	if (res)
	{
		LogError(res);
		Uninit();
		return false;
	}

	gnutls_credentials_set(m_session, GNUTLS_CRD_CERTIFICATE, m_certCredentials);

	// Setup transport functions
	gnutls_transport_set_push_function(m_session, PushFunction);
	gnutls_transport_set_pull_function(m_session, PullFunction);
	gnutls_transport_set_ptr(m_session, (gnutls_transport_ptr_t)this);
	gnutls_transport_set_lowat(m_session, 0);

	// At this point, we can start shaking hands.

	return true;
}

void CTlsSocket::Uninit()
{
	if (m_session)
	{
		gnutls_deinit(m_session);
		m_session = 0;
	}

	if (m_certCredentials)
	{
		gnutls_certificate_free_credentials(m_certCredentials);
		m_certCredentials = 0;
	}

	if (m_initialized)
	{
		m_initialized = false;
		gnutls_global_deinit();
	}

	m_tlsState = noconn;

	delete [] m_peekData;
	m_peekData = 0;
	m_peekDataLen = 0;
}

void CTlsSocket::LogError(int code)
{
	if (code == GNUTLS_E_WARNING_ALERT_RECEIVED || code == GNUTLS_E_FATAL_ALERT_RECEIVED)
		PrintAlert();

	const char* error = gnutls_strerror(code);

	if (error)
	{
#if wxUSE_UNICODE
		wxString str(error, wxConvLocal);
#else
		wxString str(error);
#endif
		m_pOwner->LogMessage(::Debug_Warning, _T("GnuTLS error %d: %s"), code, str.c_str());
	}
	else
		m_pOwner->LogMessage(::Debug_Warning, _T("GnuTLS error %d"), code);
}

void CTlsSocket::PrintAlert()
{
	gnutls_alert_description_t last_alert = gnutls_alert_get(m_session);
	const char* alert = gnutls_alert_get_name(last_alert);
	if (alert)
	{
#if wxUSE_UNICODE
		wxString str(alert, wxConvLocal);
#else
		wxString str(alert);
#endif
		m_pOwner->LogMessage(::Debug_Warning, _T("GnuTLS alert %d: %s"), last_alert, str.c_str());
	}
	else
		m_pOwner->LogMessage(::Debug_Warning, _T("GnuTLS alert %d"), last_alert);
}

ssize_t CTlsSocket::PushFunction(gnutls_transport_ptr_t ptr, const void* data, size_t len)
{
	return ((CTlsSocket*)ptr)->PushFunction(data, len);
}

ssize_t CTlsSocket::PullFunction(gnutls_transport_ptr_t ptr, void* data, size_t len)
{
	return ((CTlsSocket*)ptr)->PullFunction(data, len);
}

ssize_t CTlsSocket::PushFunction(const void* data, size_t len)
{
	if (!m_canWriteToSocket)
	{
		gnutls_transport_set_errno(m_session, EAGAIN);
		return -1;
	}

	if (!m_pSocket)
	{
		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	m_pSocket->Write(data, len);

	if (m_pSocket->Error())
	{
		if (m_pSocket->LastError() == wxSOCKET_WOULDBLOCK)
		{
			m_canWriteToSocket = false;
			gnutls_transport_set_errno(m_session, EAGAIN);
			return -1;
		}

		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	return m_pSocket->LastCount();
}

ssize_t CTlsSocket::PullFunction(void* data, size_t len)
{
	if (!m_pSocket)
	{
		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	if (m_socketClosed)
		return 0;

	if (!m_canReadFromSocket)
	{
		gnutls_transport_set_errno(m_session, EAGAIN);
		return -1;
	}

	m_canReadFromSocket = false;

	m_pSocket->Read(data, len);
	if (m_pSocket->Error())
	{
		if (m_pSocket->LastError() == wxSOCKET_WOULDBLOCK)
		{
			gnutls_transport_set_errno(m_session, EAGAIN);
			return -1;
		}

		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	return m_pSocket->LastCount();
}

void CTlsSocket::SetSocket(wxSocketBase* pSocket, wxEvtHandler* pEvtHandler)
{
	m_pEvtHandler = pEvtHandler;

	m_pSocket = pSocket;
	pSocket->SetEventHandler(*this);
}

void CTlsSocket::OnSocketEvent(wxSocketEvent& event)
{
	if (!m_session)
		return;

	switch (event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		OnRead();
		break;
	case wxSOCKET_OUTPUT:
		OnSend();
		break;
	case wxSOCKET_LOST:
		{
			char tmp[100];
			m_pSocket->Peek(&tmp, 100);
			if (!m_pSocket->Error())
			{
				int lastCount = m_pSocket->LastCount();

				if (lastCount)
					m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::OnSocketEvent(): pending data, postponing wxSOCKET_LOST"));
				else
					m_socketClosed = true;
				OnRead();

				if (lastCount)
				{
					wxSocketEvent evt;
					evt.m_event = wxSOCKET_LOST;
					wxPostEvent(this, evt);
					return;
				}
			}

			m_pOwner->LogMessage(Debug_Info, _T("CTlsSocket::OnSocketEvent(): wxSOCKET_LOST received"));

			//Uninit();
			wxSocketEvent evt;
			evt.m_event = wxSOCKET_LOST;
			wxPostEvent(m_pEvtHandler, evt);
		}
		break;
	default:
		break;
	}
}

void CTlsSocket::OnRead()
{
	m_pOwner->LogMessage(Debug_Debug, _T("CTlsSocket::OnRead()"));

	m_canReadFromSocket = true;

	if (!m_session)
		return;

	if (gnutls_record_get_direction(m_session))
		return;
		
	if (m_tlsState == handshake)
		Handshake();
	if (m_tlsState == closing)
		ContinueShutdown();
	else if (m_tlsState == conn)
	{
		CheckResumeFailedWrite();
		TriggerEvents();
	}
}

void CTlsSocket::OnSend()
{
	m_pOwner->LogMessage(Debug_Debug, _T("CTlsSocket::OnSend()"));

	m_canWriteToSocket = true;

	if (!m_session)
		return;

	if (!gnutls_record_get_direction(m_session))
		return;

	if (m_tlsState == handshake)
		Handshake();
	else if (m_tlsState == closing)
		ContinueShutdown();
	else if (m_tlsState == conn)
	{
		CheckResumeFailedWrite();
		TriggerEvents();
	}
}

int CTlsSocket::Handshake()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::Handshake()"));
	wxASSERT(m_session);

	m_tlsState = handshake;

	int res = gnutls_handshake(m_session);
	if (!res)
	{
		m_pOwner->LogMessage(Debug_Info, _T("Handshake successful"));

		m_tlsState = conn;

		wxSocketEvent evt;
		evt.m_event = wxSOCKET_CONNECTION;
		wxPostEvent(m_pEvtHandler, evt);
		TriggerEvents();
		return FZ_REPLY_OK;
	}
	else if (res == GNUTLS_E_AGAIN || res == GNUTLS_E_INTERRUPTED)
		return FZ_REPLY_WOULDBLOCK;

	Failure(res);

	return FZ_REPLY_ERROR;
}

void CTlsSocket::Read(void *buffer, unsigned int len)
{
	if (m_tlsState == handshake)
	{
		m_lastError = wxSOCKET_WOULDBLOCK;
		m_lastSuccessful = false;
		return;
	}
	else if (m_tlsState != conn)
	{
		m_lastError = wxSOCKET_IOERR;
		m_lastSuccessful = false;
		return;
	}

	m_canTriggerRead = true;

	if (m_peekDataLen)
	{
		int min = wxMin(len, m_peekDataLen);
		memcpy(buffer, m_peekData, min);
		
		if (min == m_peekDataLen)
		{
			m_peekDataLen = 0;
			delete [] m_peekData;
			m_peekData = 0;
		}
		else
		{
			memmove(m_peekData, m_peekData + min, m_peekDataLen - min);
			m_peekDataLen -= min;
		}

		TriggerEvents();

		m_lastSuccessful = true;
		m_lastCount = min;
		return;
	}
	
	int res = gnutls_record_recv(m_session, buffer, len);
	if (res >= 0)
	{
		m_lastSuccessful = true;
		m_lastCount = res;

		TriggerEvents();

		return;
	}

	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
		m_lastError = wxSOCKET_WOULDBLOCK;
	else
	{
		Failure(res);
		m_lastError = wxSOCKET_IOERR;
	}

	m_lastSuccessful = false;
	return;
}

void CTlsSocket::Write(const void *buffer, unsigned int len)
{
	if (m_tlsState == handshake)
	{
		m_lastError = wxSOCKET_WOULDBLOCK;
		m_lastSuccessful = false;
		return;
	}
	else if (m_tlsState != conn)
	{
		m_lastError = wxSOCKET_IOERR;
		m_lastSuccessful = false;
		return;
	}

	if (m_lastWriteFailed)
	{
		m_lastError = wxSOCKET_WOULDBLOCK;
		m_lastSuccessful = false;
		return;
	}

	if (m_writeSkip >= len)
	{
		m_writeSkip -= len;
		m_lastCount = len;
		m_lastSuccessful = true;
		return;
	}

	len -= m_writeSkip;

	int res = gnutls_record_send(m_session, buffer, len);
	if (res >= 0)
	{
		m_lastSuccessful = true;
		m_lastCount = res + m_writeSkip;
		m_writeSkip = 0;

		TriggerEvents();
		return;
	}

	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
	{
		m_lastError = wxSOCKET_WOULDBLOCK;
		m_lastWriteFailed = true;
	}
	else
	{
		Failure(res);
		m_lastError = wxSOCKET_IOERR;
	}
	m_lastSuccessful = false;
}

void CTlsSocket::TriggerEvents()
{
	if (m_tlsState != conn)
		return;

	if (m_canTriggerRead)
	{
		wxSocketEvent evt;
		evt.m_event = wxSOCKET_INPUT;
		wxPostEvent(m_pEvtHandler, evt);
		m_canTriggerRead = false;
	}

	if (m_canTriggerWrite)
	{
		wxSocketEvent evt;
		evt.m_event = wxSOCKET_OUTPUT;
		wxPostEvent(m_pEvtHandler, evt);
		m_canTriggerWrite = false;
	}
}

void CTlsSocket::CheckResumeFailedWrite()
{
	if (m_lastWriteFailed)
	{
		int res = gnutls_record_send(m_session, 0, 0);
		if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
			return;
		
		if (res < 0)
		{
			Failure(res);
			return;
		}

		m_writeSkip += res;
		m_lastWriteFailed = false;
		m_canTriggerWrite = true;

		wxASSERT(GNUTLS_E_INVALID_REQUEST == gnutls_record_send(m_session, 0, 0));
	}
}

void CTlsSocket::Failure(int code)
{
	LogError(code);
	Uninit();

	wxSocketEvent evt;
	evt.m_event = wxSOCKET_LOST;
	wxPostEvent(m_pEvtHandler, evt);
}

void CTlsSocket::Peek(void *buffer, unsigned int len)
{
	if (m_peekData)
	{
		int min = wxMin(len, m_peekDataLen);
		memcpy(buffer, m_peekData, min);
		
		m_lastCount = min;
		m_lastSuccessful = true;
		return;
	}

	Read(buffer, len);
	if (Error() || !LastCount())
		return;

	m_peekDataLen = LastCount();
	m_peekData = new char[m_peekDataLen];
	memcpy(m_peekData, buffer, m_peekDataLen);
}

void CTlsSocket::Shutdown()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::Shutdown()"));

	if (m_tlsState == closed)
	{
		m_lastSuccessful = true;
		return;
	}

	if (m_tlsState == closing)
	{
		m_lastSuccessful = false;
		m_lastError = wxSOCKET_WOULDBLOCK;
		return;
	}

	if (m_tlsState != conn)
	{
		m_lastSuccessful = false;
		m_lastError = wxSOCKET_IOERR;
		return;
	}

	m_tlsState = closing;

	int res = gnutls_bye(m_session, GNUTLS_SHUT_WR);
	if (!res)
	{
		m_tlsState = closed;
		m_lastSuccessful = true;
		return;
	}

	m_lastSuccessful = false;
	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
		m_lastError = wxSOCKET_WOULDBLOCK;
	else
	{
		Failure(res);
		m_lastError = wxSOCKET_IOERR;
	}
}

void CTlsSocket::ContinueShutdown()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::ContinueShutdown()"));

	int res = gnutls_bye(m_session, GNUTLS_SHUT_WR);
	if (!res)
	{
		m_tlsState = closed;

		wxSocketEvent evt;
		evt.m_event = wxSOCKET_LOST;
		wxPostEvent(m_pEvtHandler, evt);

		return;
	}

	if (res != GNUTLS_E_INTERRUPTED && res != GNUTLS_E_AGAIN)
		Failure(res);
}
