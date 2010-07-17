#include <filezilla.h>
#include "transfersocket.h"
#include "ftpcontrolsocket.h"
#include "directorylistingparser.h"
#include "optionsbase.h"
#include "iothread.h"
#include "tlssocket.h"
#include <errno.h>
#include "proxy.h"
#include "servercapabilities.h"

BEGIN_EVENT_TABLE(CTransferSocket, wxEvtHandler)
	EVT_IOTHREAD(wxID_ANY, CTransferSocket::OnIOThreadEvent)
END_EVENT_TABLE();

CTransferSocket::CTransferSocket(CFileZillaEnginePrivate *pEngine, CFtpControlSocket *pControlSocket, enum TransferMode transferMode)
{
	m_pEngine = pEngine;
	m_pControlSocket = pControlSocket;

	m_pSocketServer = 0;
	m_pSocket = 0;
	m_pBackend = 0;
	m_pProxyBackend = 0;
	m_pTlsSocket = 0;

	m_pDirectoryListingParser = 0;

	m_bActive = false;

	SetEvtHandlerEnabled(true);

	m_transferMode = transferMode;

	m_pTransferBuffer = 0;
	m_transferBufferLen = 0;

	m_transferEndReason = none;
	m_binaryMode = true;

	m_onCloseCalled = false;

	m_postponedReceive = false;
	m_postponedSend = false;

	m_shutdown = false;

	m_madeProgress = 0;
}

CTransferSocket::~CTransferSocket()
{
	if (m_transferEndReason == none)
		m_transferEndReason = successful;
	delete m_pProxyBackend;
	if (m_pTlsSocket)
		delete m_pTlsSocket;
	else if (m_pBackend)
		delete m_pBackend;
	delete m_pSocketServer;
	m_pSocketServer = 0;
	delete m_pSocket;
	m_pSocket = 0;

	if (m_pControlSocket)
	{
		if (m_transferMode == upload || m_transferMode == download)
		{
			CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);;
			if (pData && pData->pIOThread)
			{
				if (m_transferMode == download)
					FinalizeWrite();
				pData->pIOThread->SetEventHandler(0);
			}
		}
	}
}

wxString CTransferSocket::SetupActiveTransfer(const wxString& ip)
{
	// Void all previous attempts to createt a socket
	delete m_pSocket;
	m_pSocket = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pSocketServer = CreateSocketServer();
	if (!m_pSocketServer)
	{
		m_pControlSocket->LogMessage(::Debug_Warning, _T("CreateSocketServer failed"));
		return _T("");
	}

	int error;
	int port = m_pSocketServer->GetLocalPort(error);
	if (port == -1)
	{
		delete m_pSocketServer;
		m_pSocketServer = 0;

		m_pControlSocket->LogMessage(::Debug_Warning, _T("GetLocalPort failed: %s"), CSocket::GetErrorDescription(error).c_str());
		return _T("");
	}

	wxString portArguments;
	if (m_pSocketServer->GetAddressFamily() == CSocket::ipv6)
	{
		portArguments = wxString::Format(_T("|2|%s|%d|"), ip.c_str(), port);
	}
	else
	{
		portArguments = ip;
		portArguments += wxString::Format(_T(",%d,%d"), port / 256, port % 256);
		portArguments.Replace(_T("."), _T(","));
	}

	return portArguments;
}

