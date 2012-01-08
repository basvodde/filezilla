#include <filezilla.h>
#include "tlssocket.h"
#include "ControlSocket.h"

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <errno.h>

//#define TLSDEBUG 1
#if TLSDEBUG
// This is quite ugly
CControlSocket* pLoggingControlSocket;
void log_func(int level, const char* msg)
{
	if (!msg)
		return;
	wxString s(msg, wxConvLocal);
	s.Trim();
	pLoggingControlSocket->LogMessage(Debug_Debug, _T("tls: %d %s"), level, s.c_str());
}
#endif

CTlsSocket::CTlsSocket(CSocketEventHandler* pEvtHandler, CSocket* pSocket, CControlSocket* pOwner)
	: CBackend(pEvtHandler), m_pOwner(pOwner)
{
	m_pSocket = pSocket;
	m_pSocketBackend = new CSocketBackend(this, m_pSocket);

	m_session = 0;
	m_initialized = false;
	m_certCredentials = 0;

	m_canReadFromSocket = true;
	m_canWriteToSocket = true;
	m_canCheckCloseSocket = false;

	m_canTriggerRead = false;
	m_canTriggerWrite = true;

	m_tlsState = noconn;

	m_lastReadFailed = true;
	m_lastWriteFailed = false;
	m_writeSkip = 0;

	m_peekData = 0;
	m_peekDataLen = 0;

	m_socketClosed = false;

	m_implicitTrustedCert.data = 0;
	m_implicitTrustedCert.size = 0;

	m_shutdown_requested = false;

	m_socket_eof = false;

	m_require_root_trust = false;
}

CTlsSocket::~CTlsSocket()
{
	Uninit();
	delete m_pSocketBackend;
}

bool CTlsSocket::Init()
{
	// This function initializes GnuTLS
	m_initialized = true;
	int res = gnutls_global_init();
	if (res)
	{
		LogError(res);
		Uninit();
		return false;
	}

#if TLSDEBUG
	pLoggingControlSocket = m_pOwner;
	gnutls_global_set_log_function(log_func);
	gnutls_global_set_log_level(99);
#endif
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

	res = gnutls_priority_set_direct(m_session, "NORMAL:-3DES-CBC:-MD5:-SIGN-RSA-MD5:+CTYPE-X509:-CTYPE-OPENPGP", 0);
	if (res)
	{
		LogError(res);
		Uninit();
		return false;
	}

	gnutls_dh_set_prime_bits(m_session, 2048);

	gnutls_credentials_set(m_session, GNUTLS_CRD_CERTIFICATE, m_certCredentials);

	// Setup transport functions
	gnutls_transport_set_push_function(m_session, PushFunction);
	gnutls_transport_set_pull_function(m_session, PullFunction);
	gnutls_transport_set_ptr(m_session, (gnutls_transport_ptr_t)this);
#if GNUTLS_VERSION_NUMBER < 0x020c00
	gnutls_transport_set_lowat(m_session, 0);
#endif

	m_shutdown_requested = false;

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

	m_require_root_trust = false;
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
		m_pOwner->LogMessage(::Error, _T("GnuTLS error %d: %s"), code, str.c_str());
	}
	else
		m_pOwner->LogMessage(::Error, _T("GnuTLS error %d"), code);
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
#if TLSDEBUG
	m_pOwner->LogMessage(Debug_Debug, _T("CTlsSocket::PushFunction(%x, %d)"), data, len);
#endif
	if (!m_canWriteToSocket)
	{
		gnutls_transport_set_errno(m_session, EAGAIN);
		return -1;
	}

	if (!m_pSocketBackend)
	{
		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

	int error;
	int written = m_pSocketBackend->Write(data, len, error);

	if (written < 0)
	{
		if (error == EAGAIN)
		{
			m_canWriteToSocket = false;
			gnutls_transport_set_errno(m_session, EAGAIN);
			return -1;
		}

		gnutls_transport_set_errno(m_session, 0);
		return -1;
	}

#if TLSDEBUG
	m_pOwner->LogMessage(Debug_Debug, _T("  returning %d"), written);
#endif

	return written;
}

