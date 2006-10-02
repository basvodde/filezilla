#include "FileZilla.h"
#include "ftpcontrolsocket.h"
#include "transfersocket.h"
#include "directorylistingparser.h"
#include "directorycache.h"
#include "iothread.h"
#include <wx/regex.h>
#include "externalipresolver.h"
#include "servercapabilities.h"

#define LOGON_LOGON		1
#define LOGON_SYST		2
#define LOGON_FEAT		3
#define LOGON_CLNT		4
#define LOGON_OPTSUTF8	5

BEGIN_EVENT_TABLE(CFtpControlSocket, CControlSocket)
EVT_FZ_EXTERNALIPRESOLVE(wxID_ANY, CFtpControlSocket::OnExternalIPAddress)
END_EVENT_TABLE();

CRawTransferOpData::CRawTransferOpData()
	: COpData(cmd_rawtransfer)
{
	bTriedPasv = bTriedActive = false;
	bPasv = true;
}

CFtpTransferOpData::CFtpTransferOpData()
{
	transferEndReason = 0;
	tranferCommandSent = false;
	resumeOffset = 0;
}

CFtpFileTransferOpData::CFtpFileTransferOpData()
{
	pIOThread = 0;
}

CFtpFileTransferOpData::~CFtpFileTransferOpData()
{
	if (pIOThread)
	{
		CIOThread *pThread = pIOThread;
		pIOThread = 0;
		pThread->Destroy();
		delete pThread;
	}
}

enum filetransferStates
{
	filetransfer_init = 0,
	filetransfer_waitcwd,
	filetransfer_waitlist,
	filetransfer_size,
	filetransfer_mdtm,
	filetransfer_resumetest,
	filetransfer_transfer,
	filetransfer_waittransfer,
	filetransfer_waitresumetest
};

enum rawtransferStates
{
	rawtransfer_init = 0,
	rawtransfer_type,
	rawtransfer_port_pasv,
	rawtransfer_rest,
	rawtransfer_transfer,
	rawtransfer_waitfinish,
	rawtransfer_waittransferpre,
	rawtransfer_waittransfer,
	rawtransfer_waitsocket
};

class CFtpLogonOpData : public COpData
{
public:
	CFtpLogonOpData()
		: COpData(cmd_connect)
	{
		logonSequencePos = 0;
		logonType = 0;
		nCommand = 0;

		waitChallenge = false;
		gotPassword = false;
		waitForAsyncRequest = false;

	}

	virtual ~CFtpLogonOpData()
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
	m_pIPResolver = 0;
	m_pTransferSocket = 0;
	m_sentRestartOffset = false;
	m_bufferLen = 0;
	m_repliesToSkip = 0;
	m_pendingReplies = 1;
}

CFtpControlSocket::~CFtpControlSocket()
{
}

void CFtpControlSocket::OnReceive(wxSocketEvent &event)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::OnReceive()"));

	Read(m_receiveBuffer + m_bufferLen, RECVBUFFERSIZE - m_bufferLen);
	if (Error())
	{
		if (LastError() != wxSOCKET_WOULDBLOCK)
		{
			LogMessage(::Error, _("Disconnected from server"));
			DoClose();
		}
		return;
	}

	int numread = LastCount();
	if (!numread)
		return;

	m_pEngine->SetActive(true);

	char* start = m_receiveBuffer;
	m_bufferLen += numread;

	for (int i = start - m_receiveBuffer; i < m_bufferLen; i++)
	{
		char& p = m_receiveBuffer[i];
		if (p == '\r' ||
			p == '\n' ||
			p == 0)
		{
			int len = i - (start - m_receiveBuffer);
			if (!len)
			{
				start++;
				continue;
			}

			if (len > MAXLINELEN)
				len = MAXLINELEN;
			p = 0;
			wxString line = ConvToLocal(start);
			start = m_receiveBuffer + i + 1;

			ParseLine(line);

			// Abort if connection got closed
			if (!m_pCurrentServer)
				return;
		}
	}
	memmove(m_receiveBuffer, start, m_bufferLen - (start - m_receiveBuffer));
	m_bufferLen -= (start -m_receiveBuffer);
	if (m_bufferLen > MAXLINELEN)
		m_bufferLen = MAXLINELEN;
}

void CFtpControlSocket::ParseLine(wxString line)
{
	LogMessage(Response, line);
	SetAlive();

	if (GetCurrentCommandId() == cmd_connect && m_pCurOpData)
	{
		CFtpLogonOpData* pData = reinterpret_cast<CFtpLogonOpData *>(m_pCurOpData);
		if (pData->waitChallenge)
		{
			wxString& challenge = pData->challenge;
			if (challenge != _T(""))
#ifdef __WXMSW__
				challenge += _T("\r\n");
#else
				challenge += _T("\n");
#endif
			challenge += line;
		}
		else if (pData->opState == LOGON_FEAT)
		{
			wxString up = line.Upper();
			if (up == _T(" UTF8"))
				CServerCapabilities::SetCapability(*m_pCurrentServer, utf8_command, yes);
			else if (up == _T(" CLNT") || up.Left(6) == _T(" CLNT ")) 
				CServerCapabilities::SetCapability(*m_pCurrentServer, clnt_command, yes);
			else if (up == _T(" MLSD") || up.Left(6) == _T(" MLSD "))
				CServerCapabilities::SetCapability(*m_pCurrentServer, mlsd_command, yes);
			else if (up == _T(" MLST") || up.Left(6) == _T(" MLST "))
				CServerCapabilities::SetCapability(*m_pCurrentServer, mlsd_command, yes);
		}
	}
	//Check for multi-line responses
	if (line.Len() > 3)
	{
		if (m_MultilineResponseCode != _T(""))
		{
			if (line.Left(4) == m_MultilineResponseCode)
			{
				// end of multi-line found
				m_MultilineResponseCode.Clear();
				m_Response = line;
				ParseResponse();
				m_Response = _T("");
			}
		}
		// start of new multi-line
		else if (line.GetChar(3) == '-')
		{
			// DDD<SP> is the end of a multi-line response
			m_MultilineResponseCode = line.Left(3) + _T(" ");
		}
		else
		{
			m_Response = line;
			ParseResponse();
			m_Response = _T("");
		}
	}
}

void CFtpControlSocket::OnConnect(wxSocketEvent &event)
{
	SetAlive();
	LogMessage(Status, _("Connection established, waiting for welcome message..."));
	m_pendingReplies = 1;
	m_repliesToSkip = 0;
	Logon();
}

