#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"
#include <idna.h>
#include "asynchostresolver.h"
#include "directorycache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CControlSocket::OnSocketEvent)
END_EVENT_TABLE();

COpData::COpData()
{
	opId = cmd_none;
	opState = 0;

	pNextOpData = 0;

	waitForAsyncRequest = false;
}

COpData::~COpData()
{
	delete [] pNextOpData;
}

CControlSocket::CControlSocket(CFileZillaEnginePrivate *pEngine)
	: wxSocketClient(wxSOCKET_NOWAIT), CLogging(pEngine)
{
	m_pEngine = pEngine;
	m_pCurOpData = 0;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;
	m_pCurrentServer = 0;
	m_pTransferStatus = 0;
	m_transferStatusSendState = 0;
	m_onConnectCalled = false;

	SetEvtHandlerEnabled(true);
	SetEventHandler(*this);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	Notify(true);

	m_pCSConv = 0;
	m_useUTF8 = 0;
}

CControlSocket::~CControlSocket()
{
	delete m_pCSConv;

	DoClose();
}

void CControlSocket::OnConnect(wxSocketEvent &event)
{
}

void CControlSocket::OnReceive(wxSocketEvent &event)
{
}

void CControlSocket::OnSend(wxSocketEvent &event)
{
	if (m_pSendBuffer)
	{
		if (!m_nSendBufferLen)
		{
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;
			return;
		}

		Write(m_pSendBuffer, m_nSendBufferLen);
		if (Error())
		{
			if (LastError() != wxSOCKET_WOULDBLOCK)
				DoClose();
			return;
		}

		int numsent = LastCount();

		if (numsent)
			m_pEngine->SetActive(false);

		if (numsent == m_nSendBufferLen)
		{
			m_nSendBufferLen = 0;
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;
		}
		else
		{
			memmove(m_pSendBuffer, m_pSendBuffer + numsent, m_nSendBufferLen - numsent);
			m_nSendBufferLen -= numsent;
		}
	}
}

void CControlSocket::OnClose(wxSocketEvent &event)
{
	if (GetCurrentCommandId() != cmd_connect)
		LogMessage(::Error, _("Disconnected from server"));
	DoClose();
}

int CControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());

	if (server.GetEncodingType() == ENCODING_CUSTOM)
		m_pCSConv = new wxCSConv(server.GetCustomEncoding());

	CAsyncHostResolver *resolver = new CAsyncHostResolver(m_pEngine, ConvertDomainName(server.GetHost()));
	m_pEngine->AddNewAsyncHostResolver(resolver);

	resolver->Create();
	resolver->Run();

	if (m_pCurrentServer)
		delete m_pCurrentServer;
	m_pCurrentServer = new CServer(server);

	return FZ_REPLY_WOULDBLOCK;
}

int CControlSocket::ContinueConnect(const wxIPV4address *address)
{
	LogMessage(__TFILE__, __LINE__, this, Debug_Verbose, _T("ContinueConnect(%d) cmd=%d, m_pEngine=%d, m_pCurrentServer=%d"), (int)address, GetCurrentCommandId(), m_pEngine, m_pCurrentServer);
	if (GetCurrentCommandId() != cmd_connect ||
		!m_pCurrentServer)
	{
		LogMessage(Debug_Warning, _T("Invalid context for call to ContinueConnect()"));
		return DoClose(FZ_REPLY_INTERNALERROR);
	}
	
	if (!address)
	{
		LogMessage(::Error, _("Invalid hostname or host not found"));
		return ResetOperation(FZ_REPLY_ERROR | FZ_REPLY_CRITICALERROR);
	}

	wxIPV4address addr = *address;
	addr.Service(m_pCurrentServer->GetPort());

	bool res = wxSocketClient::Connect(addr, false);

	if (!res && LastError() != wxSOCKET_WOULDBLOCK)
		return ResetOperation(FZ_REPLY_ERROR);

	return FZ_REPLY_WOULDBLOCK;
}

int CControlSocket::Disconnect()
{
	LogMessage(Status, _("Disconnected from server"));

	DoClose();
	return FZ_REPLY_OK;
}

void CControlSocket::OnSocketEvent(wxSocketEvent &event)
{
	switch (event.GetSocketEvent())
	{
	case wxSOCKET_CONNECTION:
		m_onConnectCalled = true;
		OnConnect(event);
		break;
	case wxSOCKET_INPUT:
		if (!m_onConnectCalled)
		{
			m_onConnectCalled = true;
			OnConnect(event);
		}
		OnReceive(event);
		break;
	case wxSOCKET_OUTPUT:
		OnSend(event);
		break;
	case wxSOCKET_LOST:
		OnClose(event);
		break;
	}
}

