#include "FileZilla.h"
#include "ftpcontrolsocket.h"
#include "transfersocket.h"
#include "directorylistingparser.h"
#include "directorycache.h"

#include <wx/filefn.h>
#include <wx/file.h>

CFileTransferOpData::CFileTransferOpData()
{
	opId = cmd_transfer;

	pFile = 0;
	resume = false;
	tryAbsolutePath = false;
	bTriedPasv = bTriedActive = false;
	fileSize = -1;
	transferEndReason = 0;
}

CFileTransferOpData::~CFileTransferOpData()
{
	delete pFile;
}

enum filetransferStates
{
	filetransfer_init = 0,
	filetransfer_waitcwd,
	filetransfer_waitlist,
	filetransfer_size,
	filetransfer_mdtm,
	filetransfer_type,
	filetransfer_port_pasv,
	filetransfer_rest0,
	filetransfer_rest,
	filetransfer_transfer,
	filetransfer_waitfinish,
	filetransfer_waittransferpre,
	filetransfer_waittransfer,
	filetransfer_waitsocket
};

class CLogonOpData : public COpData
{
public:
	CLogonOpData()
	{
		logonSequencePos = 0;
		logonType = 0;
		nCommand = 0;

		opId = cmd_connect;

		waitChallenge = false;
		gotPassword = false;
		waitForAsyncRequest = false;

	}

	virtual ~CLogonOpData()
	{
	}

	int logonSequencePos;
	int logonType;
	int nCommand; // Next command to send in the current logon sequence

	wxString challenge; // Used for interactive logons
	bool waitChallenge;
	bool waitForAsyncRequest;
	bool gotPassword;
};

CFtpControlSocket::CFtpControlSocket(CFileZillaEnginePrivate *pEngine) : CControlSocket(pEngine)
{
	m_ReceiveBuffer.Alloc(2000);

	m_pTransferSocket = 0;
	m_sentRestartOffset = false;
}

CFtpControlSocket::~CFtpControlSocket()
{
}

#define BUFFERSIZE 4096
void CFtpControlSocket::OnReceive(wxSocketEvent &event)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::OnReceive()"));

	char *buffer = new char[BUFFERSIZE];
	Read(buffer, BUFFERSIZE);
	if (Error())
	{
		if (LastError() != wxSOCKET_WOULDBLOCK)
		{
			LogMessage(::Error, _("Disconnected from server"));
			DoClose();
		}
		delete [] buffer;
		return;
	}

	int numread = LastCount();
	if (!numread)
	{
		delete [] buffer;
		return;
	}

	m_pEngine->SetActive(true);

	for (int i = 0; i < numread; i++)
	{
		if (buffer[i] == '\r' ||
			buffer[i] == '\n' ||
			buffer[i] == 0)
		{
			if (m_ReceiveBuffer == _T(""))
				continue;

			LogMessage(Response, m_ReceiveBuffer);

			if (GetCurrentCommandId() == cmd_connect && m_pCurOpData && reinterpret_cast<CLogonOpData *>(m_pCurOpData)->waitChallenge)
			{
				wxString& challenge = reinterpret_cast<CLogonOpData *>(m_pCurOpData)->challenge;
				if (challenge != _T(""))
#ifdef __WXMSW__
					challenge += _T("\r\n");
#else
					challenge += _T("\n");
#endif
				challenge += m_ReceiveBuffer;
			}
			//Check for multi-line responses
			if (m_ReceiveBuffer.Len() > 3)
			{
				if (m_MultilineResponseCode != _T(""))
				{
					if (m_ReceiveBuffer.Left(4) == m_MultilineResponseCode)
					{
						// end of multi-line found
						m_MultilineResponseCode.Clear();
						ParseResponse();
					}
				}
				// start of new multi-line
				else if (m_ReceiveBuffer.GetChar(3) == '-')
				{
					// DDD<SP> is the end of a multi-line response
					m_MultilineResponseCode = m_ReceiveBuffer.Left(3) + _T(" ");
				}
				else
				{
					ParseResponse();
				}
			}

			m_ReceiveBuffer.clear();
		}
		else
		{
			//The command may only be 2000 chars long. This ensures that a malicious user can't
			//send extremely large commands to fill the memory of the server
			if (m_ReceiveBuffer.Len()<2000)
				m_ReceiveBuffer += buffer[i];
		}
	}

	delete [] buffer;
}

void CFtpControlSocket::OnConnect(wxSocketEvent &event)
{
	LogMessage(Status, _("Connection established, waiting for welcome message..."));
	Logon();
}

void CFtpControlSocket::ParseResponse()
{
	enum Command commandId = GetCurrentCommandId();
	switch (commandId)
	{
	case cmd_connect:
		LogonParseResponse();
		break;
	case cmd_list:
		ListParseResponse();
		break;
	case cmd_private:
		ChangeDirParseResponse();
		break;
	case cmd_transfer:
		FileTransferParseResponse();
		break;
	case cmd_raw:
		RawCommand();
		break;
	case cmd_delete:
		Delete();
		break;
	case cmd_removedir:
		RemoveDir();
		break;
	case cmd_mkdir:
		MkdirParseResponse();
		break;
	case cmd_rename:
		RenameParseResponse();
		break;
	case cmd_chmod:
		ChmodParseResponse();
		break;
	default:
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}
}

int CFtpControlSocket::Logon()
{
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("deleting nonzero pData"));
		delete m_pCurOpData;
	}
	m_pCurOpData = new CLogonOpData;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::LogonParseResponse()
{
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("LogonParseResponse without m_pCurOpData called"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_INTERNALERROR;
	}

	CLogonOpData *pData = reinterpret_cast<CLogonOpData *>(m_pCurOpData);

	const int LO = -2, ER = -1;
	const int NUMLOGIN = 9; // currently supports 9 different login sequences
	int logonseq[NUMLOGIN][20] = {
		// this array stores all of the logon sequences for the various firewalls
		// in blocks of 3 nums. 1st num is command to send, 2nd num is next point in logon sequence array
		// if 200 series response is rec'd from server as the result of the command, 3rd num is next
		// point in logon sequence if 300 series rec'd
		{0,LO,3, 1,LO,ER}, // no firewall
		{3,6,3,  4,6,ER, 5,9,9, 0,LO,12, 1,LO,ER}, // SITE hostname
		{3,6,3,  4,6,ER, 6,LO,9, 1,LO,ER}, // USER after logon
		{7,3,3,  0,LO,6, 1,LO,ER}, //proxy OPEN
		{3,6,3,  4,6,ER, 0,LO,9, 1,LO,ER}, // Transparent
		{6,LO,3, 1,LO,ER}, // USER remoteID@remotehost
		{8,6,3,  4,6,ER, 0,LO,9, 1,LO,ER}, //USER fireID@remotehost
		{9,ER,3, 1,LO,6, 2,LO,ER}, //USER remoteID@remotehost fireID
		{10,LO,3,11,LO,6,2,LO,ER} // USER remoteID@fireID@remotehost
	};

	int code = GetReplyCode();
	if (code != 2 && code != 3)
	{
		DoClose(FZ_REPLY_DISCONNECTED);
		return FZ_REPLY_DISCONNECTED;
	}
	if (!pData->opState)
	{
		pData->opState = 1;
		pData->nCommand = logonseq[pData->logonType][0];
	}
	else if (pData->opState == 1)
	{
		pData->waitChallenge = false;
		pData->logonSequencePos = logonseq[pData->logonType][pData->logonSequencePos + code - 1];

		switch(pData->logonSequencePos)
		{
		case ER: // ER means summat has gone wrong
			DoClose();
			return FZ_REPLY_ERROR;
		case LO: //LO means we are logged on
			pData->opState = 2;
		}

		pData->nCommand = logonseq[pData->logonType][pData->logonSequencePos];
	}
	else if (pData->opState == 2)
	{
		if (code == 2 && m_ReceiveBuffer.Length() > 7 && m_ReceiveBuffer.Mid(3, 4) == _T(" MVS"))
		{
			if (m_pCurrentServer->GetType() == DEFAULT)
				m_pCurrentServer->SetType(MVS);
		}

		LogMessage(Status, _("Connected"));
		ResetOperation(FZ_REPLY_OK);
		return true;
	}

	return LogonSend();
}