void CFtpControlSocket::ParseResponse()
{
	if (m_Response[0] != '1')
	{
		if (m_pendingReplies > 0)
			m_pendingReplies--;
		else
		{
			LogMessage(Debug_Warning, _T("Unexpected reply, no reply was pending."));
			return;
		}
	}

	if (m_repliesToSkip)
	{
		LogMessage(Debug_Info, _T("Skipping reply after cancelled operation."));
		if (m_Response[0] != '1')
			m_repliesToSkip--;
		return;
	}

	enum Command commandId = GetCurrentCommandId();
	switch (commandId)
	{
	case cmd_connect:
		LogonParseResponse();
		break;
	case cmd_list:
		ListParseResponse();
		break;
	case cmd_cwd:
		ChangeDirParseResponse();
		break;
	case cmd_transfer:
		FileTransferParseResponse();
		break;
	case cmd_raw:
		RawCommandParseResponse();
		break;
	case cmd_delete:
		DeleteParseResponse();
		break;
	case cmd_removedir:
		RemoveDirParseResponse();
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
	case cmd_rawtransfer:
		TransferParseResponse();
		break;
	case cmd_none:
		LogMessage(Debug_Verbose, _T("Out-of-order reply, ignoring."));
		break;
	default:
		LogMessage(Debug_Warning, _T("No action for parsing replies to command %d"), (int)commandId);
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
	m_pCurOpData = new CFtpLogonOpData;

	const enum CharsetEncoding encoding = m_pCurrentServer->GetEncodingType();
	if (encoding == ENCODING_AUTO && CServerCapabilities::GetCapability(*m_pCurrentServer, utf8_command) != no)
		m_useUTF8 = true;
	else if (encoding == ENCODING_UTF8)
		m_useUTF8 = true;

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

	CFtpLogonOpData *pData = reinterpret_cast<CFtpLogonOpData *>(m_pCurOpData);

	const int LO = -2, ER = -1;
	const int NUMLOGIN = 9; // currently supports 9 different login sequences
	int logonseq[NUMLOGIN][20] = {
		// this array stores all of the logon sequences for the various firewalls
		// in blocks of 3 nums. 1st num is command to send, 2nd num is next point in logon sequence array
		// if 200 series response is rec'd from server as the result of the command, 3rd num is next
		// point in logon sequence if 300 series rec'd
		{0,LO,3, 1,LO,6, 12,LO,ER}, // no firewall
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

	if (!pData->opState)
	{
		if (code != 2 && code != 3)
		{
			DoClose(FZ_REPLY_DISCONNECTED);
			return FZ_REPLY_DISCONNECTED;
		}

		pData->opState = LOGON_LOGON;
		pData->nCommand = logonseq[pData->logonType][0];
	}
	else if (pData->opState == LOGON_LOGON)
	{
		if (code != 2 && code != 3)
		{
			if (m_pCurrentServer->GetEncodingType() == ENCODING_AUTO && m_useUTF8)
			{
				// Fall back to local charset for the case that the server might not
				// support UTF8 and the login data contains non-ascii characters.
				bool asciiOnly = true;
				for (unsigned int i = 0; i < m_pCurrentServer->GetUser().Length(); i++)
					if ((unsigned int)m_pCurrentServer->GetUser()[i] > 127)
						asciiOnly = false;
				for (unsigned int i = 0; i < m_pCurrentServer->GetPass().Length(); i++)
					if ((unsigned int)m_pCurrentServer->GetPass()[i] > 127)
						asciiOnly = false;
				for (unsigned int i = 0; i < m_pCurrentServer->GetAccount().Length(); i++)
					if ((unsigned int)m_pCurrentServer->GetAccount()[i] > 127)
						asciiOnly = false;
				if (!asciiOnly)
				{
					LogMessage(Status, _("Login data contains non-ascii characters and server might not be UTF-8 aware. Trying local charset."), 0);
					m_useUTF8 = false;
					pData->opState = LOGON_LOGON;
					pData->nCommand = logonseq[pData->logonType][0];
					pData->logonSequencePos = 0;
					return LogonSend();
				}
			}

			int error = FZ_REPLY_DISCONNECTED;
			if (pData->nCommand == 1 && code == 5)
				error |= FZ_REPLY_PASSWORDFAILED;
			DoClose(error);
			return FZ_REPLY_ERROR;
		}

		pData->waitChallenge = false;
		pData->logonSequencePos = logonseq[pData->logonType][pData->logonSequencePos + code - 1];

		switch(pData->logonSequencePos)
		{
		case ER: // ER means something has gone wrong
			DoClose();
			return FZ_REPLY_ERROR;
		case LO: //LO means we are logged on
			wxString system;
			enum capabilities cap = CServerCapabilities::GetCapability(*GetCurrentServer(), syst_command, &system);
			if (cap == unknown)
			{
				pData->opState = LOGON_SYST;
				return LogonSend();
			}
			else if (cap == yes)
			{
				if (system.Left(3) == _T("MVS") && m_pCurrentServer->GetType() == DEFAULT)
					m_pCurrentServer->SetType(MVS);
			}

			if (CServerCapabilities::GetCapability(*GetCurrentServer(), feat_command) == unknown)
			{
				pData->opState = LOGON_FEAT;
				return LogonSend();
			}

			if (system.Find(_T("FileZilla")) == -1 &&
				m_useUTF8 && CServerCapabilities::GetCapability(*GetCurrentServer(), utf8_command) == yes)
			{
				// If server is not FileZilla Server, we might have to send some extra commands to enable
				// UTF-8
				if (CServerCapabilities::GetCapability(*GetCurrentServer(), clnt_command) == yes)
					pData->opState = LOGON_CLNT;
				else
					pData->opState = LOGON_OPTSUTF8;
				return LogonSend();
			}

			LogMessage(Status, _("Connected"));
			ResetOperation(FZ_REPLY_OK);
			return true;
		}

		pData->nCommand = logonseq[pData->logonType][pData->logonSequencePos];
	}
	else if (pData->opState == LOGON_SYST)
	{
		CServerCapabilities::SetCapability(*GetCurrentServer(), syst_command, (code == 2) ? yes : no, m_Response.Mid(4));

		if (code == 2 && m_Response.Length() > 7 && m_Response.Mid(3, 4) == _T(" MVS"))
		{
			if (m_pCurrentServer->GetType() == DEFAULT)
				m_pCurrentServer->SetType(MVS);
		}

		if (CServerCapabilities::GetCapability(*GetCurrentServer(), feat_command) == unknown)
		{
			pData->opState = LOGON_FEAT;
			return LogonSend();
		}

		if (m_Response.Find(_T("FileZilla")) == -1 &&
			m_useUTF8 && CServerCapabilities::GetCapability(*GetCurrentServer(), utf8_command) == yes)
		{
			// If server is not FileZilla Server, we might have to send some extra commands to enable
			// UTF-8
			if (CServerCapabilities::GetCapability(*GetCurrentServer(), clnt_command) == yes)
				pData->opState = LOGON_CLNT;
			else
				pData->opState = LOGON_OPTSUTF8;
			return LogonSend();
		}

		LogMessage(Status, _("Connected"));
		ResetOperation(FZ_REPLY_OK);
		return true;
	}
	else if (pData->opState == LOGON_FEAT)
	{
		if (code == 2)
		{
			CServerCapabilities::SetCapability(*GetCurrentServer(), feat_command, yes);
			if (CServerCapabilities::GetCapability(*m_pCurrentServer, utf8_command) != yes)
				CServerCapabilities::SetCapability(*m_pCurrentServer, utf8_command, no);
			if (CServerCapabilities::GetCapability(*m_pCurrentServer, clnt_command) != yes)
				CServerCapabilities::SetCapability(*m_pCurrentServer, clnt_command, no);
		}
		else
			CServerCapabilities::SetCapability(*GetCurrentServer(), feat_command, no);

		const enum CharsetEncoding encoding = m_pCurrentServer->GetEncodingType();
		if (encoding == ENCODING_AUTO && CServerCapabilities::GetCapability(*m_pCurrentServer, utf8_command) != yes)
			m_useUTF8 = false;

		wxString system;
		CServerCapabilities::GetCapability(*GetCurrentServer(), syst_command, &system);
			
		if (system.Find(_T("FileZilla")) != -1 &&
			m_useUTF8 && CServerCapabilities::GetCapability(*GetCurrentServer(), utf8_command) == yes)
		{
			if (CServerCapabilities::GetCapability(*GetCurrentServer(), clnt_command) == yes)
				pData->opState = LOGON_CLNT;
			else
				pData->opState = LOGON_OPTSUTF8;
			return LogonSend();
		}

		LogMessage(Status, _("Connected"));
		ResetOperation(FZ_REPLY_OK);
		return true;
	}
	else if (pData->opState == LOGON_CLNT)
	{
		// Don't check return code, it has no meaning for us
		pData->opState = LOGON_OPTSUTF8;
	}
	else if (pData->opState == LOGON_OPTSUTF8)
	{
		// If server obeys RFC 2640 this command had no effect, return code
		// is irrelevant

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

	CFtpLogonOpData *pData = reinterpret_cast<CFtpLogonOpData *>(m_pCurOpData);

	bool res;
	switch (pData->opState)
	{
	case LOGON_SYST:
		res = Send(_T("SYST"));
		break;
	case LOGON_LOGON:
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

			res = Send(_T("PASS ") + m_pCurrentServer->GetPass(), true);
			break;
		case 12:
			if (m_pCurrentServer->GetAccount() == _T(""))
			{
				LogMessage(::Error, _("Server requires an account. Please specify an account using the Site Manager"));
				DoClose(FZ_REPLY_DISCONNECTED);
				res = false;
				break;
			}

			res = Send(_T("ACCT ") + m_pCurrentServer->GetAccount());
			break;

		default:
			ResetOperation(FZ_REPLY_INTERNALERROR);
			res = false;
			break;
		}
		break;
	case LOGON_FEAT:
		res = Send(_T("FEAT"));
		break;
	case LOGON_CLNT:
		// Some servers refuse to enable UTF8 if client does not send CLNT command
		// to fix compatibility with Internet Explorer, but in the process breaking
		// compatibility with other clients.
		// Rather than forcing MS to fix Internet Explorer, letting other clients
		// suffer is a questionable decision in my opinion.
		res = Send(_T("CLNT FileZilla"));
		break;
	case LOGON_OPTSUTF8:
		// Handle servers that disobey RFC 2640 by having UTF8 in their FEAT
		// response but do not use UTF8 unless OPTS UTF8 ON gets send.
		// However these servers obey a conflicting ietf draft:
		// http://www.ietf.org/proceedings/02nov/I-D/draft-ietf-ftpext-utf-8-option-00.txt
		// Example servers are, amongst others, G6 FTP Server and RaidenFTPd.
		res = Send(_T("OPTS UTF8 ON"));
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
	if (m_Response == _T(""))
		return 0;

	if (m_Response[0] < '0' || m_Response[0] > '9')
		return 0;

	return m_Response[0] - '0';
}

bool CFtpControlSocket::Send(wxString str, bool maskArgs /*=false*/)
{
	int pos;
	if (maskArgs && (pos = str.Find(_T(" "))) != -1)
	{
		wxString stars('*', str.Length() - pos - 1);
		LogMessage(Command, str.Left(pos + 1) + stars);
	}
	else
		LogMessage(Command, str);

	str += _T("\r\n");
	wxCharBuffer buffer = ConvToServer(str);
	if (!buffer)
	{
		LogMessage(::Error, _T("Failed to convert command to 8 bit charset"));
		return false;
	}
	unsigned int len = (unsigned int)strlen(buffer);
	bool res = CControlSocket::Send(buffer, len);
	if (res)
		m_pendingReplies++;
	return res;
}

class CFtpListOpData : public COpData, public CFtpTransferOpData
{
public:
	CFtpListOpData()
		: COpData(cmd_list)
	{
		m_pDirectoryListingParser = 0;
	}

	virtual ~CFtpListOpData()
	{
		delete m_pDirectoryListingParser;
	}

	CServerPath path;
	wxString subDir;

	CDirectoryListingParser* m_pDirectoryListingParser;

	// Set to true to get a directory listing even if a cache
	// lookup can be made after finding out true remote directory
	bool refresh;
};

enum listStates
{
	list_init = 0,
	list_waitcwd,
	list_waittransfer,
};

int CFtpControlSocket::List(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/, bool refresh /*=false*/)
{
	LogMessage(Status, _("Retrieving directory listing..."));

	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("List called from other command"));
	}
	CFtpListOpData *pData = new CFtpListOpData;
	pData->pNextOpData = m_pCurOpData;
	m_pCurOpData = pData;

	pData->opState = list_waitcwd;

	if (path.GetType() == DEFAULT)
		path.SetType(m_pCurrentServer->GetType());
	pData->path = path;
	pData->subDir = subDir;
	pData->refresh = refresh;

	int res = ChangeDir(path, subDir);
	if (res != FZ_REPLY_OK)
		return res;

	if (!pData->refresh)
	{
		// Do a cache lookup now that we know the correct directory
		CDirectoryCache cache;
		if (pData->pNextOpData)
		{
			bool hasUnsureEntries;
			bool found = cache.DoesExist(*m_pCurrentServer, m_CurrentPath, _T(""), hasUnsureEntries);
			if (found && !hasUnsureEntries)
			{
				if (!pData->path.IsEmpty() && pData->subDir != _T(""))
					cache.AddParent(*m_pCurrentServer, m_CurrentPath, pData->path, pData->subDir);
				
				delete pData;
				m_pCurOpData = pData->pNextOpData;
				
				return FZ_REPLY_OK;
			}
		}
		else
		{
			CDirectoryListing *pListing = new CDirectoryListing;
			CDirectoryCache cache;
			bool found = cache.Lookup(*pListing, *m_pCurrentServer, m_CurrentPath, false);
			if (found)
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
	}

	// Try to lock cache
	if (!TryLockCache(m_CurrentPath))
		return FZ_REPLY_WOULDBLOCK;

	delete m_pTransferSocket;
	m_pTransferSocket = new CTransferSocket(m_pEngine, this, ::list);
	pData->m_pDirectoryListingParser = new CDirectoryListingParser(this, *m_pCurrentServer);
	m_pTransferSocket->m_pDirectoryListingParser = pData->m_pDirectoryListingParser;

	InitTransferStatus(-1, 0);

	pData->opState = list_waittransfer;
#if 0 // Disabled for now
	if (CServerCapabilities::GetCapability(*m_pCurrentServer, mlsd_command) == yes)
		return Transfer(_T("MLSD"), pData);
	else
#endif
		return Transfer(_T("LIST"), pData);
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

	CFtpListOpData *pData = static_cast<CFtpListOpData *>(m_pCurOpData);
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
			CDirectoryCache cache;
			if (pData->pNextOpData)
			{
				bool hasUnsureEntries;
				bool found = cache.DoesExist(*m_pCurrentServer, m_CurrentPath, _T(""), hasUnsureEntries);
				if (found && !hasUnsureEntries)
				{
					if (!pData->path.IsEmpty() && pData->subDir != _T(""))
						cache.AddParent(*m_pCurrentServer, m_CurrentPath, pData->path, pData->subDir);

					ResetOperation(FZ_REPLY_OK);
					return FZ_REPLY_OK;
				}
			}
			else
			{
				CDirectoryListing *pListing = new CDirectoryListing;
				CDirectoryCache cache;
				bool found = cache.Lookup(*pListing, *m_pCurrentServer, m_CurrentPath, false);
				if (found)
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
		}

		delete m_pTransferSocket;
		m_pTransferSocket = new CTransferSocket(m_pEngine, this, ::list);
		pData->m_pDirectoryListingParser = new CDirectoryListingParser(this, *m_pCurrentServer);
		m_pTransferSocket->m_pDirectoryListingParser = pData->m_pDirectoryListingParser;

		InitTransferStatus(-1, 0);

		pData->opState = list_waittransfer;
#if 0 // Disabled for now
		if (CServerCapabilities::GetCapability(*m_pCurrentServer, mlsd_command) == yes)
			return Transfer(_T("MLSD"), pData);
		else
#endif
			return Transfer(_T("LIST"), pData);
	}
	else if (pData->opState == list_waittransfer)
	{
		if (prevResult == FZ_REPLY_OK)
		{
			CDirectoryListing *pListing = pData->m_pDirectoryListingParser->Parse(m_CurrentPath);

			CDirectoryCache cache;
			cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);

			SendDirectoryListing(pListing);
			m_pEngine->ResendModifiedListings();

			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else
		{
			if (pData->tranferCommandSent && IsMisleadingListResponse())
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
			else
			{
				if (prevResult & FZ_REPLY_ERROR)
				{
					CDirectoryListing *pListing = new CDirectoryListing;
					pListing->m_failed = true;
					pListing->path = m_CurrentPath;
					SendDirectoryListing(pListing);
				}
			}

			ResetOperation(FZ_REPLY_ERROR);
			return FZ_REPLY_ERROR;
		}
	}

	LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("invalid opstate"));
	ResetOperation(FZ_REPLY_INTERNALERROR);
	return FZ_REPLY_ERROR;
}

