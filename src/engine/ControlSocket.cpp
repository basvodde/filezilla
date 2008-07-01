#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"
#include <idna.h>
#include "directorycache.h"
#include "servercapabilities.h"
#include "local_filesys.h"
#include <errno.h>

DECLARE_EVENT_TYPE(fzOBTAINLOCK, -1)
DEFINE_EVENT_TYPE(fzOBTAINLOCK)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::list<CControlSocket::t_lockInfo> CControlSocket::m_lockInfoList;

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_TIMER(wxID_ANY, CControlSocket::OnTimer)
	EVT_COMMAND(wxID_ANY, fzOBTAINLOCK, CControlSocket::OnObtainLock)
END_EVENT_TABLE();

COpData::COpData(enum Command op_Id)
	: opId(op_Id)
{
	opState = 0;

	pNextOpData = 0;

	waitForAsyncRequest = false;

	holdsLock = false;
}

COpData::~COpData()
{
	delete pNextOpData;
}

CControlSocket::CControlSocket(CFileZillaEnginePrivate *pEngine)
	: CLogging(pEngine)
{
	m_pEngine = pEngine;
	m_pCurOpData = 0;
	m_pCurrentServer = 0;
	m_pTransferStatus = 0;
	m_transferStatusSendState = 0;

	m_pCSConv = 0;
	m_useUTF8 = false;

	m_timer.SetOwner(this);

	m_closed = false;

	m_invalidateCurrentPath = false;
}

CControlSocket::~CControlSocket()
{
	DoClose();

	delete m_pCSConv;
	m_pCSConv = 0;
}