ssize_t CTlsSocket::PullFunction(void* data, size_t len)
{
#if TLSDEBUG
	m_pOwner->LogMessage(Debug_Debug, _T("CTlsSocket::PullFunction(%x, %d)"), data, len);
#endif
	if (!m_pSocketBackend)
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

	int error;
	int read = m_pSocketBackend->Read(data, len, error);
	if (read < 0)
	{
		if (error == EAGAIN)
		{
			m_canReadFromSocket = false;
			if (m_canCheckCloseSocket && !m_pSocketBackend->IsWaiting(CRateLimiter::inbound))
			{
				CSocketEvent *evt = new CSocketEvent(this, m_pSocketBackend, CSocketEvent::close);
				CSocketEventDispatcher::Get().SendEvent(evt);
			}
		}

		gnutls_transport_set_errno(m_session, error);
		return -1;
	}

	if (m_canCheckCloseSocket)
	{
		CSocketEvent *evt = new CSocketEvent(this, m_pSocketBackend, CSocketEvent::close);
		CSocketEventDispatcher::Get().SendEvent(evt);
	}

	if (!read)
		m_socket_eof = true;

#if TLSDEBUG
	m_pOwner->LogMessage(Debug_Debug, _T("  returning %d"), read);
#endif

	return read;
}

void CTlsSocket::OnSocketEvent(CSocketEvent& event)
{
	wxASSERT(m_pSocket);
	if (!m_session)
		return;

	switch (event.GetType())
	{
	case CSocketEvent::read:
		OnRead();
		break;
	case CSocketEvent::write:
		OnSend();
		break;
	case CSocketEvent::close:
		{
			m_canCheckCloseSocket = true;
			char tmp[100];
			int error;
			int peeked = m_pSocketBackend->Peek(&tmp, 100, error);
			if (peeked >= 0)
			{
				if (peeked > 0)
					m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::OnSocketEvent(): pending data, postponing close event"));
				else
				{
					m_socket_eof = true;
					m_socketClosed = true;
				}
				OnRead();

				if (peeked)
					return;
			}

			m_pOwner->LogMessage(Debug_Info, _T("CTlsSocket::OnSocketEvent(): close event received"));

			//Uninit();
			CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::close);
			CSocketEventDispatcher::Get().SendEvent(evt);
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

	const int direction = gnutls_record_get_direction(m_session);
	if (direction && !m_lastReadFailed)
		return;

	if (m_tlsState == handshake)
		ContinueHandshake();
	if (m_tlsState == closing)
		ContinueShutdown();
	else if (m_tlsState == conn)
	{
		CheckResumeFailedReadWrite();
		TriggerEvents();
	}
}

void CTlsSocket::OnSend()
{
	m_pOwner->LogMessage(Debug_Debug, _T("CTlsSocket::OnSend()"));

	m_canWriteToSocket = true;

	if (!m_session)
		return;

	const int direction = gnutls_record_get_direction(m_session);
	if (!direction && !m_lastWriteFailed)
		return;

	if (m_tlsState == handshake)
		ContinueHandshake();
	else if (m_tlsState == closing)
		ContinueShutdown();
	else if (m_tlsState == conn)
	{
		CheckResumeFailedReadWrite();
		TriggerEvents();
	}
}

bool CTlsSocket::CopySessionData(const CTlsSocket* pPrimarySocket)
{
	size_t session_data_size = 0;

	// Get buffer size
	int res = gnutls_session_get_data(pPrimarySocket->m_session, 0, &session_data_size);
	if (res)
	{
		m_pOwner->LogMessage(Debug_Info, _T("gnutls_session_get_data on primary socket failed: %d"), res);
		return false;
	}

	// Get session data
	char *session_data = new char[session_data_size];
	res = gnutls_session_get_data(pPrimarySocket->m_session, session_data, &session_data_size);
	if (res)
	{
		delete [] session_data;
		m_pOwner->LogMessage(Debug_Info, _T("gnutls_session_get_data on primary socket failed: %d"), res);
		return false;
	}

	// Set session data
	res = gnutls_session_set_data(m_session, session_data, session_data_size);
	delete [] session_data;
	if (res)
	{
		m_pOwner->LogMessage(Debug_Info, _T("gnutls_session_set_data failed: %d"), res);
		return false;
	}

	m_pOwner->LogMessage(Debug_Info, _T("Trying to resume existing TLS session."));

	return true;
}

bool CTlsSocket::ResumedSession() const
{
	return gnutls_session_is_resumed(m_session) != 0;
}

int CTlsSocket::Handshake(const CTlsSocket* pPrimarySocket /*=0*/, bool try_resume /*=false*/)
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

		if (try_resume)
			CopySessionData(pPrimarySocket);
	}

	return ContinueHandshake();
}