int CFtpControlSocket::ListParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::ListParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	LogMessage(Debug_Warning, _T("ListParseResponse should never be called"));
	ResetOperation(FZ_REPLY_INTERNALERROR);
	return FZ_REPLY_ERROR;
}

int CFtpControlSocket::ResetOperation(int nErrorCode)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::ResetOperation(%d)"), nErrorCode);

	CTransferSocket* pTransferSocket = m_pTransferSocket;
	m_pTransferSocket = 0;
	delete pTransferSocket;
	delete m_pIPResolver;
	m_pIPResolver = 0;

	m_repliesToSkip = m_pendingReplies;

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
	case cmd_cwd:
		return ChangeDirSend();
	case cmd_transfer:
		return FileTransferSend(prevResult);
	case cmd_mkdir:
		return MkdirSend();
	case cmd_rename:
		return RenameSend(prevResult);
	case cmd_chmod:
		return ChmodSend(prevResult);
	case cmd_rawtransfer:
		return TransferSend(prevResult);
	case cmd_raw:
		return RawCommandSend();
	case cmd_delete:
		return DeleteSend(prevResult);
	case cmd_removedir:
		return RemoveDirSend(prevResult);
	default:
		LogMessage(__TFILE__, __LINE__, this, ::Debug_Warning, _T("Unknown opID (%d) in SendNextCommand"), m_pCurOpData->opId);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}

	return FZ_REPLY_ERROR;
}

