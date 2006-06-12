#include "FileZilla.h"
#include "transfersocket.h"
#include "ftpcontrolsocket.h"
#include "directorylistingparser.h"
#include "optionsbase.h"
#include "iothread.h"

BEGIN_EVENT_TABLE(CTransferSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CTransferSocket::OnSocketEvent)
	EVT_IOTHREAD(wxID_ANY, CTransferSocket::OnIOThreadEvent)
END_EVENT_TABLE();

CTransferSocket::CTransferSocket(CFileZillaEnginePrivate *pEngine, CFtpControlSocket *pControlSocket, enum TransferMode transferMode)
{
	m_pEngine = pEngine;
	m_pControlSocket = pControlSocket;

	m_pSocketClient = 0;
	m_pSocketServer = 0;
	m_pSocket = 0;
	
	m_pDirectoryListingParser = 0;

	m_bActive = false;

	SetEvtHandlerEnabled(true);

	m_transferMode = transferMode;

	m_pTransferBuffer = 0;
	m_transferBufferLen = 0;
		
	m_transferEnd = false;
	m_binaryMode = true;

	m_onCloseCalled = false;
}

CTransferSocket::~CTransferSocket()
{
	m_transferEnd = true;

	delete m_pSocketClient;
	delete m_pSocketServer;
	if (!m_pSocketClient && !m_pSocketServer)
		delete m_pSocket;
	m_pSocketClient = 0;
	m_pSocketServer = 0;
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
	if (!m_pSocketClient && !m_pSocketServer)
		delete m_pSocket;
	m_pSocket = 0;
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;
	
	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(0);

	m_pSocketServer = CreateSocketServer();
	if (!m_pSocketServer)
		return _T("");

	if (!m_pSocketServer->GetLocal(addr))
	{
		delete m_pSocketServer;
		m_pSocketServer = 0;
		return _T("");
	}

	wxString portArguments = ip;
	portArguments += wxString::Format(_T(",%d,%d"), addr.Service() / 256, addr.Service() % 256);
	portArguments.Replace(_T("."), _T(","));

	m_pSocketServer->SetEventHandler(*this);
	m_pSocketServer->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	m_pSocketServer->Notify(true);

	return portArguments;
}

void CTransferSocket::OnSocketEvent(wxSocketEvent &event)
{
	switch (event.GetSocketEvent())
	{
	case wxSOCKET_CONNECTION:
		OnConnect(event);
		break;
	case wxSOCKET_INPUT:
		OnReceive();
		break;
	case wxSOCKET_OUTPUT:
		OnSend();
		break;
	case wxSOCKET_LOST:
		OnClose(event);
		break;
	}
}

void CTransferSocket::OnConnect(wxSocketEvent &event)
{
	m_pControlSocket->SetAlive();
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("OnConnect"));
	if (m_pSocketServer)
	{
		wxSocketBase *pSocket = m_pSocketServer->Accept(false);
		if (!pSocket)
			return;
		delete m_pSocketServer;
		m_pSocketServer = 0;
		m_pSocket = pSocket;

		m_pSocket->SetEventHandler(*this);
		m_pSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
		m_pSocket->Notify(true);
	}
}