void CTransferSocket::OnSocketEvent(CSocketEvent &event)
{
	if (m_pProxyBackend)
	{
		switch (event.GetType())
		{
		case CSocketEvent::connection:
			{
				const int error = event.GetError();
				if (error)
				{
					m_pControlSocket->LogMessage(::Error, _("Proxy handshake failed: %s"), CSocket::GetErrorDescription(error).c_str());
					TransferEnd(failure);
				}
				else
				{
					delete m_pProxyBackend;
					m_pProxyBackend = 0;
					OnConnect();
				}
			}
			return;
		case CSocketEvent::close:
			{
				const int error = event.GetError();
				m_pControlSocket->LogMessage(::Error, _("Proxy handshake failed: %s"), CSocket::GetErrorDescription(error).c_str());
				TransferEnd(failure);
			}
			return;
		default:
			// Uninteresting
			return;
		}
		return;
	}

	if (m_pSocketServer)
	{
		if (event.GetType() == CSocketEvent::connection)
			OnAccept(event.GetError());
		else
			m_pControlSocket->LogMessage(::Debug_Info, _T("Unhandled socket event %d from listening socket"), event.GetType());
		return;
	}

	switch (event.GetType())
	{
	case CSocketEvent::connection:
		OnConnect();
		break;
	case CSocketEvent::read:
		OnReceive();
		break;
	case CSocketEvent::write:
		OnSend();
		break;
	case CSocketEvent::close:
		OnClose(event.GetError());
		break;
	case CSocketEvent::hostaddress:
		// Booooring. No seriously, we connect by IP already, nothing to resolve
		break;
	default:
		m_pControlSocket->LogMessage(::Debug_Info, _T("Unhandled socket event %d"), event.GetType());
		break;
	}
}

void CTransferSocket::OnAccept(int error)
{
	m_pControlSocket->SetAlive();
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("CTransferSocket::OnAccept(%d)"), error);

	if (!m_pSocketServer)
	{
		m_pControlSocket->LogMessage(::Debug_Warning, _T("No socket server in OnAccept"), error);
		return;
	}

	m_pSocket = m_pSocketServer->Accept(error);
	if (!m_pSocket)
	{
		if (error == EAGAIN)
			m_pControlSocket->LogMessage(::Debug_Verbose, _T("No pending connection"));
		else
		{
			m_pControlSocket->LogMessage(::Status, _("Could not accept connection: %s"), CSocket::GetErrorDescription(error).c_str());
			TransferEnd(transfer_failure);
		}
		return;
	}
	delete m_pSocketServer;
	m_pSocketServer = 0;

	OnConnect();
}

void CTransferSocket::OnConnect()
{
	m_pControlSocket->SetAlive();
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("CTransferSocket::OnConnect"));

	if (!m_pSocket)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("CTransferSocket::OnConnect called without socket"));
		return;
	}

	if (!m_pBackend)
	{
		if (!InitBackend())
		{
			TransferEnd(transfer_failure);
			return;
		}
	}
	else if (m_pTlsSocket)
	{
		if (CServerCapabilities::GetCapability(*m_pControlSocket->m_pCurrentServer, tls_resume) == unknown)
		{
			CServerCapabilities::SetCapability(*m_pControlSocket->m_pCurrentServer, tls_resume, m_pTlsSocket->ResumedSession() ? yes : no);
		}
	}

	if (m_bActive)
		TriggerPostponedEvents();
}