int CFtpControlSocket::LogonSend()
{
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("LogonParseResponse without m_pCurOpData called"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_INTERNALERROR;
	}

	CLogonOpData *pData = reinterpret_cast<CLogonOpData *>(m_pCurOpData);

	bool res;
	switch (pData->opState)
	{
	case 2:
		res = Send(_T("SYST"));
		break;
	case 1:
		switch (pData->nCommand)
		{
		case 0:
			res = Send(_T("USER ") + m_pCurrentServer->GetUser());

			if (m_pCurrentServer->GetLogonType() == INTERACTIVE)
			{
				pData->waitChallenge = true;
				pData->challenge = _T("");
			}
			break;
		case 1:
			if (pData->challenge != _T(""))
			{
				CInteractiveLoginNotification *pNotification = new CInteractiveLoginNotification;
				pNotification->server = *m_pCurrentServer;
				pNotification->challenge = pData->challenge;
				pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
				pData->waitForAsyncRequest = true;
				pData->challenge = _T("");
				m_pEngine->AddNotification(pNotification);

				return FZ_REPLY_WOULDBLOCK;
			}

			res = Send(_T("PASS ") + m_pCurrentServer->GetPass());
			break;
		default:
			ResetOperation(FZ_REPLY_INTERNALERROR);
			res = false;
			break;
		}
		break;
	default:
		return FZ_REPLY_ERROR;
	}

	if (!res)
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::GetReplyCode() const
{
	if (m_ReceiveBuffer == _T(""))
		return 0;

	if (m_ReceiveBuffer[0] < '0' || m_ReceiveBuffer[0] > '9')
		return 0;

	return m_ReceiveBuffer[0] - '0';
}

bool CFtpControlSocket::Send(wxString str)
{
	LogMessage(Command, str);
	str += _T("\r\n");
	wxCharBuffer buffer = wxCSConv(_T("ISO8859-1")).cWX2MB(str);
	unsigned int len = (unsigned int)strlen(buffer);
	return CControlSocket::Send(buffer, len);
}

class CListOpData : public COpData
{
public:
	CListOpData()
	{
		opId = cmd_list;

		bTriedPasv = bTriedActive = false;
		transferEndReason = 0;
	}

	virtual ~CListOpData()
	{
	}

	bool bPasv;
	bool bTriedPasv;
	bool bTriedActive;

	int port;
	wxString host;

	CServerPath path;
	wxString subDir;
	bool refresh;

	int transferEndReason;
};

enum listStates
{
	list_init = 0,
	list_waitcwd,
	list_type,
	list_port_pasv,
	list_list,
	list_waitfinish,
	list_waitlistpre,
	list_waitlist,
	list_waitsocket
};

int CFtpControlSocket::List(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/, bool refresh /*=false*/)
{
	LogMessage(Status, _("Retrieving directory listing..."));

	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("List called from other command"));
	}
	CListOpData *pData = new CListOpData;
	pData->pNextOpData = m_pCurOpData;
	m_pCurOpData = pData;

	switch (m_pCurrentServer->GetPasvMode())
	{
	case MODE_PASSIVE:
		pData->bPasv = true;
		break;
	case MODE_ACTIVE:
		pData->bPasv = false;
		break;
	default:
		pData->bPasv = m_pEngine->GetOptions()->GetOptionVal(OPTION_USEPASV) != 0;
		break;
	}
	pData->opState = list_waitcwd;

	if (path.GetType() == DEFAULT)
		path.SetType(m_pCurrentServer->GetType());
	pData->path = path;
	pData->subDir = subDir;
	pData->refresh = refresh;

	int res = ChangeDir(path, subDir);
	if (res != FZ_REPLY_OK)
		return res;

	pData->opState = list_type;

	return ListSend();
}

