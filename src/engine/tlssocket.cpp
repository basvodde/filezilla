#include "FileZilla.h"
#include "tlssocket.h"
#include "ControlSocket.h"

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <errno.h>

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
	m_canCheckCloseSocket = false;

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

	m_implicitTrustedCert.data = 0;
	m_implicitTrustedCert.size = 0;
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

	delete [] m_implicitTrustedCert.data;
	m_implicitTrustedCert.data = 0;
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
		const wxSocketError error = m_pSocket->LastError();
		if (error == wxSOCKET_WOULDBLOCK)
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
			if (m_canCheckCloseSocket)
			{
				wxSocketEvent evt;
				evt.m_event = wxSOCKET_LOST;
				wxPostEvent(this, evt);
			}

			gnutls_transport_set_errno(m_session, EAGAIN);
			return -1;
		}

		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	if (m_canCheckCloseSocket)
	{
		wxSocketEvent evt;
		evt.m_event = wxSOCKET_LOST;
		wxPostEvent(this, evt);
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
			m_canCheckCloseSocket = true;
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
					return;
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

int CTlsSocket::Handshake(const CTlsSocket* pPrimarySocket /*=0*/)
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::Handshake()"));
	wxASSERT(m_session);

	m_tlsState = handshake;

	if (pPrimarySocket)
	{
		// Implicitly trust certificate of primary socket
		unsigned int cert_list_size;
		const gnutls_datum_t* const cert_list = gnutls_certificate_get_peers(pPrimarySocket->m_session, &cert_list_size);
		if (cert_list && cert_list_size)
		{
			m_implicitTrustedCert.data = new unsigned char[cert_list[0].size];
			memcpy(m_implicitTrustedCert.data, cert_list[0].data, cert_list[0].size);
			m_implicitTrustedCert.size = cert_list[0].size;
		}
	}

	int res = gnutls_handshake(m_session);
	if (!res)
	{
		m_pOwner->LogMessage(Debug_Info, _T("Handshake successful"));

		res = VerifyCertificate();
		if (res != FZ_REPLY_OK)
			return res;

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
		unsigned int min = wxMin(len, m_peekDataLen);
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

		if (res > 0)
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
	if (code)
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

void CTlsSocket::TrustCurrentCert(bool trusted)
{
	if (m_tlsState != verifycert)
	{
		m_pOwner->LogMessage(Debug_Warning, _T("TrustCurrentCert called at wrong time."));
		return;
	}

	if (trusted)
	{
		m_tlsState = conn;

		wxSocketEvent evt;
		evt.m_event = wxSOCKET_CONNECTION;
		wxPostEvent(m_pEvtHandler, evt);
		TriggerEvents();
		return;
	}

	m_pOwner->LogMessage(::Error, _("Remote certificate not trusted."));
	Failure(0);
}

static wxString bin2hex(const unsigned char* in, size_t size)
{
	wxString str;
	for (size_t i = 0; i < size; i++)
	{
		if (i)
			str += _T(":");
		str += wxString::Format(_T("%.2x"), (int)in[i]);
	}

	return str;
}

int CTlsSocket::VerifyCertificate()
{
	if (m_tlsState != handshake)
	{
		m_pOwner->LogMessage(Debug_Warning, _T("VerifyCertificate called at wrong time"));
		return FZ_REPLY_ERROR;
	}

	m_tlsState = verifycert;

	if (gnutls_certificate_type_get(m_session) != GNUTLS_CRT_X509)
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("Unsupported certificate type"));
		Failure(0);
		return FZ_REPLY_ERROR;
	}

	unsigned int cert_list_size;
	const gnutls_datum_t* const cert_list = gnutls_certificate_get_peers(m_session, &cert_list_size);
	if (!cert_list || !cert_list_size)
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("gnutls_certificate_get_peers returned no certificates"));
		Failure(0);
		return FZ_REPLY_ERROR;
	}

	if (m_implicitTrustedCert.data)
	{
		if (m_implicitTrustedCert.size != cert_list[0].size ||
			memcmp(m_implicitTrustedCert.data, cert_list[0].data, cert_list[0].size))
		{
			m_pOwner->LogMessage(::Error, _("Primary connection and data connection certificates don't match."));
			Failure(0);
			return FZ_REPLY_ERROR;
		}
		
		TrustCurrentCert(true);
		return FZ_REPLY_OK;
	}

	gnutls_x509_crt_t cert;
	if (gnutls_x509_crt_init(&cert))
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("gnutls_x509_crt_init failed"));
		Failure(0);
		return FZ_REPLY_ERROR;
	}

	if (gnutls_x509_crt_import(cert, cert_list, GNUTLS_X509_FMT_DER))
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("gnutls_x509_crt_import failed"));
		Failure(0);
		gnutls_x509_crt_deinit(cert);
		return FZ_REPLY_ERROR;
	}

	wxDateTime expirationTime = gnutls_x509_crt_get_expiration_time(cert);
	wxDateTime activationTime = gnutls_x509_crt_get_activation_time(cert);

	// Get the serial number of the certificate
	unsigned char buffer[40];
	size_t size = sizeof(buffer);
	int res = gnutls_x509_crt_get_serial(cert, buffer, &size);

	wxString serial = bin2hex(buffer, size);

	unsigned int bits;
	int algo = gnutls_x509_crt_get_pk_algorithm(cert, &bits);

	wxString algoName;
	const char* pAlgo = gnutls_pk_algorithm_get_name((gnutls_pk_algorithm_t)algo);
	if (pAlgo)
		algoName = wxString(pAlgo, wxConvUTF8);

	//int version = gnutls_x509_crt_get_version(cert);
	
	wxString subject, issuer;

	size = 0;
	res = gnutls_x509_crt_get_dn(cert, 0, &size);
	if (size)
	{
		char* dn = new char[size + 1];
		dn[size] = 0;
		if (!(res = gnutls_x509_crt_get_dn(cert, dn, &size)))
		{
			dn[size] = 0;
			subject = wxString(dn, wxConvUTF8);
		}
		else
			LogError(res);
		delete [] dn;
	}
	else
		LogError(res);
	if (subject == _T(""))
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("gnutls_x509_get_dn failed"));
		Failure(0);
		gnutls_x509_crt_deinit(cert);
		return FZ_REPLY_ERROR;
	}

	size = 0;
	res = gnutls_x509_crt_get_issuer_dn(cert, 0, &size);
	if (size)
	{
		char* dn = new char[size + 1];
		dn[size] = 0;
		if (!(res = gnutls_x509_crt_get_issuer_dn(cert, dn, &size)))
		{
			dn[size] = 0;
			issuer = wxString(dn, wxConvUTF8);
		}
		else
			LogError(res);
		delete [] dn;
	}
	else
		LogError(res);
	if (issuer == _T(""))
	{
		m_pOwner->LogMessage(::Debug_Warning, _T("gnutls_x509_get_issuer_dn failed"));
		Failure(0);
		gnutls_x509_crt_deinit(cert);
		return FZ_REPLY_ERROR;
	}

	wxString fingerprint_md5;
	wxString fingerprint_sha1;

	unsigned char digest[100];
	size = sizeof(digest) - 1;
	if (!gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_MD5, digest, &size))
	{
		digest[size] = 0;
		fingerprint_md5 = bin2hex(digest, size);
	}
	size = sizeof(digest) - 1;
	if (!gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_SHA1, digest, &size))
	{
		digest[size] = 0;
		fingerprint_sha1 = bin2hex(digest, size);
	}

	CCertificateNotification *pNotification = new CCertificateNotification(
		m_pOwner->GetCurrentServer()->GetHost(),
		m_pOwner->GetCurrentServer()->GetPort(),
		cert_list->data, cert_list->size,
		activationTime, expirationTime,
		serial,
		algoName, bits,
		fingerprint_md5,
		fingerprint_sha1,
		subject,
		issuer);

	pNotification->requestNumber = m_pOwner->GetEngine()->GetNextAsyncRequestNumber();
	
	m_pOwner->GetEngine()->AddNotification(pNotification);

	m_pOwner->LogMessage(Status, _("Verifying certificate..."));

	return FZ_REPLY_WOULDBLOCK;
}