int CTlsSocket::ContinueHandshake()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::ContinueHandshake()"));
	wxASSERT(m_session);
	wxASSERT(m_tlsState == handshake);

	int res = gnutls_handshake(m_session);
	if (!res)
	{
		m_pOwner->LogMessage(Debug_Info, _T("TLS Handshake successful"));

		if (ResumedSession())
			m_pOwner->LogMessage(Debug_Info, _T("TLS Session resumed"));

		const wxString& cipherName = GetCipherName();
		const wxString& macName = GetMacName();

		m_pOwner->LogMessage(Debug_Info, _T("Cipher: %s, MAC: %s"), cipherName.c_str(), macName.c_str());

		res = VerifyCertificate();
		if (res != FZ_REPLY_OK)
			return res;

		if (m_shutdown_requested)
		{
			int error = Shutdown();
			if (!error || error != EAGAIN)
			{
				CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::close);
				CSocketEventDispatcher::Get().SendEvent(evt);
			}
		}

		return FZ_REPLY_OK;
	}
	else if (res == GNUTLS_E_AGAIN || res == GNUTLS_E_INTERRUPTED)
		return FZ_REPLY_WOULDBLOCK;

	Failure(res, ECONNABORTED);

	return FZ_REPLY_ERROR;
}

int CTlsSocket::Read(void *buffer, unsigned int len, int& error)
{
	if (m_tlsState == handshake || m_tlsState == verifycert)
	{
		error = EAGAIN;
		return -1;
	}
	else if (m_tlsState != conn)
	{
		error = ENOTCONN;
		return -1;
	}
	else if (m_lastReadFailed)
	{
		error = EAGAIN;
		return -1;
	}

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

		error = 0;
		return min;
	}

	int res = gnutls_record_recv(m_session, buffer, len);
	if (res >= 0)
	{
		if (res > 0)
			TriggerEvents();
		else
		{
			// Peer did already initiate a shutdown, reply to it
			gnutls_bye(m_session, GNUTLS_SHUT_WR);
			// Note: Theoretically this could return a write error.
			// But we ignore it, since it is perfectly valid for peer
			// to close the connection after sending its shutdown
			// notification.
		}

		error = 0;
		return res;
	}

	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
	{
		error = EAGAIN;
		m_lastReadFailed = true;
	}
	else
	{
		Failure(res, 0);
		error = ECONNABORTED;
	}

	return -1;
}

int CTlsSocket::Write(const void *buffer, unsigned int len, int& error)
{
	if (m_tlsState == handshake || m_tlsState == verifycert)
	{
		error = EAGAIN;
		return -1;
	}
	else if (m_tlsState != conn)
	{
		error = ENOTCONN;
		return -1;
	}

	if (m_lastWriteFailed)
	{
		error = EAGAIN;
		return -1;
	}

	if (m_writeSkip >= len)
	{
		m_writeSkip -= len;
		return len;
	}

	len -= m_writeSkip;
	buffer = (char*)buffer + m_writeSkip;

	int res = gnutls_record_send(m_session, buffer, len);
	if (res >= 0)
	{
		error = 0;
		int written = res + m_writeSkip;
		m_writeSkip = 0;

		TriggerEvents();
		return written;
	}

	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
	{
		if (m_writeSkip)
		{
			error = 0;
			int written = m_writeSkip;
			m_writeSkip = 0;
			return written;
		}
		else
		{
			error = EAGAIN;
			m_lastWriteFailed = true;
			return -1;
		}
	}
	else
	{
		Failure(res, 0);
		error = ECONNABORTED;
		return -1;
	}
}

void CTlsSocket::TriggerEvents()
{
	if (m_tlsState != conn)
		return;

	if (m_canTriggerRead)
	{
		CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::read);
		CSocketEventDispatcher::Get().SendEvent(evt);
		m_canTriggerRead = false;
	}

	if (m_canTriggerWrite)
	{
		CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::write);
		CSocketEventDispatcher::Get().SendEvent(evt);
		m_canTriggerWrite = false;
	}
}

void CTlsSocket::CheckResumeFailedReadWrite()
{
	if (m_lastWriteFailed)
	{
		int res = gnutls_record_send(m_session, 0, 0);
		if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
			return;

		if (res < 0)
		{
			Failure(res, ECONNABORTED);
			return;
		}

		m_writeSkip += res;
		m_lastWriteFailed = false;
		m_canTriggerWrite = true;

		wxASSERT(GNUTLS_E_INVALID_REQUEST == gnutls_record_send(m_session, 0, 0));
	}
	if (m_lastReadFailed)
	{
		wxASSERT(!m_peekData);

		m_peekDataLen = 65536;
		m_peekData = new char[m_peekDataLen];

		int res = gnutls_record_recv(m_session, m_peekData, m_peekDataLen);
		if (res < 0)
		{
			m_peekDataLen = 0;
			delete [] m_peekData;
			m_peekData = 0;
			if (res != GNUTLS_E_INTERRUPTED && res != GNUTLS_E_AGAIN)
				Failure(res, ECONNABORTED);
			return;
		}

		if (!res)
		{
			m_peekDataLen = 0;
			delete [] m_peekData;
			m_peekData = 0;
		}
		else
			m_peekDataLen = res;

		m_lastReadFailed = false;
		m_canTriggerRead = true;
	}
}