int CFtpControlSocket::ListSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::ListSend(%d)"), prevResult);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CListOpData *pData = static_cast<CListOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	if (pData->opState == list_waitcwd)
	{
		if (prevResult != FZ_REPLY_OK)
		{
			ResetOperation(prevResult);
			return FZ_REPLY_ERROR;
		}

		if (!pData->refresh)
		{
			// Do a cache lookup now that we know the correct directory
			CDirectoryListing *pListing = new CDirectoryListing;
			CDirectoryCache cache;
			bool found = cache.Lookup(*pListing, *m_pCurrentServer, m_CurrentPath);
			if (found && !pListing->m_hasUnsureEntries)
			{
				if (!pData->path.IsEmpty() && pData->subDir != _T(""))
					cache.AddParent(*m_pCurrentServer, m_CurrentPath, pData->path, pData->subDir);
				SendDirectoryListing(pListing);
				ResetOperation(FZ_REPLY_OK);
				return FZ_REPLY_OK;
			}
			else
				delete pListing;
		}

		pData->opState = list_type;
	}

	wxString cmd;
	switch (pData->opState)
	{
	case list_type:
		cmd = _T("TYPE A");
		break;
	case list_port_pasv:
		delete m_pTransferSocket;
		m_pTransferSocket = new CTransferSocket(m_pEngine, this, ::list);
		if (pData->bPasv)
		{
			pData->bTriedPasv = true;
			cmd = _T("PASV");
		}
		else
		{
			wxString port = m_pTransferSocket->SetupActiveTransfer();
			if (port == _T(""))
			{
				if (pData->bTriedPasv)
				{
					LogMessage(::Error, _("Failed to create listening socket for active mode transfer, trying passive mode"));
					ResetOperation(FZ_REPLY_ERROR);
					return FZ_REPLY_ERROR;
				}
				pData->bTriedActive = true;
				pData->bTriedPasv = true;
				pData->bPasv = true;
				cmd = _T("PASV");
			}
			else
				cmd = _T("PORT " + port);
		}
		break;
	case list_list:
		cmd = _T("LIST");

		if (pData->bPasv)
		{
			if (!m_pTransferSocket->SetupPassiveTransfer(pData->host, pData->port))
			{
				LogMessage(::Error, _("Could not establish connection to server"));
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}

		InitTransferStatus(-1, 0);
		m_pTransferSocket->SetActive();

		break;
	case list_waitfinish:
	case list_waitlist:
	case list_waitsocket:
	case list_waitlistpre:
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("invalid opstate"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}
	if (cmd != _T(""))
		if (!Send(cmd))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::ListParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::ListParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CListOpData *pData = static_cast<CListOpData *>(m_pCurOpData);
	if (pData->opState == list_init)
		return FZ_REPLY_ERROR;

	int code = GetReplyCode();

	LogMessage(Debug_Debug, _T("  code = %d"), code);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	bool error = false;
	switch (pData->opState)
	{
	case list_type:
		// Don't check error code here, we can live perfectly without it
		pData->opState = list_port_pasv;
		break;
	case list_port_pasv:
		if (code != 2 && code != 3)
		{
			if (pData->bTriedPasv)
				if (pData->bTriedActive)
					error = true;
				else
					pData->bPasv = false;
			else
				pData->bPasv = true;
			break;
		}
		if (pData->bPasv)
		{
			int i, j;
			i = m_ReceiveBuffer.Find(_T("("));
			j = m_ReceiveBuffer.Find(_T(")"));
			if (i == -1 || j == -1)
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}

			wxString temp = m_ReceiveBuffer.Mid(i+1,(j-i)-1);
			i = temp.Find(',', true);
			long number;
			if (i == -1 || !temp.Mid(i + 1).ToLong(&number))
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}
			pData->port = number; //get ls byte of server socket
			temp = temp.Left(i);
			i = temp.Find(',', true);
			if (i == -1 || !temp.Mid(i + 1).ToLong(&number))
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}
			pData->port += 256 * number; //add ms byte of server socket
			pData->host = temp.Left(i);
			pData->host.Replace(_T(","), _T("."));
		}
		pData->opState = list_list;
		break;
	case list_list:
		if (!m_ReceiveBuffer.CmpNoCase(_T("550 No members found.")) && m_pCurrentServer->GetType() == MVS)
		{
			CDirectoryListing *pListing = new CDirectoryListing();
			pListing->path = m_CurrentPath;

			CDirectoryCache cache;
			cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);

			SendDirectoryListing(pListing);
			m_pEngine->ResendModifiedListings();

			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else if (code != 1)
			error = true;
		else
			pData->opState = list_waitfinish;
		break;
	case list_waitlistpre:
		if (code != 1)
			error = true;
		else
			pData->opState = list_waitlist;
		break;
	case list_waitfinish:
		if (code != 2 && code != 3)
			error = true;
		else
			pData->opState = list_waitsocket;
		break;
	case list_waitlist:
		if (code != 2 && code != 3)
			error = true;
		else
		{
			if (pData->transferEndReason)
			{
				error = true;
				break;
			}

			CDirectoryListing *pListing = m_pTransferSocket->m_pDirectoryListingParser->Parse(m_CurrentPath);

			CDirectoryCache cache;
			cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);

			SendDirectoryListing(pListing);
			m_pEngine->ResendModifiedListings();

			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
		error = true;
	}
	if (error)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return ListSend();
}

bool CFtpControlSocket::ParsePwdReply(wxString reply)
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

	m_CurrentPath.SetType(m_pCurrentServer->GetType());
	if (!m_CurrentPath.SetPath(reply))
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Can't parse path"));
		return false;
	}

	return true;
}

int CFtpControlSocket::ResetOperation(int nErrorCode)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::ResetOperation(%d)"), nErrorCode);

	delete m_pTransferSocket;
	m_pTransferSocket = 0;

	if (m_pCurOpData && m_pCurOpData->pNextOpData)
	{
		COpData *pNext = m_pCurOpData->pNextOpData;
		m_pCurOpData->pNextOpData = 0;
		delete m_pCurOpData;
		m_pCurOpData = pNext;
		return SendNextCommand(nErrorCode);
	}

	if (m_pCurOpData && m_pCurOpData->opId == cmd_transfer)
	{
		CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
		if (!pData->download && pData->opState >= filetransfer_transfer)
		{
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->remotePath, pData->remoteFile, CDirectoryCache::file, (nErrorCode == FZ_REPLY_OK) ? pData->fileSize : -1);

			m_pEngine->ResendModifiedListings();
		}
	}

	return CControlSocket::ResetOperation(nErrorCode);
}

int CFtpControlSocket::SendNextCommand(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::SendNextCommand(%d)"), prevResult);
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("SendNextCommand called without active operation"));
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	if (m_pCurOpData->waitForAsyncRequest)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Waiting for async request, ignoring SendNextCommand"));
		return FZ_REPLY_WOULDBLOCK;
	}

	switch (m_pCurOpData->opId)
	{
	case cmd_list:
		return ListSend(prevResult);
	case cmd_connect:
		return LogonSend();
	case cmd_private:
		return ChangeDirSend();
	case cmd_transfer:
		return FileTransferSend(prevResult);
	case cmd_mkdir:
		return MkdirSend();
	case cmd_rename:
		return RenameSend(prevResult);
	case cmd_chmod:
		return ChmodSend(prevResult);
	default:
		LogMessage(::Debug_Warning, __TFILE__, __LINE__, _T("Unknown opID (%d) in SendNextCommand"), m_pCurOpData->opId);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}

	return FZ_REPLY_ERROR;
}

class CChangeDirOpData : public COpData
{
public:
	CChangeDirOpData()
	{
		opId = cmd_private;
		triedMkd = false;
	}

	virtual ~CChangeDirOpData()
	{
	}

	CServerPath path;
	wxString subDir;
	bool triedMkd;
};

enum cwdStates
{
	cwd_init = 0,
	cwd_pwd,
	cwd_cwd,
	cwd_pwd_cwd,
	cwd_cwd_subdir,
	cwd_pwd_subdir
};

int CFtpControlSocket::ChangeDir(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/)
{
	enum cwdStates state = cwd_init;

	if (path.GetType() == DEFAULT)
		path.SetType(m_pCurrentServer->GetType());

	if (path.IsEmpty())
	{
		if (m_CurrentPath.IsEmpty())
			state = cwd_pwd;
		else
			return FZ_REPLY_OK;
	}
	else
	{
		if (m_CurrentPath != path)
			state = cwd_cwd;
		else
		{
			if (subDir == _T(""))
				return FZ_REPLY_OK;
			else
				state = cwd_cwd_subdir;
		}
	}

	CChangeDirOpData *pData = new CChangeDirOpData;
	pData->pNextOpData = m_pCurOpData;
	pData->opState = state;
	pData->path = path;
	pData->subDir = subDir;

	m_pCurOpData = pData;

	return ChangeDirSend();
}