enum Command CControlSocket::GetCurrentCommandId() const
{
	if (m_pCurOpData)
		return m_pCurOpData->opId;

	return m_pEngine->GetCurrentCommandId();
}

int CControlSocket::ResetOperation(int nErrorCode)
{
	LogMessage(Debug_Verbose, _T("CControlSocket::ResetOperation(%d)"), nErrorCode);

	if (nErrorCode & FZ_REPLY_WOULDBLOCK)
	{
		LogMessage(::Debug_Warning, _T("ResetOperation with FZ_REPLY_WOULDBLOCK in nErrorCode (%d)"), nErrorCode);
	}

	if (m_pCurOpData && m_pCurOpData->pNextOpData)
	{
		COpData *pNext = m_pCurOpData->pNextOpData;
		m_pCurOpData->pNextOpData = 0;
		delete m_pCurOpData;
		m_pCurOpData = pNext;
		return SendNextCommand(nErrorCode);
	}
	
	if (m_pCurOpData)
	{
		if (m_pCurOpData->opId == cmd_transfer)
		{
			CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
			if (!pData->download && pData->transferInitiated)
			{
				CDirectoryCache cache;
				cache.InvalidateFile(*m_pCurrentServer, pData->remotePath, pData->remoteFile, true, CDirectoryCache::file, (nErrorCode == FZ_REPLY_OK) ? pData->localFileSize : -1);

				m_pEngine->ResendModifiedListings();
			}
		}

		delete m_pCurOpData;
		m_pCurOpData = 0;
	}

	if (nErrorCode != FZ_REPLY_OK)
	{
		if ((nErrorCode & FZ_REPLY_CRITICALERROR) == FZ_REPLY_CRITICALERROR)
			LogMessage(::Error, _("Critical error"));
		const enum Command commandId = GetCurrentCommandId();
		switch (commandId)
		{
		case cmd_connect:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Connection attempt interrupted by user"));
			else
				LogMessage(::Error, _("Could not connect to server"));
			break;
		case cmd_list:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Directory listing aborted by user"));
			else
				LogMessage(::Error, _("Failed to retrieve directory listing"));
			break;
		case cmd_none:
			break;
		default:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Interrupted by user"));
			break;
		}
	}
	else
	{
		switch (GetCurrentCommandId())
		{
		case cmd_list:
			LogMessage(Status, _("Directory listing successful"));
			break;
		case cmd_transfer:
			LogMessage(Status, _("File transfer successful"));
			break;
		default:
			break;
		}
	}

	ResetTransferStatus();

	return m_pEngine->ResetOperation(nErrorCode);
}

int CControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	nErrorCode = ResetOperation(FZ_REPLY_ERROR | FZ_REPLY_DISCONNECTED | nErrorCode);
	Close();

	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;

	delete m_pCurrentServer;
	m_pCurrentServer = 0;
	
	m_onConnectCalled = false;

	SendDirectoryListing(0);

	return nErrorCode;
}

bool CControlSocket::Send(const char *buffer, int len)
{
	if (m_pSendBuffer)
	{
		char *tmp = m_pSendBuffer;
		m_pSendBuffer = new char[m_nSendBufferLen + len];
		memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
		memcpy(m_pSendBuffer + m_nSendBufferLen, buffer, len);
		m_nSendBufferLen += len;
		delete [] tmp;
	}
	else
	{
		Write(buffer, len);
		int numsent = 0;
		if (Error())
		{
			if (LastError() != wxSOCKET_WOULDBLOCK)
			{
				DoClose();
				return false;
			}
		}
		else
			numsent = LastCount();

		if (numsent)
			m_pEngine->SetActive(false);

		if (numsent < len)
		{
			char *tmp = m_pSendBuffer;
			m_pSendBuffer = new char[m_nSendBufferLen + len - numsent];
			memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
			memcpy(m_pSendBuffer + m_nSendBufferLen, buffer, len - numsent);
			m_nSendBufferLen += len - numsent;
			delete [] tmp;
		}
	}

	return true;
}

wxString CControlSocket::ConvertDomainName(wxString domain)
{
	const wxWCharBuffer buffer = wxConvCurrent->cWX2WC(domain);

	int len = 0;
	while (buffer.data()[len])
		len++;

	char *utf8 = new char[len * 2 + 2];
	wxMBConvUTF8 conv;
	conv.WC2MB(utf8, buffer, len * 2 + 2);

	char *output;
	if (idna_to_ascii_8z(utf8, &output, IDNA_ALLOW_UNASSIGNED))
	{
		delete [] utf8;
		LogMessage(::Debug_Warning, _T("Could not convert domain name"));
		return domain;
	}
	delete [] utf8;

	wxString result = wxConvCurrent->cMB2WX(output);
	free(output);
	return result;
}