void CTransferSocket::OnReceive()
{
	if (!m_pSocket || !m_bActive)
		return;

	if (m_transferMode == list)
	{
		char *pBuffer = new char[4096];
		m_pSocket->Read(pBuffer, 4096);
		if (m_pSocket->Error())
		{
			delete [] pBuffer;
			int error = m_pSocket->LastError();
			if (error == wxSOCKET_NOERROR)
				TransferEnd(0);
			else if (error != wxSOCKET_WOULDBLOCK)
				TransferEnd(1);
			return;
		}
		int numread = m_pSocket->LastCount();

		if (numread > 0)
		{
			m_pDirectoryListingParser->AddData(pBuffer, numread);
			m_pEngine->SetActive(true);
			m_pControlSocket->UpdateTransferStatus(numread);
		}
		else
		{
			delete [] pBuffer;
			if (!numread)
				TransferEnd(0);
			else if (numread < 0)
				TransferEnd(1);
		}
	}
	else if (m_transferMode == download)
	{
		if (!CheckGetNextWriteBuffer())
			return;
		
		m_pSocket->Read(m_pTransferBuffer, m_transferBufferLen);
		if (m_pSocket->Error())
		{
			int error = m_pSocket->LastError();
			if (error == wxSOCKET_NOERROR)
				FinalizeWrite();
			else if (error != wxSOCKET_WOULDBLOCK)
				TransferEnd(1);
			return;
		}
		int numread = m_pSocket->LastCount();

		if (numread > 0)
		{
			m_pEngine->SetActive(true);
			m_pControlSocket->UpdateTransferStatus(numread);

			m_pTransferBuffer += numread;
			m_transferBufferLen -= numread;

			CheckGetNextWriteBuffer();
		}
		else //!numread
			FinalizeWrite();
	}
	else if (m_transferMode == resumetest)
	{
		char buffer[2];
		m_pSocket->Read(buffer, 2);
		if (m_pSocket->Error())
		{
			if (m_pSocket->LastError() != wxSOCKET_WOULDBLOCK)
				TransferEnd(1);
			return;
		}
		int numread = m_pSocket->LastCount();
		if (!numread)
		{
			TransferEnd(0);
			return;
		}
		m_transferBufferLen += numread;

		if (m_transferBufferLen > 1)
			TransferEnd(2);
	}
}

void CTransferSocket::OnSend()
{
	if (!m_pSocket)
		return;

	if (!m_bActive)
		return;
	
	if (m_transferMode != upload)
		return;
	
	int numsent = 0;
	do
	{
		if (!CheckGetNextReadBuffer())
			return;

		m_pSocket->Write(m_pTransferBuffer, m_transferBufferLen);
		if (m_pSocket->Error())
			break;

		numsent = m_pSocket->LastCount();
		if (numsent > 0)
		{
			m_pEngine->SetActive(false);
			m_pControlSocket->UpdateTransferStatus(numsent);
		}

		m_pTransferBuffer += numsent;
		m_transferBufferLen -= numsent;
	}
	while (!m_pSocket->Error() && numsent > 0);
	
	if (m_pSocket->Error())
	{
		int error = m_pSocket->LastError();
		if (error != wxSOCKET_NOERROR && error != wxSOCKET_WOULDBLOCK)
		{
			m_pControlSocket->LogMessage(::Error, wxString::Format(_("Error %d writing to socket"), error));
			TransferEnd(1);
		}
	}
}

void CTransferSocket::OnClose(wxSocketEvent &event)
{
	m_pControlSocket->LogMessage(::Debug_Verbose, _T("OnClose"));
	m_onCloseCalled = true;
	m_pSocket->SetNotify(wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	char buffer;
	m_pSocket->Peek(&buffer, 1);
	while (!m_pSocket->Error() && m_pSocket->LastCount() == 1)
	{
		OnReceive();
		if (!m_pSocket)
			break;

		m_pSocket->Peek(&buffer, 1);
	}

	if (m_transferMode == resumetest)
	{
		if (m_transferBufferLen != 1)
		{
			TransferEnd(2);
			return;
		}
	}
	TransferEnd(0);
}

bool CTransferSocket::SetupPassiveTransfer(wxString host, int port)
{
	// Void all previous attempts to createt a socket
	if (!m_pSocketClient && !m_pSocketServer)
		delete m_pSocket;
	m_pSocket = 0;
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pSocketClient = new wxSocketClient(wxSOCKET_NOWAIT);

	m_pSocketClient->SetEventHandler(*this);
	m_pSocketClient->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	m_pSocketClient->Notify(true);

	wxIPV4address addr;
	addr.Hostname(host);
	addr.Service(port);

	bool res = m_pSocketClient->Connect(addr, false);

	if (!res && m_pSocketClient->LastError() != wxSOCKET_WOULDBLOCK)
	{
		delete m_pSocketClient;
		m_pSocketClient = 0;
		return false;
	}

	m_pSocket = m_pSocketClient;

	return true;
}

void CTransferSocket::SetActive()
{
	if (m_transferMode == download || m_transferMode == upload)
	{
		CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);;
		if (pData && pData->pIOThread)
			pData->pIOThread->SetEventHandler(this);
	}

	m_bActive = true;
	if (m_pSocket && m_pSocket->IsConnected())
	{
		OnReceive();
		OnSend();
	}
}