int CFtpControlSocket::ChangeDirParseResponse()
{
	if (!m_pCurOpData)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
	CChangeDirOpData *pData = static_cast<CChangeDirOpData *>(m_pCurOpData);

	int code = GetReplyCode();
	bool error = false;
	switch (pData->opState)
	{
	case cwd_pwd:
		if (code != 2 && code != 3)
			error = true;
		else if (ParsePwdReply(m_ReceiveBuffer))
		{
			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else
			error = true;
		break;
	case cwd_cwd:
		if (code != 2 && code != 3)
		{
			// Create remote directory if part of a file upload
			if (pData->pNextOpData && pData->pNextOpData->opId == cmd_transfer && 
				!static_cast<CFileTransferOpData *>(pData->pNextOpData)->download && !pData->triedMkd)
			{
				pData->triedMkd = true;
				int res = Mkdir(pData->path, pData->path);
				if (res != FZ_REPLY_OK)
					return res;
			}
			else
				error = true;
		}
		else
			pData->opState = cwd_pwd_cwd;
		break;
	case cwd_pwd_cwd:
		if (code != 2 && code != 3)
			error = true;
		else if (ParsePwdReply(m_ReceiveBuffer))
			if (pData->subDir == _T(""))
			{
				ResetOperation(FZ_REPLY_OK);
				return FZ_REPLY_OK;
			}
			else
				pData->opState = cwd_cwd_subdir;
		else
			error = true;
		break;
	case cwd_cwd_subdir:
		if (code != 2 && code != 3)
			error = true;
		else
			pData->opState = cwd_pwd_subdir;
		break;
	case cwd_pwd_subdir:
		if (code != 2 && code != 3)
			error = true;
		else if (ParsePwdReply(m_ReceiveBuffer))
		{
			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else
			error = true;
		break;
	}

	if (error)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return ChangeDirSend();
}

int CFtpControlSocket::ChangeDirSend()
{
	if (!m_pCurOpData)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
	CChangeDirOpData *pData = static_cast<CChangeDirOpData *>(m_pCurOpData);

	wxString cmd;
	switch (pData->opState)
	{
	case cwd_pwd:
	case cwd_pwd_cwd:
	case cwd_pwd_subdir:
		cmd = _T("PWD");
		cmd = _T("PWD");
		cmd = _T("PWD");
		break;
	case cwd_cwd:
		cmd = _T("CWD ") + pData->path.GetPath();
		m_CurrentPath.Clear();
		break;
	case cwd_cwd_subdir:
		if (pData->subDir == _T(""))
		{
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		else if (pData->subDir == _T(".."))
			cmd = _T("CDUP");
		else
			cmd = _T("CWD ") + pData->subDir;
		m_CurrentPath.Clear();
		break;
	}

	if (cmd != _T(""))
		if (!Send(cmd))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::FileTransfer(const wxString localFile, const CServerPath &remotePath,
									const wxString &remoteFile, bool download,
									const CFileTransferCommand::t_transferSettings& transferSettings)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::FileTransfer()"));

	if (download)
	{
		wxString filename = remotePath.FormatFilename(remoteFile);
		LogMessage(Status, _("Starting download of %s"), filename.c_str());
	}
	else
	{
		LogMessage(Status, _("Starting upload of %s"), localFile.c_str());
	}
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("deleting nonzero pData"));
		delete m_pCurOpData;
	}

	CFileTransferOpData *pData = new CFileTransferOpData;
	m_pCurOpData = pData;

	pData->localFile = localFile;
	pData->remotePath = remotePath;
	pData->remoteFile = remoteFile;
	pData->download = download;
	pData->transferSettings = transferSettings;

	switch (m_pCurrentServer->GetPasvMode())
	{
	case MODE_PASSIVE:
		pData->bPasv = true;
		break;
	case MODE_ACTIVE:
		pData->bPasv = false;
		break;
	default:
		pData->bPasv = m_pEngine->GetOptions()->GetOptionVal(OPTION_USEPASV) != 0;
		break;
	}

	pData->opState = filetransfer_waitcwd;

	if (pData->remotePath.GetType() == DEFAULT)
		pData->remotePath.SetType(m_pCurrentServer->GetType());

	int res = ChangeDir(pData->remotePath);
	if (res != FZ_REPLY_OK)
		return res;

	pData->opState = filetransfer_waitlist;

	CDirectoryListing listing;
	CDirectoryCache cache;
	bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
	bool shouldList = false;
	if (!found)
		shouldList = true;
	else
	{
		bool differentCase = false;

		for (unsigned int i = 0; i < listing.m_entryCount; i++)
		{
			if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
			{
				if (listing.m_pEntries[i].unsure)
				{
					shouldList = true;
					break;
				}
				if (listing.m_pEntries[i].name != pData->remoteFile)
					differentCase = true;
				else
				{
					differentCase = false;
					break;
				}
			}
		}
		if (!shouldList)
		{
			if (differentCase)
				pData->opState = filetransfer_size;
			else
			{
				pData->opState = filetransfer_type;
				res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
		}
	}
	if (shouldList)
	{
		res = List();
		if (res != FZ_REPLY_OK)
			return res;

		pData->opState = filetransfer_type;

		res = CheckOverwriteFile();
		if (res != FZ_REPLY_OK)
			return res;
	}

	return FileTransferSend();
}

int CFtpControlSocket::FileTransferParseResponse()
{
	LogMessage(Debug_Verbose, _T("FileTransferParseResponse()"));

	if (!m_pCurOpData)
		return FZ_REPLY_ERROR;

	CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
	if (pData->opState == list_init)
		return FZ_REPLY_ERROR;

	int code = GetReplyCode();
	bool error = false;
	switch (pData->opState)
	{
	case filetransfer_size:
		pData->opState = filetransfer_mdtm;
		if (m_ReceiveBuffer.Left(4) == _T("213 ") && m_ReceiveBuffer.Length() > 4)
		{
			wxString str = m_ReceiveBuffer.Mid(4);
			wxFileOffset size = 0;
			const wxChar *buf = str.c_str();
			while (*buf)
			{
				if (*buf < '0' || *buf > '9')
					break;

				size *= 10;
				size += *buf - '0';
				buf++;
			}
			pData->fileSize = size;
		}
		else if (code == 2 || code == 3)
			LogMessage(Debug_Info, _T("Invalid SIZE reply"));
		break;
	case filetransfer_mdtm:
		pData->opState = filetransfer_type;
		if (m_ReceiveBuffer.Left(4) == _T("213 ") && m_ReceiveBuffer.Length() > 16)
		{
			wxDateTime date;
			const wxChar *res = date.ParseFormat(m_ReceiveBuffer.Mid(4), _T("%Y%m%d%H%M"));
			if (res && date.IsValid())
				pData->fileTime = date;
		}

		{
			int res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
		}

		break;
	case filetransfer_type:
		if (code == 2 || code == 3)
			pData->opState = filetransfer_port_pasv;
		else
			error = true;
		break;
	case filetransfer_port_pasv:
		if (code != 2 && code != 3)
		{
			if (pData->bTriedPasv)
				if (pData->bTriedActive)
					error = true;
				else
					pData->bPasv = false;
			else
				pData->bPasv = true;
			break;
		}

		if (pData->bPasv)
		{
			int i, j;
			i = m_ReceiveBuffer.Find(_T("("));
			j = m_ReceiveBuffer.Find(_T(")"));
			if (i == -1 || j == -1)
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}

			wxString temp = m_ReceiveBuffer.Mid(i+1,(j-i)-1);
			i = temp.Find(',', true);
			long number;
			if (i == -1 || !temp.Mid(i + 1).ToLong(&number))
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}
			pData->port = number; //get ls byte of server socket
			temp = temp.Left(i);
			i = temp.Find(',', true);
			if (i == -1 || !temp.Mid(i + 1).ToLong(&number))
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}
			pData->port += 256 * number; //add ms byte of server socket
			pData->host = temp.Left(i);
			pData->host.Replace(_T(","), _T("."));
		}

		if (pData->download)
		{
			if (pData->resume)
				pData->opState = filetransfer_rest;
			else if (m_sentRestartOffset)
				pData->opState = filetransfer_rest0;
			else
				pData->opState = filetransfer_transfer;
		}
		else
			pData->opState = filetransfer_transfer;

		pData->pFile = new wxFile;
		if (pData->download)
		{
			// Be quiet
			wxLogNull nullLog;

			// Create local directory
			wxFileName fn(pData->localFile);
			wxFileName::Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);

			wxFileOffset startOffset;
			if (pData->resume)
			{
				if (!pData->pFile->Open(pData->localFile, wxFile::write_append))
				{
					LogMessage(::Error, _("Failed to open \"%s\" for appending / writing"), pData->localFile.c_str());
					error = true;
					break;
				}

				startOffset = pData->pFile->Length();
			}
			else
			{
				if (!pData->pFile->Open(pData->localFile, wxFile::write))
				{
					LogMessage(::Error, _("Failed to open \"%s\" for writing"), pData->localFile.c_str());
					error = true;
					break;
				}

				startOffset = 0;
			}

			InitTransferStatus(pData->fileSize, startOffset);
		}
		else
		{
			if (!pData->pFile->Open(pData->localFile, wxFile::read))
			{
				LogMessage(::Error, _("Failed to open \"%s\" for reading"), pData->localFile.c_str());
				error = true;
				break;
			}

			wxFileOffset startOffset;
			if (pData->resume)
			{
				if (pData->fileSize > 0)
				{
					startOffset = pData->fileSize;

					// Assume native 64 bit type exists
					if (pData->pFile->Seek(startOffset, wxFromStart) == wxInvalidOffset)
					{
						LogMessage(::Error, _("Could not seek to offset %s within file"), wxLongLong(startOffset).ToString().c_str());
						error = true;
						break;
					}
				}
				else
					startOffset = 0;
			}
			else
				startOffset = 0;

			wxFileOffset len = pData->pFile->Length();
			InitTransferStatus(len, startOffset);
		}
		break;
	case filetransfer_rest0:
		pData->opState = filetransfer_transfer;
		break;
	case filetransfer_rest:
		if (code == 3 || code == 2)
		{
			if (pData->pFile->Seek(0, wxFromEnd) == wxInvalidOffset)
			{
				LogMessage(::Error, _("Could not seek to end of file"));
				error = true;
				break;
			}
			pData->opState = filetransfer_transfer;
		}
		else
			error = true;
		break;
	case filetransfer_transfer:
		if (code != 1)
			error = true;
		else
			pData->opState = filetransfer_waitfinish;
		break;
	case filetransfer_waittransferpre:
		if (code != 1)
			error = true;
		else
			pData->opState = filetransfer_waittransfer;
		break;
	case filetransfer_waitfinish:
		if (code != 2 && code != 3)
			error = true;
		else
			pData->opState = filetransfer_waitsocket;
		break;
	case filetransfer_waittransfer:
		if (code != 2 && code != 3)
			error = true;
		else
		{
			if (pData->transferEndReason)
				error = true;
			else
			{
				ResetOperation(FZ_REPLY_OK);
				return FZ_REPLY_OK;
			}
		}
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
		error = true;
		break;
	}

	if (error)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return FileTransferSend();
}

