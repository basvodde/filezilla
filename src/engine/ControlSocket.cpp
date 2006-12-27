#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"
#include <idna.h>
#include "asynchostresolver.h"
#include "directorycache.h"

DECLARE_EVENT_TYPE(fzOBTAINLOCK, -1)
DEFINE_EVENT_TYPE(fzOBTAINLOCK)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::list<CControlSocket::t_lockInfo> CControlSocket::m_lockInfoList;

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CControlSocket::OnSocketEvent)
	EVT_TIMER(wxID_ANY, CControlSocket::OnTimer)
	EVT_COMMAND(wxID_ANY, fzOBTAINLOCK, CControlSocket::OnObtainLock)
END_EVENT_TABLE();

COpData::COpData(enum Command op_Id)
	: opId(op_Id)
{
	opState = 0;

	pNextOpData = 0;

	waitForAsyncRequest = false;
}

COpData::~COpData()
{
	delete pNextOpData;
}

CControlSocket::CControlSocket(CFileZillaEnginePrivate *pEngine)
	: wxSocketClient(wxSOCKET_NOWAIT), CLogging(pEngine)
{
	m_pBackend = new CSocketBackend(this);
	m_socketId = 0;
	m_pEngine = pEngine;
	m_pCurOpData = 0;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;
	m_pCurrentServer = 0;
	m_pTransferStatus = 0;
	m_transferStatusSendState = 0;
	m_onConnectCalled = false;

	SetEvtHandlerEnabled(true);
	SetEventHandler(*this, m_socketId);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	Notify(true);

	m_pCSConv = 0;
	m_useUTF8 = false;

	m_timer.SetOwner(this);
	
	m_closed = false;
}

CControlSocket::~CControlSocket()
{
	DoClose();

	delete m_pBackend;
	m_pBackend = 0;

	delete m_pCSConv;
	m_pCSConv = 0;
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

		m_pBackend->Write(m_pSendBuffer, m_nSendBufferLen);
		if (m_pBackend->Error())
		{
			if (m_pBackend->LastError() != wxSOCKET_WOULDBLOCK)
				DoClose();
			return;
		}

		int numsent = m_pBackend->LastCount();

		if (numsent)
		{
			SetAlive();
			m_pEngine->SetActive(false);
		}

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
	LogMessage(Debug_Verbose, _T("CControlSocket::OnClose()"));

	if (GetCurrentCommandId() != cmd_connect)
		LogMessage(::Error, _("Disconnected from server"));
	DoClose();
}

int CControlSocket::Connect(const CServer &server)
{
	SetWait(true);

	if (server.GetEncodingType() == ENCODING_CUSTOM)
		m_pCSConv = new wxCSConv(server.GetCustomEncoding());

	if (m_pCurrentServer)
		delete m_pCurrentServer;
	m_pCurrentServer = new CServer(server);

	const wxString & host = server.GetHost();
	if (!IsIpAddress(host))
	{
		LogMessage(Status, _("Resolving IP-Address for %s"), host.c_str());
		CAsyncHostResolver *resolver = new CAsyncHostResolver(m_pEngine, ConvertDomainName(host));
		m_pEngine->AddNewAsyncHostResolver(resolver);

		resolver->Create();
		resolver->Run();
	}
	else
	{
		wxIPV4address addr;
		addr.Hostname(host);
		return ContinueConnect(&addr);
	}

	return FZ_REPLY_WOULDBLOCK;
}