void CTransferSocket::OnReceive()
{
	m_pControlSocket->LogMessage(::Debug_Debug, _T("CTransferSocket::OnReceive(), m_transferMode=%d"), m_transferMode);

	if (!m_pBackend)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Postponing receive, m_pBackend was false."));
		m_postponedReceive = true;
		return;
	}

	if (!m_bActive)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Postponing receive, m_bActive was false."));
		m_postponedReceive = true;
		return;
	}

	if (m_transferMode == list)
	{
		while (true)
		{
			char *pBuffer = new char[4096];
			int error;
			int numread = m_pBackend->Read(pBuffer, 4096, error);
			if (numread < 0)
			{
				delete [] pBuffer;
				if (error != EAGAIN)
				{
					m_pControlSocket->LogMessage(::Error, _T("Could not read from transfer socket: %s"), CSocket::GetErrorDescription(error).c_str());
					TransferEnd(transfer_failure);
				}
				else if (m_onCloseCalled && !m_pBackend->IsWaiting(CRateLimiter::inbound))
					TransferEnd(successful);

				return;
			}

			if (numread > 0)
			{
				if (!m_pDirectoryListingParser->AddData(pBuffer, numread))
				{
					TransferEnd(transfer_failure);
					return;
				}

				m_pEngine->SetActive(CFileZillaEngine::recv);
				if (!m_madeProgress)
				{
					m_madeProgress = 2;
					m_pControlSocket->SetTransferStatusMadeProgress();
				}
				m_pControlSocket->UpdateTransferStatus(numread);
			}
			else
			{
				delete [] pBuffer;
				TransferEnd(successful);
				return;
			}
		}
	}
	else if (m_transferMode == download)
	{
		while (true)
		{
			if (!CheckGetNextWriteBuffer())
				return;

			int error;
			int numread = m_pBackend->Read(m_pTransferBuffer, m_transferBufferLen, error);
			if (numread < 0)
			{
				if (error != EAGAIN)
				{
					m_pControlSocket->LogMessage(::Error, _T("Could not read from transfer socket: %s"), CSocket::GetErrorDescription(error).c_str());
					TransferEnd(transfer_failure);
				}
				else if (m_onCloseCalled && !m_pBackend->IsWaiting(CRateLimiter::inbound))
					TransferEnd(successful);
				return;
			}

			if (numread > 0)
			{
				m_pEngine->SetActive(CFileZillaEngine::recv);
				if (!m_madeProgress)
				{
					m_madeProgress = 2;
					m_pControlSocket->SetTransferStatusMadeProgress();
				}
				m_pControlSocket->UpdateTransferStatus(numread);

				m_pTransferBuffer += numread;
				m_transferBufferLen -= numread;

				if (!CheckGetNextWriteBuffer())
					return;
			}
			else //!numread
			{
				FinalizeWrite();
				break;
			}
		}
	}
	else if (m_transferMode == resumetest)
	{
		while (true)
		{
			char buffer[2];
			int error;
			int numread = m_pBackend->Read(buffer, 2, error);
			if (numread < 0)
			{
				if (error != EAGAIN)
				{
					m_pControlSocket->LogMessage(::Error, _T("Could not read from transfer socket: %s"), CSocket::GetErrorDescription(error).c_str());
					TransferEnd(transfer_failure);
				}
				else if (m_onCloseCalled && !m_pBackend->IsWaiting(CRateLimiter::inbound))
				{
					if (m_transferBufferLen == 1)
						TransferEnd(successful);
					else
					{
						m_pControlSocket->LogMessage(::Debug_Warning, _T("Server incorrectly sent %d bytes"), m_transferBufferLen);
						TransferEnd(failed_resumetest);
					}
				}
				return;
			}

			if (!numread)
			{
				if (m_transferBufferLen == 1)
					TransferEnd(successful);
				else
				{
					m_pControlSocket->LogMessage(::Debug_Warning, _T("Server incorrectly sent %d bytes"), m_transferBufferLen);
					TransferEnd(failed_resumetest);
				}
				return;
			}
			m_transferBufferLen += numread;

			if (m_transferBufferLen > 1)
			{
				m_pControlSocket->LogMessage(::Debug_Warning, _T("Server incorrectly sent %d bytes"), m_transferBufferLen);
				TransferEnd(failed_resumetest);
				return;
			}
		}
	}
}