int CFtpControlSocket::FileTransferSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("FileTransferSend()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);

	if (pData->opState == filetransfer_waitcwd)
	{
		if (prevResult == FZ_REPLY_OK)
		{
			pData->opState = filetransfer_waitlist;

			CDirectoryListing listing;
			CDirectoryCache cache;
			bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
			bool shouldList = false;
			if (!found)
				shouldList = true;
			else
			{
				bool differentCase = false;

				for (unsigned int i = 0; i < listing.m_entryCount; i++)
				{
					if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
					{
						if (listing.m_pEntries[i].unsure)
						{
							shouldList = true;
							break;
						}
						if (listing.m_pEntries[i].name != pData->remoteFile)
							differentCase = true;
						else
						{
							differentCase = false;
							break;
						}
					}
				}
				if (!shouldList)
				{
					if (differentCase)
						pData->opState = filetransfer_size;
					else
					{
						pData->opState = filetransfer_type;
						int res = CheckOverwriteFile();
						if (res != FZ_REPLY_OK)
							return res;
					}
				}
			}
			if (shouldList)
			{
				int res = List();
				if (res != FZ_REPLY_OK)
					return res;

				pData->opState = filetransfer_type;

				res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
		}
		else if (prevResult == FZ_REPLY_ERROR)
		{
			pData->tryAbsolutePath = true;
			pData->opState = filetransfer_size;
		}
		else
		{
			ResetOperation(prevResult);
			return FZ_REPLY_ERROR;
		}
	}
	else if (pData->opState == filetransfer_waitlist)
	{
		if (prevResult == FZ_REPLY_OK)
		{
			CDirectoryListing listing;
			CDirectoryCache cache;
			bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
			if (!found)
				pData->opState = filetransfer_size;
			else
			{
				bool differentCase = false;

				for (unsigned int i = 0; i < listing.m_entryCount; i++)
				{
					if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
					{
						if (listing.m_pEntries[i].unsure)
						{
							differentCase = true;
							break;
						}
						if (listing.m_pEntries[i].name != pData->remoteFile)
							differentCase = true;
						else
						{
							differentCase = false;
							break;
						}
					}
				}
				if (differentCase)
					pData->opState = filetransfer_size;
				else
				{
					pData->opState = filetransfer_type;

					int res = CheckOverwriteFile();
					if (res != FZ_REPLY_OK)
						return res;
				}
			}
		}
		else if (prevResult == FZ_REPLY_ERROR)
		{
			pData->opState = filetransfer_size;
		}
		else
		{
			ResetOperation(prevResult);
			return FZ_REPLY_ERROR;
		}
	}

	wxString cmd;
	switch (pData->opState)
	{
	case filetransfer_size:
		cmd = _T("SIZE ");
		cmd += pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath);
		break;
	case filetransfer_mdtm:
		cmd = _T("MDTM ");
		cmd += pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath);
		break;
	case filetransfer_type:
		if (pData->transferSettings.binary)
			cmd = _T("TYPE I");
		else
			cmd = _T("TYPE A");
		break;
	case filetransfer_port_pasv:
		if (m_pTransferSocket)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("m_pTransferSocket != 0"));
			delete m_pTransferSocket;
		}
		m_pTransferSocket = new CTransferSocket(m_pEngine, this, pData->download ? download : upload);
		m_pTransferSocket->m_binaryMode = pData->transferSettings.binary;
		if (pData->bPasv)
		{
			pData->bTriedPasv = true;
			cmd = _T("PASV");
		}
		else
		{
			wxString port = m_pTransferSocket->SetupActiveTransfer();
			if (port == _T(""))
			{
				if (pData->bTriedPasv)
				{
					LogMessage(::Error, _("Failed to create listening socket for active mode transfer, trying passive mode"));
					ResetOperation(FZ_REPLY_ERROR);
					return FZ_REPLY_ERROR;
				}
				pData->bTriedActive = true;
				pData->bTriedPasv = true;
				pData->bPasv = true;
				cmd = _T("PASV");
			}
			else
				cmd = _T("PORT " + port);
		}
		break;
	case filetransfer_rest0:
		cmd = _T("REST 0");
		break;
	case filetransfer_rest:
		if (!pData->pFile)
		{
			LogMessage(::Debug_Warning, _T("Can't send REST command, can't get local file length since pData->pFile is null"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		else
		{
			wxFileOffset offset = pData->pFile->Length();
			cmd = _T("REST ") + ((wxLongLong)offset).ToString();
			m_sentRestartOffset = true;
		}
		break;
	case filetransfer_transfer:
		if (pData->download)
			cmd = _T("RETR ");
		else if (pData->resume)
			cmd = _T("APPE ");
		else
			cmd = _T("STOR ");
		cmd += pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath);

		if (pData->bPasv)
		{
			if (!m_pTransferSocket->SetupPassiveTransfer(pData->host, pData->port))
			{
				LogMessage(::Error, _("Could not establish passive connection to server"));
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}

		m_pTransferSocket->SetActive();
		break;
	}

	if (cmd != _T(""))
		if (!Send(cmd))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

void CFtpControlSocket::TransferEnd(int reason)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::TransferEnd(%d)"), reason);

	// If m_pTransferSocket is zero, the message was sent by the previous command.
	// We can safely ignore it.
	// It does not cause problems, since before creating the next transfer socket, other
	// messages which were added to the queue later than this one will be processed first.
	if (!m_pCurOpData || !m_pTransferSocket)
	{
		LogMessage(Debug_Verbose, _T("TransferEnd message from previous operation"), reason);
		return;
	}

	// Even is reason indicates a failure, don't reset operation
	// yet, wait for the reply to the LIST/RETR/... commands

	if (GetCurrentCommandId() == cmd_list)
	{
		CListOpData *pData = static_cast<CListOpData *>(m_pCurOpData);
		if (pData->opState < list_list || pData->opState == list_waitlist || pData->opState == list_waitlistpre)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
			ResetOperation(FZ_REPLY_ERROR);
			return;
		}

		pData->transferEndReason = reason;

		switch (pData->opState)
		{
		case list_list:
			pData->opState = list_waitlistpre;
			break;
		case list_waitfinish:
			pData->opState = list_waitlist;
			break;
		case list_waitsocket:
			{
				if (!reason)
				{
					CDirectoryListing *pListing = m_pTransferSocket->m_pDirectoryListingParser->Parse(m_CurrentPath);

					CDirectoryCache cache;
					cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);

					SendDirectoryListing(pListing);
					m_pEngine->ResendModifiedListings();

					ResetOperation(FZ_REPLY_OK);
				}
				else
					ResetOperation(FZ_REPLY_ERROR);
			}
			break;
		default:
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
			ResetOperation(FZ_REPLY_ERROR);
		}
	}
	else if (GetCurrentCommandId() == cmd_transfer)
	{
		CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
		if (pData->opState < filetransfer_transfer || pData->opState == filetransfer_waittransfer || pData->opState == filetransfer_waittransferpre)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
			ResetOperation(FZ_REPLY_ERROR);
			return;
		}

		pData->transferEndReason = reason;

		switch (pData->opState)
		{
		case filetransfer_transfer:
			pData->opState = filetransfer_waittransferpre;
			break;
		case filetransfer_waitfinish:
			pData->opState = filetransfer_waittransfer;
			break;
		case filetransfer_waitsocket:
			{
				if (reason)
					ResetOperation(FZ_REPLY_ERROR);
				else
					ResetOperation(FZ_REPLY_OK);
			}
			break;
		default:
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
		}
	}
}