void CControlSocket::Cancel()
{
	if (GetCurrentCommandId() != cmd_none)
	{
		if (GetCurrentCommandId() == cmd_connect)
			DoClose(FZ_REPLY_CANCELED);
		else
			ResetOperation(FZ_REPLY_CANCELED);
	}
}

void CControlSocket::ResetTransferStatus()
{
	delete m_pTransferStatus;
	m_pTransferStatus = 0;

	m_pEngine->AddNotification(new CTransferStatusNotification(0));

	m_transferStatusSendState = 0;
}

void CControlSocket::InitTransferStatus(wxFileOffset totalSize, wxFileOffset startOffset)
{
	if (startOffset < 0)
		startOffset = 0;

	delete m_pTransferStatus;
	m_pTransferStatus = new CTransferStatus();

	m_pTransferStatus->started = wxDateTime::Now();
	m_pTransferStatus->totalSize = totalSize;
	m_pTransferStatus->startOffset = startOffset;
	m_pTransferStatus->currentOffset = startOffset;
}

void CControlSocket::UpdateTransferStatus(wxFileOffset transferredBytes)
{
	if (!m_pTransferStatus)
		return;

	m_pTransferStatus->currentOffset += transferredBytes;

	if (!m_transferStatusSendState)
		m_pEngine->AddNotification(new CTransferStatusNotification(new CTransferStatus(*m_pTransferStatus)));
	m_transferStatusSendState = 2;
}

bool CControlSocket::GetTransferStatus(CTransferStatus &status, bool &changed)
{
	if (!m_pTransferStatus)
	{
		changed = false;
		m_transferStatusSendState = 0;
		return false;
	}

	status = *m_pTransferStatus;
	if (m_transferStatusSendState == 2)
	{
		changed = true;
		m_transferStatusSendState = 1;
		return true;
	}
	else
	{
		changed = false;
		m_transferStatusSendState = 0;
		return true;
	}
}


const CServer* CControlSocket::GetCurrentServer() const
{
	return m_pCurrentServer;
}

void CControlSocket::SendDirectoryListing(CDirectoryListing* pListing)
{
	m_pEngine->SendDirectoryListing(pListing);
}

bool CControlSocket::ParsePwdReply(wxString reply, bool unquoted /*=false*/)
{
	if (!unquoted)
	{
		int pos1 = reply.Find('"');
		int pos2 = reply.Find('"', true);
		if (pos1 == -1 || pos1 >= pos2)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No quoted path found in pwd reply, trying first token as path"));
			pos1 = reply.Find(' ');
			if (pos1 == -1)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Can't parse path"));
				return false;
			}

			pos2 = reply.Mid(pos1 + 1).Find(' ');
			if (pos2 == -1)
				pos2 = (int)reply.Length();
		}
		reply = reply.Mid(pos1 + 1, pos2 - pos1 - 1);
	}

	m_CurrentPath.SetType(m_pCurrentServer->GetType());
	if (!m_CurrentPath.SetPath(reply))
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Can't parse path"));
		return false;
	}

	return true;
}

int CControlSocket::CheckOverwriteFile()
{
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);

	if (pData->download)
	{
		if (!wxFile::Exists(pData->localFile))
			return FZ_REPLY_OK;
	}

	CDirectoryListing listing;
	CDirectoryCache cache;
	bool foundListing = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);

	bool found = false;
	if (!pData->download)
	{
		if (foundListing)
		{
			for (unsigned int i = 0; i < listing.m_entryCount; i++)
			{
				if (listing.m_pEntries[i].name == pData->remoteFile)
				{
					found = true;
					break;
				}
			}
		}
		if (!found && pData->remoteFileSize == -1 && !pData->fileTime.IsValid())
			return FZ_REPLY_OK;
	}

	CFileExistsNotification *pNotification = new CFileExistsNotification;

	pNotification->download = pData->download;
	pNotification->localFile = pData->localFile;
	pNotification->remoteFile = pData->remoteFile;
	pNotification->remotePath = pData->remotePath;
	pNotification->localSize = pData->localFileSize;
	pNotification->remoteSize = pData->remoteFileSize;

	wxStructStat buf;
	int result;
	result = wxStat(pData->localFile, &buf);
	if (!result)
	{
		pNotification->localTime = wxDateTime(buf.st_mtime);
		if (!pNotification->localTime.IsValid())
			pNotification->localTime = wxDateTime(buf.st_ctime);
	}

	if (pData->fileTime.IsValid())
		pNotification->remoteTime = pData->fileTime;

	if (foundListing)
	{
		for (unsigned int i = 0; i < listing.m_entryCount; i++)
		{
			if (listing.m_pEntries[i].name == pData->remoteFile)
			{
				if (!pData->fileTime.IsValid())
				{
					if (listing.m_pEntries[i].hasDate)
					{
						if (VerifySetDate(pNotification->remoteTime, listing.m_pEntries[i].date.year, (enum wxDateTime::Month)(listing.m_pEntries[i].date.month - 1), listing.m_pEntries[i].date.day) && 
							listing.m_pEntries[i].hasTime)
						{
							pNotification->remoteTime.SetHour(listing.m_pEntries[i].time.hour);
							pNotification->remoteTime.SetMinute(listing.m_pEntries[i].time.minute);
						}
						if (pNotification->remoteTime.IsValid())
							pData->fileTime = pNotification->remoteTime;
					}
				}
			}
		}
	}


	pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
	pData->waitForAsyncRequest = true;

	m_pEngine->AddNotification(pNotification);

	return FZ_REPLY_WOULDBLOCK;
}

