#include "FileZilla.h"
#include "transfersocket.h"
#include "ftpcontrolsocket.h"
#include "directorylistingparser.h"

#include <wx/file.h>

BEGIN_EVENT_TABLE(CTransferSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CTransferSocket::OnSocketEvent)
END_EVENT_TABLE();

#define BUFFERSIZE 65535

CTransferSocket::CTransferSocket(CFileZillaEngine *pEngine, CFtpControlSocket *pControlSocket, enum TransferMode transferMode)
{
	m_pEngine = pEngine;
	m_pControlSocket = pControlSocket;

	m_pSocketClient = 0;
	m_pSocketServer = 0;
	m_pSocket = 0;

	m_bActive = false;

	SetEvtHandlerEnabled(true);

	if (transferMode == list)
		m_pDirectoryListingParser = new CDirectoryListingParser(pEngine);
	else
		m_pDirectoryListingParser = 0;

	m_transferMode = transferMode;

	m_pTransferBuffer = 0;
	m_transferBufferPos = 0;
}

CTransferSocket::~CTransferSocket()
{
	delete m_pSocketClient;
	delete m_pSocketServer;
	if (!m_pSocketClient && !m_pSocketServer)
		delete m_pSocket;

	delete m_pDirectoryListingParser;
	
	delete [] m_pTransferBuffer;
}

wxString CTransferSocket::SetupActiveTransfer()
{
	// Void all previous attempts to createt a socket
	if (!m_pSocketClient && !m_pSocketServer)
	{
		delete m_pSocket;
		m_pSocket = 0;
	}
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;
	
	wxIPV4address addr, controladdr;
	addr.AnyAddress();
	addr.Service(0);

	m_pSocketServer = new wxSocketServer(addr, wxSOCKET_NOWAIT);
	if (!m_pSocketServer->Ok() || !m_pSocketServer->GetLocal(addr) || !m_pControlSocket->GetLocal(controladdr))
	{
		delete m_pSocketServer;
		m_pSocketServer = 0;
		return _T("");
	}
	wxString port = controladdr.IPAddress() + wxString::Format(_T(",%d,%d"), addr.Service() / 256, addr.Service() % 256);
	port.Replace(_T("."), _T(","));

	m_pSocketServer->SetEventHandler(*this);
	m_pSocketServer->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	m_pSocketServer->Notify(true);

	return port;
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
			m_pEngine->SetActive(true);
			m_pDirectoryListingParser->AddData(pBuffer, numread);
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
		CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pControlSocket->m_pCurOpData);
		if (!m_pTransferBuffer)
			m_pTransferBuffer = new char[4096];

		m_pSocket->Read(m_pTransferBuffer, 4096);
		if (m_pSocket->Error())
		{
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
			m_pEngine->SetActive(true);
			if (pData->leftSize > 0)
			{
				if (pData->leftSize > numread)
					pData->leftSize -= numread;
				else
					pData->leftSize = 0;
			}
			if (pData->pFile->Write(m_pTransferBuffer, numread) != static_cast<size_t>(numread))
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
			}
		}
	}
}

void CTransferSocket::OnSend()
{
	if (!m_bActive)
		return;
	
	if (m_transferMode != upload)
		return;
	
	if (!m_pTransferBuffer)
		m_pTransferBuffer = new char[BUFFERSIZE];
	
	CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pControlSocket->m_pCurOpData);
	int numsent;
	do
	{
		if (m_transferBufferPos * 2 < BUFFERSIZE)
		{
			if (pData->pFile)
			{
				int numread = pData->pFile->Read(m_pTransferBuffer + m_transferBufferPos, BUFFERSIZE - m_transferBufferPos);
				if (numread < BUFFERSIZE - m_transferBufferPos)
				{
					delete pData->pFile;
					pData->pFile = 0;
				}
				if (numread > 0)
					m_transferBufferPos += numread;
				
				if (numread < 0)
				{
					m_pControlSocket->LogMessage(::Error, _("Can't read from file"));
					TransferEnd(1);
					return;
				}
				else if (!numread && !m_transferBufferPos)
				{
					TransferEnd(0);
					return;
				}
			}
			else if (!m_transferBufferPos)
			{
				TransferEnd(0);
				return;
			}
		}
		m_pSocket->Write(m_pTransferBuffer, m_transferBufferPos);
		if (!m_pSocket->Error())
		{
			numsent = m_pSocket->LastCount();
			if (numsent > 0)
				m_pEngine->SetActive(true);

			if (pData->leftSize > 0)
			{
				if (pData->leftSize > numsent)
					pData->leftSize -= numsent;
				else
					pData->leftSize = 0;
				
				memmove(m_pTransferBuffer, m_pTransferBuffer + numsent, m_transferBufferPos - numsent);
				m_transferBufferPos -= numsent;
			}
		}
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
	else if (!pData->pFile && !m_transferBufferPos)
		TransferEnd(0);
}

void CTransferSocket::OnClose(wxSocketEvent &event)
{
	TransferEnd(0);
}

bool CTransferSocket::SetupPassiveTransfer(wxString host, int port)
{
	// Void all previous attempts to createt a socket
	if (!m_pSocketClient && !m_pSocketServer)
	{
		delete m_pSocket;
		m_pSocket = 0;
	}
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pSocketClient = new wxSocketClient(wxSOCKET_NOWAIT);

	m_pSocketClient->SetEventHandler(*this);
	m_pSocketClient->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);

	wxIPV4address addr;
	addr.Hostname(host);
	addr.Service(port);

	bool res = m_pSocketClient->Connect(addr, false);

	if (!res && m_pSocketClient->LastError() != wxSOCKET_WOULDBLOCK)
		return false;

	m_pSocket = m_pSocketClient;

	return true;
}

void CTransferSocket::SetActive()
{
	m_bActive = true;
	if (m_pSocket && m_pSocket->IsConnected())
	{
		OnReceive();
		OnSend();
	}
}

void CTransferSocket::TransferEnd(int reason)
{
	if (!m_pSocket)
		return;
	if (!m_pSocketClient && !m_pSocketServer)
	{
		delete m_pSocket;
		m_pSocket = 0;
	}
	delete m_pSocketClient;
	m_pSocketClient = 0;
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pEngine->SendEvent(engineTransferEnd, reason);
}