int CControlSocket::ContinueConnect(const wxIPV4address *address)
{
	LogMessage(__TFILE__, __LINE__, this, Debug_Verbose, _T("CControlSocket::ContinueConnect(%p) m_pEngine=%p"), address, m_pEngine);
	if (GetCurrentCommandId() != cmd_connect ||
		!m_pCurrentServer)
	{
		LogMessage(Debug_Warning, _T("Invalid context for call to ContinueConnect(), cmd=%d, m_pCurrentServer=%p"), GetCurrentCommandId(), m_pCurrentServer);
		return DoClose(FZ_REPLY_INTERNALERROR);
	}
	
	if (!address)
	{
		LogMessage(::Error, _("Invalid hostname or host not found"));
		return DoClose(FZ_REPLY_ERROR | FZ_REPLY_CRITICALERROR);
	}

	const unsigned int port = m_pCurrentServer->GetPort();
	LogMessage(Status, _("Connecting to %s:%d..."), address->IPAddress().c_str(), port);

	wxIPV4address addr = *address;
	addr.Service(port);

	bool res = wxSocketClient::Connect(addr, false);

	if (!res && LastError() != wxSOCKET_WOULDBLOCK)
		return DoClose();

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
	if (event.GetId() != m_socketId)
		return;

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

	if (m_pCurOpData && m_pCurOpData->opId != cmd_rawtransfer)
	{
		UnlockCache();
	}

	if (m_pCurOpData && m_pCurOpData->pNextOpData && (nErrorCode & FZ_REPLY_INTERNALERROR) != FZ_REPLY_INTERNALERROR)
	{
		COpData *pNext = m_pCurOpData->pNextOpData;
		m_pCurOpData->pNextOpData = 0;
		delete m_pCurOpData;
		m_pCurOpData = pNext;
		return SendNextCommand(nErrorCode);
	}

	if ((nErrorCode & FZ_REPLY_CRITICALERROR) == FZ_REPLY_CRITICALERROR)
		LogMessage(::Error, _("Critical error"));

	if (m_pCurOpData)
	{
		const enum Command commandId = m_pCurOpData->opId;
		switch (commandId)
		{
		case cmd_none:
			break;
		case cmd_connect:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Connection attempt interrupted by user"));
			else if (nErrorCode != FZ_REPLY_OK)
				LogMessage(::Error, _("Could not connect to server"));
			break;
		case cmd_list:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Directory listing aborted by user"));
			else if (nErrorCode != FZ_REPLY_OK)
				LogMessage(::Error, _("Failed to retrieve directory listing"));
			else
				LogMessage(Status, _("Directory listing successful"));
			break;
		case cmd_transfer:
			{
				CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
				if (!pData->download && pData->transferInitiated)
				{
					CDirectoryCache cache;
					cache.UpdateFile(*m_pCurrentServer, pData->remotePath, pData->remoteFile, true, CDirectoryCache::file, (nErrorCode == FZ_REPLY_OK) ? pData->localFileSize : -1);

					m_pEngine->SendDirectoryListingNotification(pData->remotePath, false, true, false);
				}
				if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
					LogMessage(::Error, _("Transfer aborted by user"));
				else if (nErrorCode == FZ_REPLY_OK)
					LogMessage(Status, _("File transfer successful"));
			}
			break;
		default:
			if ((nErrorCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
				LogMessage(::Error, _("Interrupted by user"));
			break;
		}

		delete m_pCurOpData;
		m_pCurOpData = 0;
	}

	ResetTransferStatus();

	SetWait(false);

	return m_pEngine->ResetOperation(nErrorCode);
}

int CControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	if (m_closed)
	{
		wxASSERT(!m_pCurOpData);
		return nErrorCode;
	}
	
	m_closed = true;

	nErrorCode = ResetOperation(FZ_REPLY_ERROR | FZ_REPLY_DISCONNECTED | nErrorCode);
	
	ResetSocket();

	delete m_pCurrentServer;
	m_pCurrentServer = 0;

	return nErrorCode;
}

void CControlSocket::ResetSocket()
{
	Close();

	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;

	m_onConnectCalled = false;

	SetEventHandler(*this, ++m_socketId);
}

bool CControlSocket::Send(const char *buffer, int len)
{
	SetWait(true);
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
		m_pBackend->Write(buffer, len);
		int numsent = 0;
		if (m_pBackend->Error())
		{
			if (m_pBackend->LastError() != wxSOCKET_WOULDBLOCK)
			{
				DoClose();
				return false;
			}
		}
		else
			numsent = m_pBackend->LastCount();

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

	m_pTransferStatus->totalSize = totalSize;
	m_pTransferStatus->startOffset = startOffset;
	m_pTransferStatus->currentOffset = startOffset;
}

void CControlSocket::SetTransferStatusStartTime()
{
	if (!m_pTransferStatus)
		return;

	m_pTransferStatus->started = wxDateTime::Now();
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

bool CControlSocket::ParsePwdReply(wxString reply, bool unquoted /*=false*/, const CServerPath& defaultPath /*=CServerPath()*/)
{
	if (!unquoted)
	{
		int pos1 = reply.Find('"');
		int pos2 = reply.Find('"', true);
		if (pos1 == -1 || pos1 >= pos2)
		{
			int pos1 = reply.Find('\'');
			int pos2 = reply.Find('\'', true);

			if (pos1 != -1 && pos1 < pos2)
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Broken server sending single-quoted path instead of double-quoted path."));
		}
		if (pos1 == -1 || pos1 >= pos2)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No quoted path found in pwd reply, trying first token as path"));
			pos1 = reply.Find(' ');
			if (pos1 != -1)
			{
				pos2 = reply.Mid(pos1 + 1).Find(' ');
				if (pos2 == -1)
					pos2 = (int)reply.Length();
				reply = reply.Mid(pos1 + 1, pos2 - pos1 - 1);
			}
			else
				reply = _T("");
		}
		else
			reply = reply.Mid(pos1 + 1, pos2 - pos1 - 1);
	}

	m_CurrentPath.SetType(m_pCurrentServer->GetType());
	if (reply == _T("") || !m_CurrentPath.SetPath(reply))
	{
		if (reply != _T(""))
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Failed to parse returned path."));
		else
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Server returned empty path."));

		if (!defaultPath.IsEmpty())
		{
			LogMessage(Debug_Warning, _T("Assuming path is '%s'."), defaultPath.GetPath().c_str());
			m_CurrentPath = defaultPath;
			return true;
		}
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

	CDirentry entry;
	bool dirDidExist;
	bool matchedCase;
	CDirectoryCache cache;
	bool found = cache.LookupFile(entry, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, pData->remoteFile, dirDidExist, matchedCase);

	// Ignore entries with wrong case
	if (!matchedCase)
		found = false;

	if (!pData->download)
	{
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
	pNotification->ascii = !pData->transferSettings.binary;

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

	if (found)
	{
		if (!pData->fileTime.IsValid())
		{
			if (entry.hasDate)
			{
				pNotification->remoteTime = entry.time;
				pData->fileTime = entry.time;
			}
		}
	}

	pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
	pData->waitForAsyncRequest = true;

	m_pEngine->AddNotification(pNotification);

	return FZ_REPLY_WOULDBLOCK;
}

CFileTransferOpData::CFileTransferOpData() :
	COpData(cmd_transfer),
	localFileSize(-1), remoteFileSize(-1),
	tryAbsolutePath(false), resume(false), transferInitiated(false)
{
}

CFileTransferOpData::~CFileTransferOpData()
{
}

wxString CControlSocket::ConvToLocal(const char* buffer)
{
	if (m_useUTF8)
	{
		wxChar* out = ConvToLocalBuffer(buffer, wxConvUTF8);
		if (out)
		{
			wxString str = out;
			delete [] out;
			return str;
		}

		// Fall back to local charset on error
		if (m_pCurrentServer->GetEncodingType() != ENCODING_UTF8)
		{
			LogMessage(Status, _("Invalid character sequence received, disabling UTF-8. Select UTF-8 option in site manager to force UTF-8."));
			m_useUTF8 = false;
		}
	}
	
	if (m_pCSConv)
	{
		wxChar* out = ConvToLocalBuffer(buffer, *m_pCSConv);
		if (out)
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
	if (!len || len == wxCONV_FAILED)
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

		// Fall back to local charset on error
		if (m_pCurrentServer->GetEncodingType() != ENCODING_UTF8)
		{
			LogMessage(Status, _("Invalid character sequence received, disabling UTF-8. Select UTF-8 option in site manager to force UTF-8."));
			m_useUTF8 = false;
		}
	}

	if (m_pCSConv)
	{
		wxChar* res = ConvToLocalBuffer(buffer, *m_pCSConv);
		if (res && *res)
			return res;
	}

	// Fallback: Conversion using current locale
#if wxUSE_UNICODE
	wxChar* res = ConvToLocalBuffer(buffer, *wxConvCurrent);
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

void CControlSocket::OnTimer(wxTimerEvent& event)
{
	int timeout = m_pEngine->GetOptions()->GetOptionVal(OPTION_TIMEOUT);
	if (!timeout)
		return;

	if (m_pCurOpData && m_pCurOpData->waitForAsyncRequest)
		return;

	if (m_stopWatch.Time() > (timeout * 1000))
	{
		
		LogMessage(::Error, _("Connection timed out"));
		DoClose(FZ_REPLY_TIMEOUT);
	}
}

void CControlSocket::SetAlive()
{
	m_stopWatch.Start();
}

void CControlSocket::SetWait(bool wait)
{
	if (wait)
	{
		if (m_timer.IsRunning())
			return;

		m_stopWatch.Start();
		m_timer.Start(1000);
	}
	else if (m_timer.IsRunning())
		m_timer.Stop();
}

int CControlSocket::SendNextCommand(int prevResult /*=FZ_REPLY_OK*/)
{
	ResetOperation(prevResult);
	return FZ_REPLY_ERROR;
}

const std::list<CControlSocket::t_lockInfo>::iterator CControlSocket::GetLockStatus()
{
	std::list<t_lockInfo>::iterator iter;
	for (iter = m_lockInfoList.begin(); iter != m_lockInfoList.end(); iter++)
		if (iter->pControlSocket == this)
			break;

	return iter;
}

bool CControlSocket::TryLockCache(const CServerPath& directory)
{
	wxASSERT(GetLockStatus() == m_lockInfoList.end());
	wxASSERT(m_pCurrentServer);

	t_lockInfo info;
	info.directory = directory;
	info.pControlSocket = this;
	info.waiting = false;

	// Try to find other instance holding the lock
	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != m_lockInfoList.end(); iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;

		if (directory == iter->directory)
		{
			// Some other instance is holding the lock
			info.waiting = true;
			break;
		}
	}

	m_lockInfoList.push_back(info);
	return !info.waiting;
}

void CControlSocket::UnlockCache()
{
	std::list<t_lockInfo>::iterator iter = GetLockStatus();
	if (iter == m_lockInfoList.end())
		return;

	CServerPath directory = iter->directory;
	m_lockInfoList.erase(iter);

	// Find other instance waiting for the lock
	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != m_lockInfoList.end(); iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;

		if (iter->directory != directory)
			continue;

		// Send notification
		wxCommandEvent evt(fzOBTAINLOCK);
		iter->pControlSocket->AddPendingEvent(evt);
		break;
	}
}

bool CControlSocket::ObtainLockFromEvent()
{
	std::list<t_lockInfo>::iterator own = GetLockStatus();
	if (own == m_lockInfoList.end())
		return false;

	if (!own->waiting)
		return false;

	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != own; iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;

		if (iter->directory == own->directory)
			return false;
	}

	own->waiting = false;

	return true;
}

void CControlSocket::OnObtainLock(wxCommandEvent& event)
{
	if (!ObtainLockFromEvent())
		return;

	SendNextCommand();

	UnlockCache();
}