void CTransferSocket::OnSend()
{
	if (!m_pBackend)
		return;

	if (!m_bActive)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Postponing send"));
		m_postponedSend = true;
		return;
	}

	if (m_transferMode != upload)
		return;

	int error;
	int written;
	do
	{
		if (!CheckGetNextReadBuffer())
			return;

		written = m_pBackend->Write(m_pTransferBuffer, m_transferBufferLen, error);
		if (written <= 0)
			break;

		m_pEngine->SetActive(CFileZillaEngine::send);
		if (m_madeProgress == 1)
		{
			m_pControlSocket->LogMessage(::Debug_Debug, _T("Made progress in CTransferSocket::OnSend()"));
			m_madeProgress = 2;
			m_pControlSocket->SetTransferStatusMadeProgress();
		}
		m_pControlSocket->UpdateTransferStatus(written);

		m_pTransferBuffer += written;
		m_transferBufferLen -= written;
	}
	while (true);

	if (written < 0)
	{
		if (error == EAGAIN)
		{
			if (!m_madeProgress)
			{
				m_pControlSocket->LogMessage(::Debug_Debug, _T("First EAGAIN in CTransferSocket::OnSend()"));
				m_madeProgress = 1;
				m_pControlSocket->SetTransferStatusMadeProgress();
			}
		}
		else
		{
			m_pControlSocket->LogMessage(Error, _T("Could not write to transfer socket: %s"), CSocket::GetErrorDescription(error).c_str());
			TransferEnd(transfer_failure);
		}
	}
}

void CTransferSocket::OnClose(int error)
{
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("CTransferSocket::OnClose(%d)"), error);
	m_onCloseCalled = true;

	if (m_transferEndReason != none)
		return;

	if (!m_pBackend)
	{
		if (!InitBackend())
		{
			TransferEnd(transfer_failure);
			return;
		}
	}

	if (m_transferMode == upload)
	{
		if (m_shutdown && m_pTlsSocket)
		{
			if (m_pTlsSocket->Shutdown() != 0)
				TransferEnd(transfer_failure);
			else
				TransferEnd(successful);
		}
		else
			TransferEnd(transfer_failure);
		return;
	}

	if (error)
	{
		m_pControlSocket->LogMessage(::Error, _("Transfer connection interrupted: %s"), CSocket::GetErrorDescription(error).c_str());
		TransferEnd(transfer_failure);
		return;
	}

	char buffer[100];
	int numread = m_pBackend->Peek(&buffer, 100, error);
	if (numread > 0)
	{
#ifndef __WXMSW__
		wxFAIL_MSG(_T("Peek isn't supposed to return data after close notification"));
#endif

		// MSDN says this:
		//   FD_CLOSE being posted after all data is read from a socket.
		//   An application should check for remaining data upon receipt
		//   of FD_CLOSE to avoid any possibility of losing data.
		// First half is actually plain wrong.
		OnReceive();

		return;
	}
	else if (numread < 0 && error != EAGAIN)
	{
		m_pControlSocket->LogMessage(::Error, _("Transfer connection interrupted: %s"), CSocket::GetErrorDescription(error).c_str());
		TransferEnd(transfer_failure);
		return;
	}

	if (m_transferMode == resumetest)
	{
		if (m_transferBufferLen != 1)
		{
			TransferEnd(failed_resumetest);
			return;
		}
	}
	TransferEnd(successful);
}

bool CTransferSocket::SetupPassiveTransfer(wxString host, int port)
{
	// Void all previous attempts to createt a socket
	delete m_pSocket;
	m_pSocket = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pSocket = new CSocket(this);

	if (m_pControlSocket->m_pProxyBackend)
	{
		m_pProxyBackend = new CProxySocket(this, m_pSocket, m_pControlSocket);

		int res = m_pProxyBackend->Handshake(m_pControlSocket->m_pProxyBackend->GetProxyType(),
											 host, port,
											 m_pControlSocket->m_pProxyBackend->GetUser(), m_pControlSocket->m_pProxyBackend->GetPass());

		if (res != EINPROGRESS)
		{
			delete m_pProxyBackend;
			m_pProxyBackend = 0;
			delete m_pSocket;
			m_pSocket = 0;
			return false;
		}
		int error;
		host = m_pControlSocket->m_pSocket->GetPeerIP();
		port = m_pControlSocket->m_pSocket->GetRemotePort(error);
	}

	SetSocketBufferSizes(m_pSocket);

	int res = m_pSocket->Connect(host, port);
	if (res && res != EINPROGRESS)
	{
		delete m_pProxyBackend;
		m_pProxyBackend = 0;
		delete m_pSocket;
		m_pSocket = 0;
		return false;
	}

	return true;
}