CFileTransferOpData::CFileTransferOpData() :
	localFileSize(-1), remoteFileSize(-1),
	tryAbsolutePath(false), resume(false), transferInitiated(false)
{
	opId = cmd_transfer;
}

CFileTransferOpData::~CFileTransferOpData()
{
}

wxString CControlSocket::ConvToLocal(const char* buffer)
{
	if (m_useUTF8)
	{
		wxChar* out = ConvToLocalBuffer(buffer, wxConvUTF8);
		if (buffer)
		{
			wxString str = out;
			delete [] out;
			return str;
		}
	}
	
	if (m_pCSConv)
	{
		wxChar* out = ConvToLocalBuffer(buffer, *m_pCSConv);
		if (buffer)
		{
			wxString str = out;
			delete [] out;
			return str;
		}
	}

	wxString str = wxConvCurrent->cMB2WX(buffer);

	return str;
}

wxChar* CControlSocket::ConvToLocalBuffer(const char* buffer, wxMBConv& conv)
{
	size_t len = conv.MB2WC(0, buffer, 0);
	if (!len)
		return 0;

	wchar_t* unicode = new wchar_t[len + 1];
	conv.MB2WC(unicode, buffer, len + 1);
#if wxUSE_UNICODE
	return unicode;
#else
	len = wxConvCurrent->WC2MB(0, unicode, 0);
	if (!len)
	{
		delete [] unicode;
		return 0;
	}

	wxChar* output = new wxChar[len + 1];
	wxConvCurrent->WC2MB(output, unicode, len + 1);
	delete [] unicode;
	return output;
#endif
}

wxChar* CControlSocket::ConvToLocalBuffer(const char* buffer)
{
	if (m_useUTF8)
	{
		wxChar* res = ConvToLocalBuffer(buffer, wxConvUTF8);
		if (res && *res)
			return res;
	}

	if (m_pCSConv)
	{
		wxChar* res = ConvToLocalBuffer(buffer, *m_pCSConv);
		if (res && *res)
			return res;
	}

	// Fallback: Conversion using current locale
#if wxUSE_UNICODE
	wxChar* res = ConvToLocalBuffer(buffer, wxConvUTF8);
#else
	// No conversion needed, just copy
	wxChar* res = new wxChar[strlen(buffer) + 1];
	strcpy(res, buffer);
#endif

	return res;
}

wxCharBuffer CControlSocket::ConvToServer(const wxString& str)
{
	if (m_useUTF8)
	{
#if wxUSE_UNICODE
		wxCharBuffer buffer = wxConvUTF8.cWX2MB(str);
#else
		wxWCharBuffer unicode = wxConvCurrent->cMB2WC(str);
		wxCharBuffer buffer;
		if (unicode)
			 buffer = wxConvUTF8.cWC2MB(unicode);
#endif

		if (buffer)
			return buffer;
	}

	if (m_pCSConv)
	{
#if wxUSE_UNICODE
		wxCharBuffer buffer = m_pCSConv->cWX2MB(str);
#else
		wxWCharBuffer unicode = wxConvCurrent->cMB2WC(str);
		wxCharBuffer buffer;
		if (unicode)
			 buffer = m_pCSConv->cWC2MB(unicode);
#endif

		if (buffer)
			return buffer;
	}

	wxCharBuffer buffer = wxConvCurrent->cWX2MB(str);
	if (!buffer)
		buffer = wxCSConv(_T("ISO8859-1")).cWX2MB(str);

	return buffer;
}

wxString CControlSocket::GetLocalIP() const
{
	wxIPV4address addr;
	if (!GetLocal(addr))
		return _T("");

	return addr.IPAddress();
}

wxString CControlSocket::GetPeerIP() const
{
	wxIPV4address addr;
	if (!GetPeer(addr))
		return _T("");

	return addr.IPAddress();
}