int CFtpControlSocket::CheckOverwriteFile()
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
		if (!found && pData->fileSize == -1 && !pData->fileTime.IsValid())
			return FZ_REPLY_OK;
	}

	CFileExistsNotification *pNotification = new CFileExistsNotification;

	pNotification->download = pData->download;

	pNotification->localFile = pData->localFile;

	wxStructStat buf;
	int result;
	result = wxStat(pData->localFile, &buf);
	if (!result)
	{
		pNotification->localSize = buf.st_size;
		pNotification->localTime = wxDateTime(buf.st_mtime);
		if (!pNotification->localTime.IsValid())
			pNotification->localTime = wxDateTime(buf.st_ctime);
	}

	pNotification->remoteFile = pData->remoteFile;
	pNotification->remotePath = pData->remotePath;

	if (pData->fileSize != -1)
		pNotification->remoteSize = pData->fileSize;

	if (pData->fileTime.IsValid())
		pNotification->remoteTime = pData->fileTime;

	if (foundListing)
	{
		for (unsigned int i = 0; i < listing.m_entryCount; i++)
		{
			if (listing.m_pEntries[i].name == pData->remoteFile)
			{
				if (pData->fileSize == -1)
				{
					wxLongLong size = listing.m_pEntries[i].size;
					if (size >= 0)
					{
						pNotification->remoteSize = size;
						pData->fileSize = size.GetLo() + ((wxFileOffset)size.GetHi() << 32);
					}
				}
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

bool CFtpControlSocket::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	switch (pNotification->GetRequestID())
	{
	case reqId_fileexists:
		{
			if (!m_pCurOpData || m_pCurOpData->opId != cmd_transfer)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No or invalid operation in progress, ignoring request reply %f"), pNotification->GetRequestID());
				return false;
			}

			CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);

			if (!pData->waitForAsyncRequest)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Not waiting for request reply, ignoring request reply %d"), pNotification->GetRequestID());
				return false;
			}
			pData->waitForAsyncRequest = false;

			CFileExistsNotification *pFileExistsNotification = reinterpret_cast<CFileExistsNotification *>(pNotification);
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
			case CFileExistsNotification::resume:
				if (pData->download && pFileExistsNotification->localSize != -1)
					pData->resume = true;
				else if (!pData->download && pFileExistsNotification->remoteSize != -1)
					pData->resume = true;
				SendNextCommand();
				break;
			case CFileExistsNotification::rename:
				if (pData->download)
				{
					wxFileName fn = pData->localFile;
					fn.SetFullName(pFileExistsNotification->newName);
					pData->localFile = fn.GetFullPath();

					if (CheckOverwriteFile() == FZ_REPLY_OK)
						SendNextCommand();
				}
				else
				{
					pData->remoteFile = pFileExistsNotification->newName;

					CDirectoryListing listing;
					CDirectoryCache cache;
					bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
					if (!found)
					{
						pData->opState = filetransfer_size;
						SendNextCommand();
					}
					else
					{
						bool differentCase = false;
						bool found = false;
						for (unsigned int i = 0; i < listing.m_entryCount; i++)
						{
							if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
							{
								if (listing.m_pEntries[i].name != pData->remoteFile)
									differentCase = true;
								else
								{
									found = true;
									break;
								}
							}
						}
						if (found)
						{
							if (CheckOverwriteFile() == FZ_REPLY_OK)
								SendNextCommand();
						}
						else if (differentCase)
						{
							pData->opState = filetransfer_size;
							SendNextCommand();
						}
						else
							SendNextCommand();
					}

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
		}
		break;
	case reqId_interactiveLogin:
		{
			if (!m_pCurOpData || m_pCurOpData->opId != cmd_connect)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No or invalid operation in progress, ignoring request reply %d"), pNotification->GetRequestID());
				return false;
			}

			CLogonOpData* pData = static_cast<CLogonOpData*>(m_pCurOpData);

			if (!pData->waitForAsyncRequest)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Not waiting for request reply, ignoring request reply %d"), pNotification->GetRequestID());
				return false;
			}
			pData->waitForAsyncRequest = false;

			CInteractiveLoginNotification *pInteractiveLoginNotification = reinterpret_cast<CInteractiveLoginNotification *>(pNotification);
			if (!pInteractiveLoginNotification->passwordSet)
			{
				ResetOperation(FZ_REPLY_CANCELED);
				return false;
			}
			m_pCurrentServer->SetUser(m_pCurrentServer->GetUser(), pInteractiveLoginNotification->server.GetPass());
			pData->gotPassword = true;
			SendNextCommand();
		}
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown request %d"), pNotification->GetRequestID());
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return false;
	}

	return true;
}