void CTransferSocket::SetActive()
{
	if (m_transferEndReason != none)
		return;
	if (m_transferMode == download || m_transferMode == upload)
	{
		CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);;
		if (pData && pData->pIOThread)
			pData->pIOThread->SetEventHandler(this);
	}

	m_bActive = true;
	if (!m_pSocket)
		return;
	
	if (m_pSocket->GetState() == CSocket::connected || m_pSocket->GetState() == CSocket::closing)
		TriggerPostponedEvents();
}

void CTransferSocket::TransferEnd(enum TransferEndReason reason)
{
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("CTransferSocket::TransferEnd(%d)"), reason);

	if (m_transferEndReason != none)
		return;
	m_transferEndReason = reason;

	delete m_pSocketServer;
	m_pSocketServer = 0;

	if (m_pProxyBackend)
	{
		delete m_pProxyBackend;
		m_pProxyBackend = 0;
	}
	if (m_pTlsSocket)
	{
		if (m_pBackend == m_pTlsSocket)
			m_pBackend = 0;
        delete m_pTlsSocket;
		m_pTlsSocket = 0;
	}
	if (m_pBackend)
	{
		delete m_pBackend;
		m_pBackend = 0;
	}
	delete m_pSocket;
	m_pSocket = 0;

	m_pEngine->SendEvent(engineTransferEnd);
}

CSocket* CTransferSocket::CreateSocketServer(int port)
{
	CSocket* pServer = new CSocket(this);
	int res = pServer->Listen(m_pControlSocket->m_pSocket->GetAddressFamily(), port);
	if (res)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Could not listen on port %d: %s"), port, CSocket::GetErrorDescription(res).c_str());
		delete pServer;
		return 0;
	}

	SetSocketBufferSizes(pServer);

	return pServer;
}

CSocket* CTransferSocket::CreateSocketServer()
{
	if (!m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS))
	{
		// Ask the systen for a port
		CSocket* pServer = CreateSocketServer(0);
		return pServer;
	}

	// Try out all ports in the port range.
	// Upon first call, we try to use a random port fist, after that
	// increase the port step by step

	// Windows only: I think there's a bug in the socket implementation of
	// Windows: Even if using SO_REUSEADDR, using the same local address
	// twice will fail unless there are a couple of minutes between the
	// connection attempts. This may cause problems if transferring lots of
	// files with a narrow port range.

	static int start = 0;

	int low = m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS_LOW);
	int high = m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS_HIGH);
	if (low > high)
		low = high;

	if (start < low || start > high)
	{
		start = GetRandomNumber(low, high);
		wxASSERT(start >= low && start <= high);
	}

	CSocket* pServer = 0;

	int count = high - low + 1;
	while (count--)
	{
		pServer = CreateSocketServer(start++);
		if (pServer)
			break;
		if (start > high)
			start = low;
	}

	return pServer;
}

bool CTransferSocket::CheckGetNextWriteBuffer()
{
	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);;
	if (!m_transferBufferLen)
	{
		int res = pData->pIOThread->GetNextWriteBuffer(&m_pTransferBuffer);

		if (res == IO_Again)
			return false;
		else if (res == IO_Error)
		{
			wxString error = pData->pIOThread->GetError();
			if (error != _T(""))
				m_pControlSocket->LogMessage(::Error, _("Can't write data to file: %s"), error.c_str());
			else
				m_pControlSocket->LogMessage(::Error, _("Can't write data to file."));
			TransferEnd(transfer_failure_critical);
			return false;
		}

		m_transferBufferLen = BUFFERSIZE;
	}

	return true;
}