void CTransferSocket::TransferEnd(int reason)
{
	if (m_transferEnd)
		return;
	m_transferEnd = true;

	if (!m_pSocketClient && !m_pSocketServer)
		delete m_pSocket;
	m_pSocket = 0;
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pEngine->SendEvent(engineTransferEnd, reason);
}

wxSocketServer* CTransferSocket::CreateSocketServer()
{
	wxIPV4address addr, controladdr;
	addr.AnyAddress();

	if (!m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS))
	{
		// Ask the systen for a port
		addr.Service(0);
		wxSocketServer* pServer = new wxSocketServer(addr, wxSOCKET_NOWAIT | wxSOCKET_REUSEADDR);
		if (!pServer->Ok())
		{
			delete pServer;
			pServer = 0;
			return 0;
		}
		return pServer;
	}

	// Try out all ports in the port range.
	// Upon first call, we try to use a random port fist, after that
	// increase the port step by step
	
	// Windows only: I think there's a bug in the socket implementation of
	// Windows: Even if using wxSOCKET_REUSEADDR, using the same local address
	// twice will fail unless there are a couple of minutes between the 
	// connection attempts. This may cause problems if transferring lots of 
	// files with a narrow port range.

	static int start = 0;

	int low = m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS_LOW);
	int high = m_pEngine->GetOptions()->GetOptionVal(OPTION_LIMITPORTS_HIGH);

	if (start < low || start > high)
	{
		srand( (unsigned)time( NULL ) );
		start = rand() * (high - low) / (RAND_MAX + 1) + low;
	}

	for (int i = start; i <= high; i++)
	{
		addr.Service(i);
		wxSocketServer* pServer = new wxSocketServer(addr, wxSOCKET_NOWAIT | wxSOCKET_REUSEADDR);
		if (pServer->Ok())
		{
			start = i + 1;
			if (start > high)
				start = low;
			return pServer;
		}
		
		delete pServer;
	}

	for (int i = low; i < start; i++)
	{
		addr.Service(i);
		wxSocketServer* pServer = new wxSocketServer(addr, wxSOCKET_NOWAIT | wxSOCKET_REUSEADDR);
		if (pServer->Ok())
		{
			start = i + 1;
			if (start > high)
				start = low;
			return pServer;
		}
		
		delete pServer;
	}
	
	return 0;
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
			wxLongLong free;
			if (wxGetDiskSpace(pData->localFile, 0, &free))
			{
				if (free == 0)
					m_pControlSocket->LogMessage(::Error, _("Can't write data to file, disk is full."));
				else
					m_pControlSocket->LogMessage(::Error, _("Can't write data to file."));
			}
			else
				m_pControlSocket->LogMessage(::Error, _("Can't write data to file."));
			TransferEnd(1);
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
			TransferEnd(1);
			return false;
		}
		else if (res == IO_Success)
		{
			TransferEnd(0);
			return false;
		}
		m_transferBufferLen = res;
	}

	return true;
}

void CTransferSocket::OnIOThreadEvent(CIOThreadEvent& event)
{
	if (!m_bActive || m_transferEnd)
		return;

	if (m_transferMode == download)
		OnReceive();
	else if (m_transferMode == upload)
		OnSend();
}

void CTransferSocket::FinalizeWrite()
{
	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(static_cast<CRawTransferOpData *>(m_pControlSocket->m_pCurOpData)->pOldData);
	if (pData->pIOThread->Finalize(BUFFERSIZE - m_transferBufferLen))
		TransferEnd(0);
	else
	{
		m_pControlSocket->LogMessage(::Error, _("Can't write data to file."));
		TransferEnd(1);
	}
}