class CFtpChangeDirOpData : public COpData
{
public:
	CFtpChangeDirOpData()
		: COpData(cmd_cwd)
	{
		triedMkd = false;
	}

	virtual ~CFtpChangeDirOpData()
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

	CFtpChangeDirOpData *pData = new CFtpChangeDirOpData;
	pData->pNextOpData = m_pCurOpData;
	pData->opState = state;
	pData->path = path;
	pData->subDir = subDir;

	m_pCurOpData = pData;

	return SendNextCommand();;
}

int CFtpControlSocket::ChangeDirParseResponse()
{
	if (!m_pCurOpData)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
	CFtpChangeDirOpData *pData = static_cast<CFtpChangeDirOpData *>(m_pCurOpData);

	int code = GetReplyCode();
	bool error = false;
	switch (pData->opState)
	{
	case cwd_pwd:
		if (code != 2 && code != 3)
			error = true;
		else if (ParsePwdReply(m_Response))
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
				!static_cast<CFtpFileTransferOpData *>(pData->pNextOpData)->download && !pData->triedMkd)
			{
				pData->triedMkd = true;
				int res = Mkdir(pData->path);
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
		else if (ParsePwdReply(m_Response))
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
		else if (ParsePwdReply(m_Response))
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
	CFtpChangeDirOpData *pData = static_cast<CFtpChangeDirOpData *>(m_pCurOpData);

	wxString cmd;
	switch (pData->opState)
	{
	case cwd_pwd:
	case cwd_pwd_cwd:
	case cwd_pwd_subdir:
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
	
	if (localFile == _T(""))
	{
		if (!download)
			ResetOperation(FZ_REPLY_CRITICALERROR | FZ_REPLY_NOTSUPPORTED);
		else
			ResetOperation(FZ_REPLY_SYNTAXERROR);
		return FZ_REPLY_ERROR;
	}

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

	CFtpFileTransferOpData *pData = new CFtpFileTransferOpData;
	m_pCurOpData = pData;

	pData->localFile = localFile;
	pData->remotePath = remotePath;
	pData->remoteFile = remoteFile;
	pData->download = download;
	pData->transferSettings = transferSettings;
	pData->binary = transferSettings.binary;

	pData->opState = filetransfer_waitcwd;

	if (pData->remotePath.GetType() == DEFAULT)
		pData->remotePath.SetType(m_pCurrentServer->GetType());

	wxStructStat buf;
	int result;
	result = wxStat(pData->localFile, &buf);
	if (!result)
	{
		pData->localFileSize = buf.st_size;
	}

	int res = ChangeDir(pData->remotePath);
	if (res != FZ_REPLY_OK)
		return res;

	pData->opState = filetransfer_waitlist;

	CDirentry entry;
	bool dirDidExist;
	bool matchedCase;
	CDirectoryCache cache;
	bool found = cache.LookupFile(entry, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, pData->remoteFile, dirDidExist, matchedCase);
	bool shouldList = false;
	if (!found)
	{
		if (!dirDidExist)
			shouldList = true;
	}
	else
	{
		if (entry.unsure)
			shouldList = true;
		else
		{
			if (matchedCase)
			{
				pData->remoteFileSize = entry.size.GetLo() + ((wxFileOffset)entry.size.GetHi() << 32);

				pData->opState = filetransfer_resumetest;
				res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
			else
				pData->opState = filetransfer_size;
		}
	}
	if (shouldList)
	{
		res = List(CServerPath(), _T(""), true);
		if (res != FZ_REPLY_OK)
			return res;
	}

	pData->opState = filetransfer_resumetest;

	res = CheckOverwriteFile();
	if (res != FZ_REPLY_OK)
		return res;

	return SendNextCommand();
}

int CFtpControlSocket::FileTransferParseResponse()
{
	LogMessage(Debug_Verbose, _T("FileTransferParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(m_pCurOpData);
	if (pData->opState == list_init)
		return FZ_REPLY_ERROR;

	int code = GetReplyCode();
	bool error = false;
	switch (pData->opState)
	{
	case filetransfer_size:
		pData->opState = filetransfer_mdtm;
		if (m_Response.Left(4) == _T("213 ") && m_Response.Length() > 4)
		{
			wxString str = m_Response.Mid(4);
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
			pData->remoteFileSize = size;
		}
		else if (code == 2 || code == 3)
			LogMessage(Debug_Info, _T("Invalid SIZE reply"));
		break;
	case filetransfer_mdtm:
		pData->opState = filetransfer_resumetest;
		if (m_Response.Left(4) == _T("213 ") && m_Response.Length() > 16)
		{
			wxDateTime date;
			const wxChar *res = date.ParseFormat(m_Response.Mid(4), _T("%Y%m%d%H%M"));
			if (res && date.IsValid())
				pData->fileTime = date;
		}

		{
			int res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
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

	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(m_pCurOpData);

	if (pData->opState == filetransfer_waitcwd)
	{
		if (prevResult == FZ_REPLY_OK)
		{
			pData->opState = filetransfer_waitlist;

			CDirentry entry;
			bool dirDidExist;
			bool matchedCase;
			CDirectoryCache cache;
			bool found = cache.LookupFile(entry, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, pData->remoteFile, dirDidExist, matchedCase);
			bool shouldList = false;
			if (!found)
			{
				if (!dirDidExist)
					shouldList = true;
			}
			else
			{
				if (entry.unsure)
					shouldList = true;
				else
				{
					if (matchedCase)
					{
						pData->remoteFileSize = entry.size.GetLo() + ((wxFileOffset)entry.size.GetHi() << 32);

						pData->opState = filetransfer_resumetest;
						int res = CheckOverwriteFile();
						if (res != FZ_REPLY_OK)
							return res;
					}
					else
						pData->opState = filetransfer_size;
				}
			}
			if (shouldList)
			{
				int res = List();
				if (res != FZ_REPLY_OK)
					return res;
			}

			pData->opState = filetransfer_resumetest;

			int res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
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
			CDirentry entry;
			bool dirDidExist;
			bool matchedCase;
			CDirectoryCache cache;
			bool found = cache.LookupFile(entry, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, pData->remoteFile, dirDidExist, matchedCase);
			if (!found)
			{
				if (!dirDidExist)
                    pData->opState = filetransfer_size;
				else
				{
					pData->opState = filetransfer_resumetest;
					int res = CheckOverwriteFile();
					if (res != FZ_REPLY_OK)
						return res;
				}
			}
			else
			{
				if (matchedCase && !entry.unsure)
				{
					pData->remoteFileSize = entry.size.GetLo() + ((wxFileOffset)entry.size.GetHi() << 32);

					pData->opState = filetransfer_resumetest;
					int res = CheckOverwriteFile();
					if (res != FZ_REPLY_OK)
						return res;
				}
				else
					pData->opState = filetransfer_size;
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
	else if (pData->opState == filetransfer_waittransfer)
	{
		if (pData->tranferCommandSent)
			pData->transferInitiated = true;
		ResetOperation(prevResult);
		return prevResult;
	}
	else if (pData->opState == filetransfer_waitresumetest)
	{
		if (prevResult != FZ_REPLY_OK)
		{
			if (pData->transferEndReason == 2)
			{
				if (pData->localFileSize > ((wxFileOffset)1 << 32))
				{
					CServerCapabilities::SetCapability(*m_pCurrentServer, resume4GBbug, yes);
					LogMessage(::Error, _("Server does not support resume of files > 4GB."));
				}
				else
				{
					CServerCapabilities::SetCapability(*m_pCurrentServer, resume2GBbug, yes);
					LogMessage(::Error, _("Server does not support resume of files > 2GB."));
				}

				ResetOperation(prevResult | FZ_REPLY_CRITICALERROR);
				return FZ_REPLY_ERROR;
			}
			else
				ResetOperation(prevResult);
			return prevResult;
		}
		if (pData->localFileSize > ((wxFileOffset)1 << 32))
			CServerCapabilities::SetCapability(*m_pCurrentServer, resume4GBbug, no);
		else
			CServerCapabilities::SetCapability(*m_pCurrentServer, resume2GBbug, no);

		pData->opState = filetransfer_transfer;
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
	case filetransfer_resumetest:
	case filetransfer_transfer:
		if (m_pTransferSocket)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Verbose, _T("m_pTransferSocket != 0"));
			delete m_pTransferSocket;
			m_pTransferSocket = 0;
		}

		{
			wxFile* pFile = new wxFile;
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
					if (!pFile->Open(pData->localFile, wxFile::write_append))
					{
						delete pFile;
						LogMessage(::Error, _("Failed to open \"%s\" for appending / writing"), pData->localFile.c_str());
						ResetOperation(FZ_REPLY_ERROR);
						return FZ_REPLY_ERROR;
					}

					startOffset = pFile->SeekEnd(0);
					if (startOffset == wxInvalidOffset)
					{
						delete pFile;
						LogMessage(::Error, _("Could not seek to the end of the file"));
						ResetOperation(FZ_REPLY_ERROR);
						return FZ_REPLY_ERROR;
					}
					pData->localFileSize = pFile->Length();

					// Check resume capabilities
					if (pData->opState == filetransfer_resumetest)
					{
						int res = FileTransferTestResumeCapability();
						if ((res & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
						{
							delete pFile;
							// Server does not support resume but remote and local filesizes are equal
							return FZ_REPLY_OK;
						}
						if (res != FZ_REPLY_OK)
						{
							delete pFile;
							return res;
						}
					}
				}
				else
				{
					if (!pFile->Open(pData->localFile, wxFile::write))
					{
						delete pFile;
						LogMessage(::Error, _("Failed to open \"%s\" for writing"), pData->localFile.c_str());
						ResetOperation(FZ_REPLY_ERROR);
						return FZ_REPLY_ERROR;
					}

					pData->localFileSize = pFile->Length();
					startOffset = 0;
				}

				if (pData->resume)
					pData->resumeOffset = pData->localFileSize;
				else
					pData->resumeOffset = 0;

				InitTransferStatus(pData->remoteFileSize, startOffset);
			}
			else
			{
				if (!pFile->Open(pData->localFile, wxFile::read))
				{
					delete pFile;
					LogMessage(::Error, _("Failed to open \"%s\" for reading"), pData->localFile.c_str());
					ResetOperation(FZ_REPLY_ERROR);
					return FZ_REPLY_ERROR;
				}

				wxFileOffset startOffset;
				if (pData->resume)
				{
					if (pData->remoteFileSize > 0)
					{
						startOffset = pData->remoteFileSize;

						// Assume native 64 bit type exists
						if (pFile->Seek(startOffset, wxFromStart) == wxInvalidOffset)
						{
							delete pFile;
							LogMessage(::Error, _("Could not seek to offset %s within file"), wxLongLong(startOffset).ToString().c_str());
							ResetOperation(FZ_REPLY_ERROR);
							return FZ_REPLY_ERROR;
						}
					}
					else
						startOffset = 0;
				}
				else
					startOffset = 0;

				wxFileOffset len = pFile->Length();
				InitTransferStatus(len, startOffset);
			}
			pData->pIOThread = new CIOThread;
			if (!pData->pIOThread->Create(pFile, !pData->download, pData->binary))
			{
				// CIOThread will delete pFile
				delete pData->pIOThread;
				pData->pIOThread = 0;
				LogMessage(::Error, _("Could not spawn IO thread"));
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}

		m_pTransferSocket = new CTransferSocket(m_pEngine, this, pData->download ? download : upload);
		m_pTransferSocket->m_binaryMode = pData->transferSettings.binary;

		if (pData->download)
			cmd = _T("RETR ");
		else if (pData->resume)
			cmd = _T("APPE ");
		else
			cmd = _T("STOR ");
		cmd += pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath);

		pData->opState = filetransfer_waittransfer;
		return Transfer(cmd, pData);
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

	if (GetCurrentCommandId() != cmd_rawtransfer)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
		ResetOperation(FZ_REPLY_ERROR);
		return;
	}

	// Even if reason indicates a failure, don't reset operation
	// yet, wait for the reply to the LIST/RETR/... commands

	CRawTransferOpData *pData = static_cast<CRawTransferOpData *>(m_pCurOpData);
	if (pData->opState < rawtransfer_transfer || pData->opState == rawtransfer_waittransfer || pData->opState == rawtransfer_waittransferpre)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
		ResetOperation(FZ_REPLY_ERROR);
		return;
	}

	pData->pOldData->transferEndReason = reason;

	switch (pData->opState)
	{
	case rawtransfer_transfer:
		pData->opState = rawtransfer_waittransferpre;
		break;
	case rawtransfer_waitfinish:
		pData->opState = rawtransfer_waittransfer;
		break;
	case rawtransfer_waitsocket:
		ResetOperation((!reason) ? FZ_REPLY_OK : FZ_REPLY_ERROR);
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
		ResetOperation(FZ_REPLY_ERROR);
	}
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

			CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(m_pCurOpData);

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

					wxStructStat buf;
					int result;
					result = wxStat(pData->localFile, &buf);
					if (!result)
						pData->localFileSize = buf.st_size;
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
					bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, true);
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
									wxLongLong size = listing.m_pEntries[i].size;
									pData->remoteFileSize = size.GetLo() + ((wxFileOffset)size.GetHi() << 32);
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

			CFtpLogonOpData* pData = static_cast<CFtpLogonOpData*>(m_pCurOpData);

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

class CRawCommandOpData : public COpData
{
public:
	CRawCommandOpData(const wxString& command)
		: COpData(cmd_raw)
	{
		m_command = command;
	}

	wxString m_command;
};

int CFtpControlSocket::RawCommand(const wxString& command)
{
	wxASSERT(command != _T(""));

	m_pCurOpData = new CRawCommandOpData(command);

	return SendNextCommand();
}

int CFtpControlSocket::RawCommandSend()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::RawCommandSend"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateServer(*m_pCurrentServer);
	m_CurrentPath = CServerPath();

	CRawCommandOpData *pData = static_cast<CRawCommandOpData *>(m_pCurOpData);

	if (!Send(pData->m_command))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::RawCommandParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::RawCommandParseResponse"));

	int code = GetReplyCode();
	if (code == 2 || code == 3)
	{
		ResetOperation(FZ_REPLY_OK);
		return FZ_REPLY_OK;
	}
	else
	{
        ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
}

class CFtpDeleteOpData : public COpData
{
public:
	CFtpDeleteOpData()
		: COpData(cmd_delete)
	{
	}

	virtual ~CFtpDeleteOpData() { }

	CServerPath path;
	wxString file;
	bool omitPath;
};

int CFtpControlSocket::Delete(const CServerPath& path /*=CServerPath()*/, const wxString& file /*=_T("")*/)
{
	wxASSERT(!m_pCurOpData);
	CFtpDeleteOpData *pData = new CFtpDeleteOpData();
	m_pCurOpData = pData;
	pData->path = path;
	pData->file = file;
	pData->omitPath = true;

	int res = ChangeDir(pData->path);
	if (res != FZ_REPLY_OK)
		return res;

	return SendNextCommand();
}

int CFtpControlSocket::DeleteSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::DeleteSend(%d)"), prevResult);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpDeleteOpData *pData = static_cast<CFtpDeleteOpData *>(m_pCurOpData);

	if (prevResult != FZ_REPLY_OK)
		pData->omitPath = false;

	wxString filename = pData->path.FormatFilename(pData->file, pData->omitPath);
	if (filename == _T(""))
	{
		LogMessage(::Error, wxString::Format(_T("Filename cannot be constructed for folder %s and filename %s"), pData->path.GetPath().c_str(), pData->file.c_str()));
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, pData->path, pData->file, false);

	if (!Send(_T("DELE ") + filename))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::DeleteParseResponse()
{	
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::DeleteParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpDeleteOpData *pData = static_cast<CFtpDeleteOpData *>(m_pCurOpData);

	int code = GetReplyCode();
	if (code != 2 && code != 3)
		return ResetOperation(FZ_REPLY_ERROR);

	CDirectoryCache cache;
	cache.RemoveFile(*m_pCurrentServer, pData->path, pData->file);
	m_pEngine->ResendModifiedListings();

	return ResetOperation(FZ_REPLY_OK);
}

class CFtpRemoveDirOpData : public COpData
{
public:
	CFtpRemoveDirOpData()
		: COpData(cmd_removedir)
	{
	}

	virtual ~CFtpRemoveDirOpData() { }

	CServerPath path;
	CServerPath fullPath;
	wxString subDir;
	bool omitPath;
};

int CFtpControlSocket::RemoveDir(const CServerPath& path, const wxString& subDir)
{
	wxASSERT(!m_pCurOpData);
	CFtpRemoveDirOpData *pData = new CFtpRemoveDirOpData();
	m_pCurOpData = pData;
	pData->path = path;
	pData->subDir = subDir;
	pData->omitPath = true;
	pData->fullPath = path;

	if (!pData->fullPath.AddSegment(subDir))
	{
		LogMessage(::Error, wxString::Format(_T("Path cannot be constructed for folder %s and subdir %s"), path.GetPath().c_str(), subDir.c_str()));
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	int res = ChangeDir(pData->path);
	if (res != FZ_REPLY_OK)
		return res;

	return SendNextCommand();
}

int CFtpControlSocket::RemoveDirSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::RemoveDirSende()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpRemoveDirOpData *pData = static_cast<CFtpRemoveDirOpData *>(m_pCurOpData);

	if (prevResult != FZ_REPLY_OK)
		pData->omitPath = false;

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, pData->path, pData->subDir, false, CDirectoryCache::dir);

	if (pData->omitPath)
	{
		if (!Send(_T("RMD ") + pData->subDir))
			return FZ_REPLY_ERROR;
	}
	else
		if (!Send(_T("RMD ") + pData->fullPath.GetPath()))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CFtpControlSocket::RemoveDirParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::RemoveDirParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpRemoveDirOpData *pData = static_cast<CFtpRemoveDirOpData *>(m_pCurOpData);

	int code = GetReplyCode();
	if (code != 2 && code != 3)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

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

int CFtpControlSocket::Mkdir(const CServerPath& path)
{
	/* Directory creation works like this: First find a parent directory into
	 * which we can CWD, then create the subdirs one by one. If either part
	 * fails, try MKD with the full path directly.
	 */

	if (!m_pCurOpData)
		LogMessage(Status, _("Creating directory '%s'..."), path.GetPath().c_str());

	CMkdirOpData *pData = new CMkdirOpData;
	pData->opState = mkd_findparent;
	pData->path = path;

	if (!m_CurrentPath.IsEmpty())
	{
		// Unless the server is broken, a directory already exists if current directory is a subdir of it.
		if (m_CurrentPath == path || m_CurrentPath.IsSubdirOf(path, false))
			return FZ_REPLY_OK;

		if (m_CurrentPath.IsParentOf(path, false))
			pData->commonParent = m_CurrentPath;
		else
			pData->commonParent = path.GetCommonParent(m_CurrentPath);
	}

	pData->currentPath = path.GetParent();
	pData->segments.push_back(path.GetLastSegment());

	if (pData->currentPath == m_CurrentPath)
		pData->opState = mkd_mkdsub;

	pData->pNextOpData = m_pCurOpData;
	m_pCurOpData = pData;

	return SendNextCommand();
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
		else if (pData->currentPath == pData->commonParent)
			pData->opState = mkd_tryfull;
		else if (pData->currentPath.HasParent())
		{
			const CServerPath& parent = pData->currentPath.GetParent();
			pData->segments.push_front(pData->currentPath.GetLastSegment());
			pData->currentPath = parent;
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
			cache.InvalidateFile(*m_pCurrentServer, pData->currentPath, pData->segments.front(), true, CDirectoryCache::dir);
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

class CFtpRenameOpData : public COpData
{
public:
	CFtpRenameOpData(const CRenameCommand& command)
		: COpData(cmd_rename), m_cmd(command)
	{
		m_useAbsolute = false;
	}

	virtual ~CFtpRenameOpData() { }

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

	CFtpRenameOpData *pData = new CFtpRenameOpData(command);
	pData->opState = rename_rnfrom;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetFromPath());
	if (res != FZ_REPLY_OK)
		return res;

	return SendNextCommand();
}

int CFtpControlSocket::RenameParseResponse()
{
	CFtpRenameOpData *pData = static_cast<CFtpRenameOpData*>(m_pCurOpData);
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
	CFtpRenameOpData *pData = static_cast<CFtpRenameOpData*>(m_pCurOpData);
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
		{
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile(), false);
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile(), false);
			res = Send(_T("RNTO ") + pData->m_cmd.GetToPath().FormatFilename(pData->m_cmd.GetToFile(), !pData->m_useAbsolute && pData->m_cmd.GetFromPath() == pData->m_cmd.GetToPath()));
			break;
		}
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

class CFtpChmodOpData : public COpData
{
public:
	CFtpChmodOpData(const CChmodCommand& command)
		: COpData(cmd_chmod), m_cmd(command)
	{
		m_useAbsolute = false;
	}

	virtual ~CFtpChmodOpData() { }

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

	CFtpChmodOpData *pData = new CFtpChmodOpData(command);
	pData->opState = chmod_chmod;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetPath());
	if (res != FZ_REPLY_OK)
		return res;

	return SendNextCommand();
}

int CFtpControlSocket::ChmodParseResponse()
{
	CFtpChmodOpData *pData = static_cast<CFtpChmodOpData*>(m_pCurOpData);
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
	cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetPath(), pData->m_cmd.GetFile(), false, CDirectoryCache::unknown);

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CFtpControlSocket::ChmodSend(int prevResult /*=FZ_REPLY_OK*/)
{
	CFtpChmodOpData *pData = static_cast<CFtpChmodOpData*>(m_pCurOpData);
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

bool CFtpControlSocket::IsMisleadingListResponse() const
{
	// Some servers are broken. Instead of an empty listing, some MVS servers
	// for example they return "550 no members found"
	// Other servers return "550 No files found."

	if (!m_Response.CmpNoCase(_T("550 No members found.")))
		return true;

	if (!m_Response.CmpNoCase(_T("550 No data sets found.")))
		return true;

	if (!m_Response.CmpNoCase(_T("550 No files found.")))
		return true;

	return false;
}

bool CFtpControlSocket::ParsePasvResponse(CRawTransferOpData* pData)
{
	// Validate ip address
	wxString digit = _T("0*[0-9]{1,3}");
	const wxChar* dot = _T(",");
	wxString exp = _T("( |\\()(") + digit + dot + digit + dot + digit + dot + digit + dot + digit + dot + digit + _T(")( |\\)|$)");
	wxRegEx regex;
	regex.Compile(exp);

	if (!regex.Matches(m_Response))
		return false;

	pData->host = regex.GetMatch(m_Response, 2);

	int i = pData->host.Find(',', true);
	long number;
	if (i == -1 || !pData->host.Mid(i + 1).ToLong(&number))
		return false;

	pData->port = number; //get ls byte of server socket
	pData->host = pData->host.Left(i);
	i = pData->host.Find(',', true);
	if (i == -1 || !pData->host.Mid(i + 1).ToLong(&number))
		return false;

	pData->port += 256 * number; //add ms byte of server socket
	pData->host = pData-> host.Left(i);
	pData->host.Replace(_T(","), _T("."));

	const wxString peerIP = GetPeerIP();
	if (!IsRoutableAddress(pData->host) && IsRoutableAddress(peerIP))
	{
		if (!m_pEngine->GetOptions()->GetOptionVal(OPTION_PASVREPLYFALLBACKMODE) || pData->bTriedActive)
		{
			LogMessage(Debug_Verbose, _("Server sent passive reply with unroutable address. Using server address instead."));
			pData->host = peerIP;
		}
		else
			return false;
	}

	return true;
}

int CFtpControlSocket::GetExternalIPAddress(wxString& address)
{
	int mode = m_pEngine->GetOptions()->GetOptionVal(OPTION_EXTERNALIPMODE);

	if (mode)
	{
		if (m_pEngine->GetOptions()->GetOptionVal(OPTION_NOEXTERNALONLOCAL) &&
			!IsRoutableAddress(GetPeerIP()))
			// Skip next block, use local address
			goto getLocalIP;
	}

	if (mode == 1)
	{
		wxString ip = m_pEngine->GetOptions()->GetOption(OPTION_EXTERNALIP);
		if (ip != _T(""))
		{
			address = ip;
			return FZ_REPLY_OK;
		}

		LogMessage(::Debug_Warning, _("No external IP address set, trying default."));
	}
	else if (mode == 2)
	{
		if (!m_pIPResolver)
		{
			wxString resolverAddress = m_pEngine->GetOptions()->GetOption(OPTION_EXTERNALIPRESOLVER);

			LogMessage(::Debug_Info, _("Retrieving external IP address from %s"), resolverAddress.c_str());

			m_pIPResolver = new CExternalIPResolver(this);
			m_pIPResolver->GetExternalIP(resolverAddress, true);
			if (!m_pIPResolver->Done())
			{
				LogMessage(::Debug_Verbose, _T("Waiting for resolver thread"));
				return FZ_REPLY_WOULDBLOCK;
			}
		}
		if (!m_pIPResolver->Successful())
		{
			delete m_pIPResolver;
			m_pIPResolver = 0;

			LogMessage(::Debug_Warning, _("Failed to retrieve external ip address, using local address"));
		}
		else
		{
			LogMessage(::Debug_Info, _T("Got external IP address"));
			address = m_pIPResolver->GetIP();

			delete m_pIPResolver;
			m_pIPResolver = 0;

			return FZ_REPLY_OK;
		}
	}

getLocalIP:

	address = GetLocalIP();
	if (address == _T(""))
	{
		LogMessage(::Error, _("Failed to retrieve local ip address."), 1);
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_OK;
}

void CFtpControlSocket::OnExternalIPAddress(fzExternalIPResolveEvent& event)
{
	LogMessage(::Debug_Verbose, _T("CFtpControlSocket::OnExternalIPAddress()"));
	if (!m_pIPResolver)
	{
		LogMessage(::Debug_Info, _T("Ignoring event"));
		return;
	}

	SendNextCommand();
}

int CFtpControlSocket::Transfer(const wxString& cmd, CFtpTransferOpData* oldData)
{
	CRawTransferOpData *pData = new CRawTransferOpData;
	pData->pNextOpData = m_pCurOpData;
	m_pCurOpData = pData;

	pData->cmd = cmd;
	pData->pOldData = oldData;
	pData->pOldData->transferEndReason = 0;

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

	pData->opState = rawtransfer_type;

	return SendNextCommand();
}

int CFtpControlSocket::TransferParseResponse()
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::TransferParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CRawTransferOpData *pData = static_cast<CRawTransferOpData *>(m_pCurOpData);
	if (pData->opState == rawtransfer_init)
		return FZ_REPLY_ERROR;

	int code = GetReplyCode();

	LogMessage(Debug_Debug, _T("  code = %d"), code);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	bool error = false;
	switch (pData->opState)
	{
	case rawtransfer_type:
		if (code != 2 && code != 2)
			error = true;
		else
			pData->opState = rawtransfer_port_pasv;
		break;
	case rawtransfer_port_pasv:
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
			if (!ParsePasvResponse(pData))
			{
				if (!pData->bTriedActive)
					pData->bPasv = false;
				else
					error = true;
				break;
			}
		}
		if (pData->pOldData->resumeOffset > 0 || m_sentRestartOffset)
			pData->opState = rawtransfer_rest;
		else
			pData->opState = rawtransfer_transfer;
		break;
	case rawtransfer_rest:
		if (pData->pOldData->resumeOffset == 0)
			m_sentRestartOffset = false;
		if (pData->pOldData->resumeOffset != 0 && code != 2 && code != 3)
			error = true;
		else
			pData->opState = rawtransfer_transfer;
		break;
	case rawtransfer_transfer:
		if (code != 1)
			error = true;
		else
			pData->opState = rawtransfer_waitfinish;
		break;
	case rawtransfer_waittransferpre:
		if (code != 1)
			error = true;
		else
			pData->opState = rawtransfer_waittransfer;
		break;
	case rawtransfer_waitfinish:
		if (code != 2 && code != 3)
			error = true;
		else
			pData->opState = rawtransfer_waitsocket;
		break;
	case rawtransfer_waittransfer:
		if (code != 2 && code != 3)
			error = true;
		else
		{
			if (pData->pOldData->transferEndReason)
			{
				error = true;
				break;
			}

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

	return TransferSend();
}

int CFtpControlSocket::TransferSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::TransferSend(%d)"), prevResult);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!m_pTransferSocket)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pTransferSocket"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CRawTransferOpData *pData = static_cast<CRawTransferOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	wxString cmd;
	switch (pData->opState)
	{
	case rawtransfer_type:
		if (pData->pOldData->binary)
			cmd = _T("TYPE I");
		else
			cmd = _T("TYPE A");
		break;
	case rawtransfer_port_pasv:
		if (pData->bPasv)
		{
			pData->bTriedPasv = true;
			cmd = _T("PASV");
		}
		else
		{
			wxString address;
			int res = GetExternalIPAddress(address);
			if (res == FZ_REPLY_WOULDBLOCK)
				return res;
			else if (res == FZ_REPLY_OK)
			{
				wxString portArgument = m_pTransferSocket->SetupActiveTransfer(address);
                if (portArgument != _T(""))
				{
					pData->bTriedActive = true;
					cmd = _T("PORT " + portArgument);
					break;
				}
			}

			if (pData->bTriedPasv)
			{
				LogMessage(::Error, _("Failed to create listening socket for active mode transfer"));
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
			LogMessage(::Debug_Warning, _("Failed to create listening socket for active mode transfer"));
			pData->bTriedActive = true;
			pData->bTriedPasv = true;
			pData->bPasv = true;
			cmd = _T("PASV");
		}
		break;
	case rawtransfer_rest:
		cmd = _T("REST ") + pData->pOldData->resumeOffset.ToString();
		if (pData->pOldData->resumeOffset > 0)
			m_sentRestartOffset = true;
		break;
	case rawtransfer_transfer:
		if (pData->bPasv)
		{
			if (!m_pTransferSocket->SetupPassiveTransfer(pData->host, pData->port))
			{
				LogMessage(::Error, _("Could not establish connection to server"));
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}

		cmd = pData->cmd;
		pData->pOldData->tranferCommandSent = true;

		SetTransferStatusStartTime();
		m_pTransferSocket->SetActive();
		break;
	case rawtransfer_waitfinish:
	case rawtransfer_waittransferpre:
	case rawtransfer_waittransfer:
	case rawtransfer_waitsocket:
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

int CFtpControlSocket::FileTransferTestResumeCapability()
{
	LogMessage(Debug_Verbose, _T("FileTransferTestResumeCapability()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CFtpFileTransferOpData *pData = static_cast<CFtpFileTransferOpData *>(m_pCurOpData);

	if (!pData->download)
		return FZ_REPLY_OK;

	for (int i = 0; i < 2; i++)
	{
		if (pData->localFileSize >= ((wxFileOffset)1 << (i ? 31 : 32)))
		{
			switch (CServerCapabilities::GetCapability(*GetCurrentServer(), i ? resume2GBbug : resume4GBbug))
			{
			case yes:
				if (pData->remoteFileSize == pData->localFileSize)
				{
					LogMessage(::Debug_Info, _("Server does not support resume of files > %d GB. End transfer since filesizes match."), i ? 2 : 4);
					ResetOperation(FZ_REPLY_OK);
					return FZ_REPLY_CANCELED;
				}
				LogMessage(::Error, _("Server does not support resume of files > %d GB."), i ? 2 : 4);
				ResetOperation(FZ_REPLY_CRITICALERROR);
				return FZ_REPLY_ERROR;
			case unknown:
				if (pData->remoteFileSize < pData->localFileSize)
				{
					// Don't perform test
					break;
				}
				if (pData->remoteFileSize == pData->localFileSize)
				{
					LogMessage(::Debug_Info, _("Server may not support resume of files > %d GB. End transfer since filesizes match."), i ? 2 : 4);
					ResetOperation(FZ_REPLY_OK);
					return FZ_REPLY_CANCELED;
				}
				else if (pData->remoteFileSize > pData->localFileSize)
				{
					LogMessage(Status, _("Testing resume capabilities of server"));

					pData->opState = filetransfer_waitresumetest;
					pData->resumeOffset = pData->remoteFileSize - 1;

					m_pTransferSocket = new CTransferSocket(m_pEngine, this, resumetest);

					return Transfer(_T("RETR ") + pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath), pData);
				}
				break;
			case no:
				break;
			}
		}
	}

	return FZ_REPLY_OK;
}