void CTlsSocket::Failure(int code, int socket_error)
{
	m_pOwner->LogMessage(::Debug_Debug, _T("CTlsSocket::Failure(%d, %d)"), code, socket_error);
	if (code)
	{
		LogError(code);
		if (code == GNUTLS_E_UNEXPECTED_PACKET_LENGTH && m_socket_eof)
			m_pOwner->LogMessage(Status, _("Server did not properly shut down TLS connection"));
	}
	Uninit();

	if (socket_error)
	{
		CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::close, socket_error);
		CSocketEventDispatcher::Get().SendEvent(evt);
	}
}

int CTlsSocket::Peek(void *buffer, unsigned int len, int& error)
{
	if (m_peekData)
	{
		int min = wxMin(len, m_peekDataLen);
		memcpy(buffer, m_peekData, min);

		error = 0;
		return min;
	}

	int read = Read(buffer, len, error);
	if (read <= 0)
		return read;

	m_peekDataLen = read;
	m_peekData = new char[m_peekDataLen];
	memcpy(m_peekData, buffer, m_peekDataLen);

	return read;
}

int CTlsSocket::Shutdown()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::Shutdown()"));

	if (m_tlsState == closed)
		return 0;

	if (m_tlsState == closing)
		return EAGAIN;

	if (m_tlsState == handshake || m_tlsState == verifycert)
	{
		// Shutdown during handshake is not a good idea.
		m_pOwner->LogMessage(Debug_Verbose, _T("Shutdown during handshake, postponing"));
		m_shutdown_requested = true;
		return EAGAIN;
	}

	if (m_tlsState != conn)
		return ECONNABORTED;

	m_tlsState = closing;

	int res = gnutls_bye(m_session, GNUTLS_SHUT_WR);
	if (!res)
	{
		m_tlsState = closed;
		return 0;
	}

	if (res == GNUTLS_E_INTERRUPTED || res == GNUTLS_E_AGAIN)
		return EAGAIN;

	Failure(res, 0);
	return ECONNABORTED;
}