int CControlSocket::Disconnect()
{
	LogMessage(Status, _("Disconnected from server"));

	DoClose();
	return FZ_REPLY_OK;
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

	if (m_pCurOpData && m_pCurOpData->holdsLock)
		UnlockCache();

	if (m_pCurOpData && m_pCurOpData->pNextOpData)
	{
		COpData *pNext = m_pCurOpData->pNextOpData;
		m_pCurOpData->pNextOpData = 0;
		delete m_pCurOpData;
		m_pCurOpData = pNext;
		if (nErrorCode == FZ_REPLY_OK ||
			nErrorCode == FZ_REPLY_ERROR ||
			nErrorCode == FZ_REPLY_CRITICALERROR)
		{
			return ParseSubcommandResult(nErrorCode);
		}
		else
			return ResetOperation(nErrorCode);
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
					if (!m_pCurrentServer)
						LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurrentServer is 0"));
					else
					{
						CDirectoryCache cache;
						bool updated = cache.UpdateFile(*m_pCurrentServer, pData->remotePath, pData->remoteFile, true, CDirectoryCache::file, (nErrorCode == FZ_REPLY_OK) ? pData->localFileSize : -1);

						if (updated)
							m_pEngine->SendDirectoryListingNotification(pData->remotePath, false, true, false);
					}
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

	if (m_invalidateCurrentPath)
	{
		m_CurrentPath.Clear();
		m_invalidateCurrentPath = false;
	}

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

	delete m_pCurrentServer;
	m_pCurrentServer = 0;

	return nErrorCode;
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

void CControlSocket::InitTransferStatus(wxFileOffset totalSize, wxFileOffset startOffset, bool list)
{
	if (startOffset < 0)
		startOffset = 0;

	delete m_pTransferStatus;
	m_pTransferStatus = new CTransferStatus();

	m_pTransferStatus->list = list;
	m_pTransferStatus->totalSize = totalSize;
	m_pTransferStatus->startOffset = startOffset;
	m_pTransferStatus->currentOffset = startOffset;
	m_pTransferStatus->madeProgress = false;
}

void CControlSocket::SetTransferStatusStartTime()
{
	if (!m_pTransferStatus)
		return;

	m_pTransferStatus->started = wxDateTime::Now();
}

void CControlSocket::SetTransferStatusMadeProgress()
{
	if (!m_pTransferStatus)
		return;

	m_pTransferStatus->madeProgress = true;
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
			pos1 = reply.Find('\'');
			pos2 = reply.Find('\'', true);

			if (pos1 != -1 && pos1 < pos2)
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Broken server sending single-quoted path instead of double-quoted path."));
		}
		if (pos1 == -1 || pos1 >= pos2)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Broken server, no quoted path found in pwd reply, trying first token as path"));
			pos1 = reply.Find(' ');
			if (pos1 != -1)
			{
				reply = reply.Mid(pos1 + 1);
				pos2 = reply.Find(' ');
				if (pos2 != -1)
					reply = reply.Left(pos2);
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
	CServerPath remotePath;
	if (pData->tryAbsolutePath || m_CurrentPath.IsEmpty())
		remotePath = pData->remotePath;
	else
		remotePath = m_CurrentPath;
	bool found = cache.LookupFile(entry, *m_pCurrentServer, remotePath, pData->remoteFile, dirDidExist, matchedCase);

	// Ignore entries with wrong case
	if (found && !matchedCase)
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

	if (pData->download && pNotification->localSize != -1)
		pNotification->canResume = true;
	else if (!pData->download && pNotification->remoteSize != -1)
		pNotification->canResume = true;
	else
		pNotification->canResume = false;

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

	SendAsyncRequest(pNotification);

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

	wxCSConv conv(_T("ISO-8859-1"));
	wxString str = conv.cMB2WX(buffer);
	if (str == _T(""))
		str = wxConvCurrent->cMB2WX(buffer);

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

wxCharBuffer CControlSocket::ConvToServer(const wxString& str, bool force_utf8 /*=false*/)
{
	if (m_useUTF8 || force_utf8)
	{
#if wxUSE_UNICODE
		wxCharBuffer buffer = wxConvUTF8.cWX2MB(str);
#else
		wxWCharBuffer unicode = wxConvCurrent->cMB2WC(str);
		wxCharBuffer buffer;
		if (unicode)
			 buffer = wxConvUTF8.cWC2MB(unicode);
#endif

		if (buffer || force_utf8)
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
		wxASSERT(!m_timer.IsRunning());
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

int CControlSocket::SendNextCommand()
{
	ResetOperation(FZ_REPLY_INTERNALERROR);
	return FZ_REPLY_ERROR;
}

int CControlSocket::ParseSubcommandResult(int prevResult)
{
	ResetOperation(FZ_REPLY_INTERNALERROR);
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

bool CControlSocket::TryLockCache(enum locking_reason reason, const CServerPath& directory)
{
	wxASSERT(m_pCurrentServer);
	wxASSERT(m_pCurOpData);

	std::list<t_lockInfo>::iterator own = GetLockStatus();
	if (own == m_lockInfoList.end())
	{
		t_lockInfo info;
		info.directory = directory;
		info.pControlSocket = this;
		info.waiting = true;
		info.reason = reason;
		info.lockcount = 0;
		m_lockInfoList.push_back(info);
		own = --m_lockInfoList.end();
	}
	else
	{
		if (own->lockcount)
		{
			if (!m_pCurOpData->holdsLock)
			{
				m_pCurOpData->holdsLock = true;
				own->lockcount++;
			}
			return true;
		}
		wxASSERT(own->waiting);
		wxASSERT(own->reason == reason);
	}

	// Needs to be set in any case so that ResetOperation 
	// unlocks or cancels the lock wait
	m_pCurOpData->holdsLock = true;

	// Try to find other instance holding the lock
	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != own; iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;
		if (directory != iter->directory)
			continue;
		if (reason != iter->reason)
			continue;

		// Some other instance is holding the lock
		return false;
	}

	own->lockcount++;
	own->waiting = false;
	return true;
}

bool CControlSocket::IsLocked(enum locking_reason reason, const CServerPath& directory)
{
	wxASSERT(m_pCurrentServer);

	std::list<t_lockInfo>::iterator own = GetLockStatus();
	if (own != m_lockInfoList.end())
		return true;

	// Try to find other instance holding the lock
	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != own; iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;
		if (directory != iter->directory)
			continue;
		if (reason != iter->reason)
			continue;

		// Some instance is holding the lock
		return true;
	}

	return false;
}

void CControlSocket::UnlockCache()
{
	if (!m_pCurOpData || !m_pCurOpData->holdsLock)
		return;
	m_pCurOpData->holdsLock = false;

	std::list<t_lockInfo>::iterator iter = GetLockStatus();
	if (iter == m_lockInfoList.end())
		return;

	iter->lockcount--;
	if (iter->lockcount)
		return;

	CServerPath directory = iter->directory;
	enum locking_reason reason = iter->reason;

	m_lockInfoList.erase(iter);

	// Find other instance waiting for the lock
	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != m_lockInfoList.end(); iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;

		if (iter->directory != directory)
			continue;

		if (iter->reason != reason)
			continue;

		// Send notification
		wxCommandEvent evt(fzOBTAINLOCK);
		iter->pControlSocket->AddPendingEvent(evt);
		break;
	}
}

enum CControlSocket::locking_reason CControlSocket::ObtainLockFromEvent()
{
	wxASSERT(m_pCurOpData);

	std::list<t_lockInfo>::iterator own = GetLockStatus();
	if (own == m_lockInfoList.end())
		return lock_unknown;

	if (!own->waiting)
		return lock_unknown;

	for (std::list<t_lockInfo>::const_iterator iter = m_lockInfoList.begin(); iter != own; iter++)
	{
		if (*m_pCurrentServer != *iter->pControlSocket->m_pCurrentServer)
			continue;

		if (iter->directory != own->directory)
			continue;

		if (iter->reason != own->reason)
			continue;

		// Another instance comes before us
		return lock_unknown;
	}

	own->waiting = false;
	own->lockcount++;

	return own->reason;
}

void CControlSocket::OnObtainLock(wxCommandEvent& event)
{
	if (ObtainLockFromEvent() == lock_unknown)
		return;

	SendNextCommand();

	UnlockCache();
}

void CControlSocket::InvalidateCurrentWorkingDir(const CServerPath& path)
{
	wxASSERT(!path.IsEmpty());
	if (m_CurrentPath.IsEmpty())
		return;

	if (m_CurrentPath == path || path.IsParentOf(m_CurrentPath, false))
	{
		if (m_pCurOpData)
			m_invalidateCurrentPath = true;
		else
			m_CurrentPath.Clear();
	}
}

wxTimeSpan CControlSocket::GetTimezoneOffset()
{
	if (!m_pCurrentServer)
		return wxTimeSpan();

	int seconds = 0;
	if (CServerCapabilities::GetCapability(*m_pCurrentServer, timezone_offset, &seconds) != yes)
		return wxTimeSpan();

	return wxTimeSpan(0, 0, seconds);
}

void CControlSocket::SendAsyncRequest(CAsyncRequestNotification* pNotification)
{
	wxASSERT(pNotification);

	pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();

	if (m_pCurOpData)
		m_pCurOpData->waitForAsyncRequest = true;
	m_pEngine->AddNotification(pNotification);
}

// ------------------
// CRealControlSocket
// ------------------

BEGIN_EVENT_TABLE(CRealControlSocket, CControlSocket)
	EVT_FZ_SOCKET(wxID_ANY, CRealControlSocket::OnSocketEvent)
END_EVENT_TABLE()

CRealControlSocket::CRealControlSocket(CFileZillaEnginePrivate *pEngine)
	: CControlSocket(pEngine), CSocket(this, 0)
{
	m_pBackend = new CSocketBackend(this, this);

	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;
}

CRealControlSocket::~CRealControlSocket()
{
	Close();
	delete m_pBackend;
	m_pBackend = 0;
}

bool CRealControlSocket::Send(const char *buffer, int len)
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

void CRealControlSocket::OnSocketEvent(CSocketEvent &event)
{
	if (!m_pBackend)
		return;

	if (event.GetId() != m_pBackend->GetId())
		return;

	switch (event.GetType())
	{
	case CSocketEvent::hostaddress:
		{
			const wxString& address = event.GetData();
			LogMessage(Status, _("Connecting to %s..."), address.c_str()); 
		}
		break;
	case CSocketEvent::connection_next:
		if (event.GetError())
			LogMessage(Status, _("Connection attempt failed with \"%s\", trying next address."), CSocket::GetErrorDescription(event.GetError()).c_str()); 
		break;
	case CSocketEvent::connection:
		if (event.GetError())
		{
			LogMessage(Status, _("Connection attempt failed with \"%s\"."), CSocket::GetErrorDescription(event.GetError()).c_str()); 
			OnClose(event.GetError());
		}
		else
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
	default:
		LogMessage(Debug_Warning, _T("Unhandled socket event %d"), event.GetType());
		break;
	}
}

void CRealControlSocket::OnConnect()
{
}

void CRealControlSocket::OnReceive()
{
}

void CRealControlSocket::OnSend()
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

void CRealControlSocket::OnClose(int error)
{
	LogMessage(Debug_Verbose, _T("CRealControlSocket::OnClose(%d)"), error);

	if (GetCurrentCommandId() != cmd_connect)
	{
		if (!error)
			LogMessage(::Error, _("Connection closed by server"));
		else
			LogMessage(::Error, _("Disconnected from server: %s"), CSocket::GetErrorDescription(error).c_str());
	}
	DoClose();
}

int CRealControlSocket::Connect(const CServer &server)
{
	SetWait(true);

	if (server.GetEncodingType() == ENCODING_CUSTOM)
		m_pCSConv = new wxCSConv(server.GetCustomEncoding());

	if (m_pCurrentServer)
		delete m_pCurrentServer;
	m_pCurrentServer = new CServer(server);

	CConnectOpData* pData;
	if (!m_pCurOpData || m_pCurOpData->opId != cmd_connect)
		pData = 0;
	else
		pData = static_cast<CConnectOpData *>(m_pCurOpData);

	wxString host = pData ? pData->host : server.GetHost();
	const unsigned int port = pData ? pData->port : m_pCurrentServer->GetPort();

	// International domain names
	host = ConvertDomainName(host);

	LogMessage(Status, _("Resolving address of %s"), host.c_str());
	
	int res = CSocket::Connect(host, port);

	// Treat success same as EINPROGRESS, we wait for connect notification in any case
	if (res && res != EINPROGRESS)
	{
		LogMessage(::Error, _("Could not connect to server: %s"), CSocket::GetErrorDescription(res).c_str());
		DoClose();
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}

int CRealControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	ResetSocket();

	return CControlSocket::DoClose(nErrorCode);
}

void CRealControlSocket::ResetSocket()
{
	Close();

	if (m_pSendBuffer)
	{
		delete [] m_pSendBuffer;
		m_pSendBuffer = 0;
		m_nSendBufferLen = 0;
	}

	delete m_pBackend;
	m_pBackend = 0;
}

bool CControlSocket::SetFileExistsAction(CFileExistsNotification *pFileExistsNotification)
{
	wxASSERT(pFileExistsNotification);

	if (!m_pCurOpData || m_pCurOpData->opId != cmd_transfer)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No or invalid operation in progress, ignoring request reply %f"), pFileExistsNotification->GetRequestID());
		return false;
	}

	CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);

	switch (pFileExistsNotification->overwriteAction)
	{
	case CFileExistsNotification::overwrite:
		SendNextCommand();
		break;
	case CFileExistsNotification::overwriteNewer:
		if (!pFileExistsNotification->localTime.IsValid() || !pFileExistsNotification->remoteTime.IsValid())
			SendNextCommand();
		else if (pFileExistsNotification->download && pFileExistsNotification->localTime.IsEarlierThan(pFileExistsNotification->remoteTime))
			SendNextCommand();
		else if (!pFileExistsNotification->download && pFileExistsNotification->localTime.IsLaterThan(pFileExistsNotification->remoteTime))
			SendNextCommand();
		else
		{
			if (pData->download)
			{
				wxString filename = pData->remotePath.FormatFilename(pData->remoteFile);
				LogMessage(Status, _("Skipping download of %s"), filename.c_str());
			}
			else
			{
				LogMessage(Status, _("Skipping upload of %s"), pData->localFile.c_str());
			}
			ResetOperation(FZ_REPLY_OK);
		}
		break;
	case CFileExistsNotification::overwriteSize:
		/* First compare flags both size known but different, one size known and the other not (obviously they are different).
		Second compare flags the remaining case in which we need to send command : both size unknown */
		if ((pFileExistsNotification->localSize != pFileExistsNotification->remoteSize) || (pFileExistsNotification->localSize == -1))
			SendNextCommand();
		else
		{
			if (pData->download)
			{
				wxString filename = pData->remotePath.FormatFilename(pData->remoteFile);
				LogMessage(Status, _("Skipping download of %s"), filename.c_str());
			}
			else
			{
				LogMessage(Status, _("Skipping upload of %s"), pData->localFile.c_str());
			}
			ResetOperation(FZ_REPLY_OK);
		}
		break;
	case CFileExistsNotification::overwriteSizeOrNewer:
		if (!pFileExistsNotification->localTime.IsValid() || !pFileExistsNotification->remoteTime.IsValid())
			SendNextCommand();
		/* First compare flags both size known but different, one size known and the other not (obviously they are different).
		Second compare flags the remaining case in which we need to send command : both size unknown */
		else if ((pFileExistsNotification->localSize != pFileExistsNotification->remoteSize) || (pFileExistsNotification->localSize == -1))
			SendNextCommand();
		else if (pFileExistsNotification->download && pFileExistsNotification->localTime.IsEarlierThan(pFileExistsNotification->remoteTime))
			SendNextCommand();
		else if (!pFileExistsNotification->download && pFileExistsNotification->localTime.IsLaterThan(pFileExistsNotification->remoteTime))
			SendNextCommand();
		else
		{
			if (pData->download)
			{
				wxString filename = pData->remotePath.FormatFilename(pData->remoteFile);
				LogMessage(Status, _("Skipping download of %s"), filename.c_str());
			}
			else
			{
				LogMessage(Status, _("Skipping upload of %s"), pData->localFile.c_str());
			}
			ResetOperation(FZ_REPLY_OK);
		}
		break;
	case CFileExistsNotification::resume:
		if (pData->download && pData->localFileSize != -1)
			pData->resume = true;
		else if (!pData->download && pData->remoteFileSize != -1)
			pData->resume = true;
		SendNextCommand();
		break;
	case CFileExistsNotification::rename:
		if (pData->download)
		{
			wxFileName fn = pData->localFile;
			fn.SetFullName(pFileExistsNotification->newName);
			pData->localFile = fn.GetFullPath();

			wxLongLong size;
			bool isLink;
			if (CLocalFileSystem::GetFileInfo(pData->localFile, isLink, &size, 0, 0) == CLocalFileSystem::file)
				pData->localFileSize = size.GetValue();
			else
				pData->localFileSize = -1;

			if (CheckOverwriteFile() == FZ_REPLY_OK)
				SendNextCommand();
		}
		else
		{
			pData->remoteFile = pFileExistsNotification->newName;

			CDirectoryListing listing;
			CDirectoryCache cache;
			bool is_outdated = false;
			bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, true, is_outdated);
			if (found)
			{
				bool differentCase = false;
				bool found = false;
				for (unsigned int i = 0; i < listing.GetCount(); i++)
				{
					if (!listing[i].name.CmpNoCase(pData->remoteFile))
					{
						if (listing[i].name != pData->remoteFile)
							differentCase = true;
						else
						{
							wxLongLong size = listing[i].size;
							pData->remoteFileSize = size.GetLo() + ((wxFileOffset)size.GetHi() << 32);
							found = true;
							break;
						}
					}
				}
				if (found)
				{
					if (CheckOverwriteFile() != FZ_REPLY_OK)
						break;
				}
			}
			SendNextCommand();
		}
		break;
	case CFileExistsNotification::skip:
		if (pData->download)
		{
			wxString filename = pData->remotePath.FormatFilename(pData->remoteFile);
			LogMessage(Status, _("Skipping download of %s"), filename.c_str());
		}
		else
		{
			LogMessage(Status, _("Skipping upload of %s"), pData->localFile.c_str());
		}
		ResetOperation(FZ_REPLY_OK);
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown file exists action: %d"), pFileExistsNotification->overwriteAction);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return false;
	}

	return true;
}