int CFtpControlSocket::RawCommand(const wxString& command /*=_T("")*/)
{
	if (command != _T(""))
	{
		if (!Send(command))
			return FZ_REPLY_ERROR;

		return FZ_REPLY_WOULDBLOCK;
	}

	CDirectoryCache cache;
	cache.InvalidateServer(*m_pCurrentServer);

	int code = GetReplyCode();
	if (code == 2 || code == 3)
		return ResetOperation(FZ_REPLY_OK);
	else
		return ResetOperation(FZ_REPLY_ERROR);
}

int CFtpControlSocket::Delete(const CServerPath& path /*=CServerPath()*/, const wxString& file /*=_T("")*/)
{
	class CDeleteOpData : public COpData
	{
	public:
		CDeleteOpData() { opId = cmd_delete; }
		virtual ~CDeleteOpData() { }

		CServerPath path;
		wxString file;
	};

	if (!path.IsEmpty())
	{
		wxASSERT(!m_pCurOpData);
		CDeleteOpData *pData = new CDeleteOpData();
		m_pCurOpData = pData;
		pData->path = path;
		pData->file = file;

		wxString filename = path.FormatFilename(file);
		if (filename == _T(""))
		{
			LogMessage(::Error, wxString::Format(_T("Filename cannot be constructed for folder %s and filename %s"), path.GetPath().c_str(), file.c_str()));
			return FZ_REPLY_ERROR;
		}

		if (!Send(_T("DELE ") + filename))
			return FZ_REPLY_ERROR;

		return FZ_REPLY_WOULDBLOCK;
	}

	int code = GetReplyCode();
	if (code != 2 && code != 3)
		return ResetOperation(FZ_REPLY_ERROR);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CDeleteOpData *pData = static_cast<CDeleteOpData *>(m_pCurOpData);
	CDirectoryCache cache;
	cache.RemoveFile(*m_pCurrentServer, pData->path, pData->file);
	m_pEngine->ResendModifiedListings();

	return ResetOperation(FZ_REPLY_OK);
}

int CFtpControlSocket::RemoveDir(const CServerPath& path /*=CServerPath()*/, const wxString& subDir /*=_T("")*/)
{
	class CRemoveDirOpData : public COpData
	{
	public:
		CRemoveDirOpData() { opId = cmd_removedir; }
		virtual ~CRemoveDirOpData() { }

		CServerPath path;
		wxString subDir;
	};

	if (!path.IsEmpty())
	{
		wxASSERT(!m_pCurOpData);
		CRemoveDirOpData *pData = new CRemoveDirOpData();
		m_pCurOpData = pData;
		pData->path = path;
		pData->subDir = subDir;

		CServerPath path = pData->path;
		
		if (!path.AddSegment(subDir))
		{
			LogMessage(::Error, wxString::Format(_T("Path cannot be constructed for folder %s and subdir %s"), path.GetPath().c_str(), subDir.c_str()));
			return FZ_REPLY_ERROR;
		}

		if (!Send(_T("RMD ") + path.GetPath()))
			return FZ_REPLY_ERROR;

		return FZ_REPLY_WOULDBLOCK;
	}

	int code = GetReplyCode();
	if (code != 2 && code != 3)
		return ResetOperation(FZ_REPLY_ERROR);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CRemoveDirOpData *pData = static_cast<CRemoveDirOpData *>(m_pCurOpData);
	CDirectoryCache cache;
	cache.RemoveDir(*m_pCurrentServer, pData->path, pData->subDir);
	m_pEngine->ResendModifiedListings();

	return ResetOperation(FZ_REPLY_OK);
}

enum mkdStates
{
	mkd_init = 0,
	mkd_findparent,
	mkd_mkdsub,
	mkd_cwdsub,
	mkd_tryfull
};

class CMkdirOpData : public COpData
{
public:
	CMkdirOpData()
	{
		opId = cmd_mkdir;
	}

	virtual ~CMkdirOpData()
	{
	}

	CServerPath path;
	CServerPath currentPath;
	std::list<wxString> segments;
};