void CTlsSocket::ContinueShutdown()
{
	m_pOwner->LogMessage(Debug_Verbose, _T("CTlsSocket::ContinueShutdown()"));

	int res = gnutls_bye(m_session, GNUTLS_SHUT_WR);
	if (!res)
	{
		m_tlsState = closed;

		CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::close);
		CSocketEventDispatcher::Get().SendEvent(evt);

		return;
	}

	if (res != GNUTLS_E_INTERRUPTED && res != GNUTLS_E_AGAIN)
		Failure(res, ECONNABORTED);
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

		if (m_lastWriteFailed)
			m_lastWriteFailed = false;
		CheckResumeFailedReadWrite();

		if (m_tlsState == conn)
		{
			CSocketEvent *evt = new CSocketEvent(m_pEvtHandler, this, CSocketEvent::connection);
			CSocketEventDispatcher::Get().SendEvent(evt);
		}

		TriggerEvents();

		return;
	}

	m_pOwner->LogMessage(::Error, _("Remote certificate not trusted."));
	Failure(0, ECONNABORTED);
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
		m_pOwner->LogMessage(::Debug_Warning, _T("VerifyCertificate called at wrong time"));
		return FZ_REPLY_ERROR;
	}

	m_tlsState = verifycert;

	if (gnutls_certificate_type_get(m_session) != GNUTLS_CRT_X509)
	{
		m_pOwner->LogMessage(::Error, _("Unsupported certificate type"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	unsigned int status = 0;
	if (gnutls_certificate_verify_peers2(m_session, &status) < 0)
	{
		m_pOwner->LogMessage(::Error, _("Failed to verify peer certificate"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	if (status & GNUTLS_CERT_REVOKED)
	{
		m_pOwner->LogMessage(::Error, _("Beware! Certificate has been revoked"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	if (status & GNUTLS_CERT_SIGNER_NOT_CA)
	{
		m_pOwner->LogMessage(::Error, _("Incomplete chain, top certificate is not self-signed certificate authority certificate"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	if (m_require_root_trust && status & GNUTLS_CERT_SIGNER_NOT_FOUND)
	{
		m_pOwner->LogMessage(::Error, _("Root certificate is not trusted"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	unsigned int cert_list_size;
	const gnutls_datum_t* cert_list = gnutls_certificate_get_peers(m_session, &cert_list_size);
	if (!cert_list || !cert_list_size)
	{
		m_pOwner->LogMessage(::Error, _("gnutls_certificate_get_peers returned no certificates"));
		Failure(0, ECONNABORTED);
		return FZ_REPLY_ERROR;
	}

	if (m_implicitTrustedCert.data)
	{
		if (m_implicitTrustedCert.size != cert_list[0].size ||
			memcmp(m_implicitTrustedCert.data, cert_list[0].data, cert_list[0].size))
		{
			m_pOwner->LogMessage(::Error, _("Primary connection and data connection certificates don't match."));
			Failure(0, ECONNABORTED);
			return FZ_REPLY_ERROR;
		}

		TrustCurrentCert(true);

		if (m_tlsState != conn)
			return FZ_REPLY_ERROR;
		return FZ_REPLY_OK;
	}

	std::vector<CCertificate> certificates;
	for (unsigned int i = 0; i < cert_list_size; i++)
	{
		gnutls_x509_crt_t cert;
		if (gnutls_x509_crt_init(&cert))
		{
			m_pOwner->LogMessage(::Error, _("Could not initialize structure for peer certificates, gnutls_x509_crt_init failed"));
			Failure(0, ECONNABORTED);
			return FZ_REPLY_ERROR;
		}

		if (gnutls_x509_crt_import(cert, cert_list, GNUTLS_X509_FMT_DER))
		{
			m_pOwner->LogMessage(::Error, _("Could not import peer certificates, gnutls_x509_crt_import failed"));
			Failure(0, ECONNABORTED);
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
			m_pOwner->LogMessage(::Error, _("Could not get distinguished name of certificate subject, gnutls_x509_get_dn failed"));
			Failure(0, ECONNABORTED);
			gnutls_x509_crt_deinit(cert);
			return FZ_REPLY_ERROR;
		}

		size = 0;
		res = gnutls_x509_crt_get_issuer_dn(cert, 0, &size);
		if (size)
		{
			char* dn = new char[++size + 1];
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
			m_pOwner->LogMessage(::Error, _("Could not get distinguished name of certificate issuer, gnutls_x509_get_issuer_dn failed"));
			Failure(0, ECONNABORTED);
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

		certificates.push_back(CCertificate(
			cert_list->data, cert_list->size,
			activationTime, expirationTime,
			serial,
			algoName, bits,
			fingerprint_md5,
			fingerprint_sha1,
			subject,
			issuer));

		cert_list++;
	}

	CCertificateNotification *pNotification = new CCertificateNotification(
		m_pOwner->GetCurrentServer()->GetHost(),
		m_pOwner->GetCurrentServer()->GetPort(),
		GetCipherName(),
		GetMacName(),
		certificates);

	m_pOwner->SendAsyncRequest(pNotification);

	m_pOwner->LogMessage(Status, _("Verifying certificate..."));

	return FZ_REPLY_WOULDBLOCK;
}

void CTlsSocket::OnRateAvailable(enum CRateLimiter::rate_direction direction)
{
}

wxString CTlsSocket::GetCipherName()
{
	const char* cipher = gnutls_cipher_get_name(gnutls_cipher_get(m_session));
	if (cipher && *cipher)
		return wxString(cipher, wxConvUTF8);
	else
		return _T("unknown");
}

wxString CTlsSocket::GetMacName()
{
	const char* mac = gnutls_mac_get_name(gnutls_mac_get(m_session));
	if (mac && *mac)
		return wxString(mac, wxConvUTF8);
	else
		return _T("unknown");
}

bool CTlsSocket::AddTrustedRootCertificate(const wxString& cert)
{
	if (!m_initialized)
		return false;

	if (cert == _T(""))
		return false;

	m_require_root_trust = true;

	const wxWX2MBbuf str = cert.mb_str();

	gnutls_datum_t datum;
	datum.data = (unsigned char*)(const char*)str;
	datum.size = strlen(str);

	return gnutls_certificate_set_x509_trust_mem(m_certCredentials, &datum, GNUTLS_X509_FMT_PEM) > 0;
}