bool CTransferSocket::CheckGetNextReadBuffer()
{
	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);;
	if (!m_transferBufferLen)
	{
		int res = pData->pIOThread->GetNextReadBuffer(&m_pTransferBuffer);
		if (res == IO_Again)
			return false;
		else if (res == IO_Error)
		{
			m_pControlSocket->LogMessage(::Error, _("Can't read from file"));
			TransferEnd(transfer_failure);
			return false;
		}
		else if (res == IO_Success)
		{
			if (m_pTlsSocket)
			{
				m_shutdown = true;
				
				int error = m_pTlsSocket->Shutdown();
				if (error != 0)
				{
					if (error != EAGAIN)
						TransferEnd(transfer_failure);
					return false;
				}
			}
			TransferEnd(successful);
			return false;
		}
		m_transferBufferLen = res;
	}

	return true;
}

void CTransferSocket::OnIOThreadEvent(CIOThreadEvent& event)
{
	if (!m_bActive || m_transferEndReason != none)
		return;

	if (m_transferMode == download)
		OnReceive();
	else if (m_transferMode == upload)
		OnSend();
}

void CTransferSocket::FinalizeWrite()
{
	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);
	bool res = pData->pIOThread->Finalize(BUFFERSIZE - m_transferBufferLen);
	if (m_transferEndReason != none)
		return;

	if (res)
		TransferEnd(successful);
	else
	{
		wxString error = pData->pIOThread->GetError();
		if (error != _T(""))
			m_pControlSocket->LogMessage(::Error, _("Can't write data to file: %s"), error.c_str());
		else
			m_pControlSocket->LogMessage(::Error, _("Can't write data to file."));
		TransferEnd(transfer_failure_critical);
	}
}

bool CTransferSocket::InitTls(const CTlsSocket* pPrimaryTlsSocket)
{
	wxASSERT(!m_pBackend);
	m_pTlsSocket = new CTlsSocket(this, m_pSocket, m_pControlSocket);

	if (!m_pTlsSocket->Init())
	{
		delete m_pTlsSocket;
		m_pTlsSocket = 0;
		return false;
	}

	bool try_resume = CServerCapabilities::GetCapability(*m_pControlSocket->m_pCurrentServer, tls_resume) != no;

	int res = m_pTlsSocket->Handshake(pPrimaryTlsSocket, try_resume);
	if (res && res != FZ_REPLY_WOULDBLOCK)
	{
		delete m_pTlsSocket;
		m_pTlsSocket = 0;
		return false;
	}

	m_pBackend = m_pTlsSocket;

	return true;
}

void CTransferSocket::TriggerPostponedEvents()
{
	wxASSERT(m_bActive);

	if (m_postponedReceive)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Executing postponed receive"));
		m_postponedReceive = false;
		OnReceive();
		if (m_transferEndReason != none)
			return;
	}
	if (m_postponedSend)
	{
		m_pControlSocket->LogMessage(::Debug_Verbose, _T("Executing postponed send"));
		m_postponedSend = false;
		OnSend();
		if (m_transferEndReason != none)
			return;
	}
	if (m_onCloseCalled)
		OnClose(0);
}

bool CTransferSocket::InitBackend()
{
	if (m_pBackend)
		return true;

	if (m_pControlSocket->m_protectDataChannel)
	{
		if (!InitTls(m_pControlSocket->m_pTlsSocket))
			return false;
	}
	else
		m_pBackend = new CSocketBackend(this, m_pSocket);

	return true;
}

void CTransferSocket::SetSocketBufferSizes(CSocket* pSocket)
{
	wxCHECK_RET(pSocket, _("SetSocketBufferSize called without socket"));

	const int size_read = m_pEngine->GetOptions()->GetOptionVal(OPTION_SOCKET_BUFFERSIZE_RECV);
	const int size_write = m_pEngine->GetOptions()->GetOptionVal(OPTION_SOCKET_BUFFERSIZE_SEND);
	pSocket->SetBufferSizes(size_read, size_write);
}