int CFtpControlSocket::Mkdir(const CServerPath& path, CServerPath start /*=CServerPath()*/)
{
	/* Directory creation works like this: First find a parent directory into
	 * which we can CWD, then create the subdirs one by one. If either part 
	 * fails, try MKD with the full path directly.
	 */

	if (!m_pCurOpData)
		LogMessage(Status, _("Creating directory '%s'..."), path.GetPath().c_str());

	CMkdirOpData *pData = new CMkdirOpData;
	pData->pNextOpData = m_pCurOpData;
	pData->opState = mkd_findparent;
	pData->path = path;

	if (!start.IsEmpty())
	{
		CServerPath curPath = path;
		if (start != curPath && !start.IsParentOf(curPath, false))
		{
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		while (curPath != start)
		{
			pData->segments.push_back(curPath.GetLastSegment());
			curPath = curPath.GetParent();
		}
		pData->currentPath = curPath.GetParent();
		pData->segments.push_back(curPath.GetLastSegment());
	}
	else
	{
		pData->currentPath = path.GetParent();
		pData->segments.push_back(path.GetLastSegment());
	}
	
	m_pCurOpData = pData;

	return MkdirSend();
}

int CFtpControlSocket::MkdirParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::MkdirParseResonse"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CMkdirOpData *pData = static_cast<CMkdirOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	int code = GetReplyCode();
	bool error = false;
	switch (pData->opState)
	{
	case mkd_findparent:
		if (code == 2 || code == 3)
		{
			m_CurrentPath = pData->currentPath;
			pData->opState = mkd_mkdsub;
		}
		else if (pData->currentPath.HasParent())
		{
			pData->segments.push_front(pData->currentPath.GetLastSegment());
			pData->currentPath = pData->currentPath.GetParent();
		}
		else
			pData->opState = mkd_tryfull;
		break;
	case mkd_mkdsub:
		if (code == 2 || code == 3)
		{
			if (pData->segments.empty())
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("  pData->segments is empty"));
				ResetOperation(FZ_REPLY_INTERNALERROR);
				return FZ_REPLY_ERROR;
			}
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->currentPath, pData->segments.front(), CDirectoryCache::dir);
			m_pEngine->ResendModifiedListings();

			pData->currentPath.AddSegment(pData->segments.front());
			pData->segments.pop_front();

			if (pData->segments.empty())
			{
				ResetOperation(FZ_REPLY_OK);
				return FZ_REPLY_OK;
			}
			else
				pData->opState = mkd_cwdsub;
		}
		else
			pData->opState = mkd_tryfull;
		break;
	case mkd_cwdsub:
		if (code == 2 || code == 3)
		{
			m_CurrentPath = pData->currentPath;
			pData->opState = mkd_mkdsub;
		}
		else
			pData->opState = mkd_tryfull;
		break;
	case mkd_tryfull:
		if (code != 2 && code != 3)
			error = true;
		else
		{
			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("unknown op state: %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (error)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return MkdirSend();
}

int CFtpControlSocket::MkdirSend()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::MkdirSend"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CMkdirOpData *pData = static_cast<CMkdirOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	bool res;
	switch (pData->opState)
	{
	case mkd_findparent:
	case mkd_cwdsub:
		m_CurrentPath.Clear();
		res = Send(_T("CWD ") + pData->currentPath.GetPath());
		break;
	case mkd_mkdsub:
		res = Send(_T("MKD ") + pData->segments.front());
		break;
	case mkd_tryfull:
		res = Send(_T("MKD ") + pData->path.GetPath());
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("unknown op state: %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!res)
		return FZ_REPLY_ERROR;
	
	return FZ_REPLY_WOULDBLOCK;
}

class CRenameOpData : public COpData
{
public:
	CRenameOpData(const CRenameCommand& command)
		: m_cmd(command)
	{
		opId = cmd_rename;
		m_useAbsolute = false;
	}

	virtual ~CRenameOpData() { }

	CRenameCommand m_cmd;
	bool m_useAbsolute;
};

enum renameStates
{
	rename_init = 0,
	rename_rnfrom,
	rename_rnto
};

int CFtpControlSocket::Rename(const CRenameCommand& command)
{
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData not empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	LogMessage(Status, _("Renaming '%s' to '%s'"), command.GetFromPath().FormatFilename(command.GetFromFile()).c_str(), command.GetToPath().FormatFilename(command.GetToFile()).c_str());

	CRenameOpData *pData = new CRenameOpData(command);
	pData->opState = rename_rnfrom;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetFromPath());
	if (res != FZ_REPLY_OK)
		return res;
	
	return RenameSend();
}

int CFtpControlSocket::RenameParseResponse()
{
	CRenameOpData *pData = static_cast<CRenameOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	int code = GetReplyCode();
	if (code != 2 && code != 3)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	if (pData->opState == rename_rnfrom)
		pData->opState = rename_rnto;
	else
	{
		CDirectoryCache cache;
		cache.Rename(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile(), pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile());

		m_pEngine->ResendModifiedListings();

		ResetOperation(FZ_REPLY_OK);
		return FZ_REPLY_OK;
	}

	return RenameSend();
}

int CFtpControlSocket::RenameSend(int prevResult /*=FZ_REPLY_OK*/)
{
	CRenameOpData *pData = static_cast<CRenameOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (prevResult != FZ_REPLY_OK)
		pData->m_useAbsolute = true;

	bool res;
	switch (pData->opState)
	{
	case rename_rnfrom:
		res = Send(_T("RNFR ") + pData->m_cmd.GetFromPath().FormatFilename(pData->m_cmd.GetFromFile(), !pData->m_useAbsolute));
		break;
	case rename_rnto:
		res = Send(_T("RNTO ") + pData->m_cmd.GetToPath().FormatFilename(pData->m_cmd.GetToFile(), !pData->m_useAbsolute && pData->m_cmd.GetFromPath() == pData->m_cmd.GetToPath()));
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("unknown op state: %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!res)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}

class CChmodOpData : public COpData
{
public:
	CChmodOpData(const CChmodCommand& command)
		: m_cmd(command)
	{
		opId = cmd_chmod;
		m_useAbsolute = false;
	}

	virtual ~CChmodOpData() { }

	CChmodCommand m_cmd;
	bool m_useAbsolute;
};

enum chmodStates
{
	chmod_init = 0,
	chmod_chmod
};

int CFtpControlSocket::Chmod(const CChmodCommand& command)
{
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData not empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	LogMessage(Status, _("Set permissions of '%s' to '%s'"), command.GetPath().FormatFilename(command.GetFile()).c_str(), command.GetPermission().c_str());

	CChmodOpData *pData = new CChmodOpData(command);
	pData->opState = chmod_chmod;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetPath());
	if (res != FZ_REPLY_OK)
		return res;
	
	return ChmodSend();
}

int CFtpControlSocket::ChmodParseResponse()
{
	CChmodOpData *pData = static_cast<CChmodOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	int code = GetReplyCode();
	if (code != 2 && code != 3)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetPath(), pData->m_cmd.GetFile(), CDirectoryCache::unknown);

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CFtpControlSocket::ChmodSend(int prevResult /*=FZ_REPLY_OK*/)
{
	CChmodOpData *pData = static_cast<CChmodOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (prevResult != FZ_REPLY_OK)
		pData->m_useAbsolute = true;

	bool res;
	switch (pData->opState)
	{
	case chmod_chmod:
		res = Send(_T("SITE CHMOD ") + pData->m_cmd.GetPermission() + _T(" ") + pData->m_cmd.GetPath().FormatFilename(pData->m_cmd.GetFile(), !pData->m_useAbsolute));
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("unknown op state: %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!res)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}
