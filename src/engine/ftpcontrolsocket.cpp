#include "FileZilla.h"
#include "ftpcontrolsocket.h"
#include "transfersocket.h"
#include "directorylistingparser.h"
#include "directorycache.h"

#include <wx/filefn.h>
#include <wx/file.h>

CFtpControlSocket::CFtpControlSocket(CFileZillaEngine *pEngine) : CControlSocket(pEngine)
{
	m_ReceiveBuffer.Alloc(2000);

	m_pTransferSocket = 0;
}

CFtpControlSocket::~CFtpControlSocket()
{
	delete m_pCurOpData;
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
				
			//Check for multi-line responses
			if (m_ReceiveBuffer.Len() > 3)
			{
				if (m_MultilineResponseCode != _T(""))
				{
					if (m_ReceiveBuffer.Left(4) != m_MultilineResponseCode)
 						m_ReceiveBuffer.Empty();
					else // end of multi-line found
					{
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
}

void CFtpControlSocket::ParseResponse()
{
	enum Command commandId = GetCurrentCommandId();
	switch (commandId)
	{
	case cmd_connect:
		Logon();
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
	default:
		break;
	}
}

class CLogonOpData : public COpData
{
public:
	CLogonOpData()
	{
		logonSequencePos = 0;
		logonType = 0;

		opId = cmd_connect;
	}

	virtual ~CLogonOpData()
	{
	}

	int logonSequencePos;
	int logonType;
};

bool CFtpControlSocket::Logon()
{
	if (!m_pCurOpData)
		m_pCurOpData = new CLogonOpData;

	CLogonOpData *pData = (CLogonOpData *)(m_pCurOpData);

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

	int nCommand = 0;
	int code = GetReplyCode();
	if (code != 2 && code != 3)
	{
		DoClose(FZ_REPLY_DISCONNECTED);
		return false;
	}
	if (!pData->opState)
	{
		pData->opState = 1;
		nCommand = logonseq[pData->logonType][0];
	}
	else if (pData->opState == 1)
	{
		pData->logonSequencePos = logonseq[pData->logonType][pData->logonSequencePos + code - 1];

		switch(pData->logonSequencePos)
		{
		case ER: // ER means summat has gone wrong
			DoClose();
			return false;
		case LO: //LO means we are logged on
			pData->opState = 2;
			Send(_T("SYST"));
			return false;
		}

		nCommand = logonseq[pData->logonType][pData->logonSequencePos];
	}
	else if (pData->opState == 2)
	{
		LogMessage(Status, _("Connected"));
		ResetOperation(FZ_REPLY_OK);
		return true;
	}

	switch (nCommand)
	{
	case 0:
		Send(_T("USER ") + m_pCurrentServer->GetUser());
		break;
	case 1:
		Send(_T("PASS ") + m_pCurrentServer->GetPass());
		break;
	default:
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}

	return false;
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
	wxCharBuffer buffer = wxConvCurrent->cWX2MB(str);
	unsigned int len = (unsigned int)strlen(buffer);
	return CControlSocket::Send(buffer, len);
}

class CListOpData : public COpData
{
public:
	CListOpData()
	{
		opId = cmd_list;
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
};

enum listStates
{
	list_init = 0,
	list_waitcwd,
	list_port_pasv,
	list_type,
	list_list,
	list_waitfinish,
	list_waitlistpre,
	list_waitlist,
	list_waitsocket
};

int CFtpControlSocket::List(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/)
{
	LogMessage(Status, _("Retrieving directory listing..."));

	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("List called from other command"));
	}
	CListOpData *pData = new CListOpData;
	pData->pNextOpData = m_pCurOpData;
	m_pCurOpData = pData;
			
	pData->bPasv = m_pEngine->GetOptions()->GetOptionVal(OPTION_USEPASV) != 0;
	pData->bTriedPasv = pData->bTriedActive = false;
	pData->opState = list_waitcwd;

	pData->path = path;
	pData->subDir = subDir;

	int res = ChangeDir(path, subDir);
	if (res != FZ_REPLY_OK)
		return res;

	pData->opState = list_port_pasv;

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
		pData->opState = list_port_pasv;
	}

	wxString cmd;
	switch (pData->opState)
	{
	case list_port_pasv:
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
	case list_type:
		cmd = _T("TYPE A");
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
			pData->port = number; //add ms byte of server socket
			pData->port += 256 * number;
			pData->host = temp.Left(i);
			pData->host.Replace(_T(","), _T("."));
		}
		pData->opState = list_type;
		break;
	case list_type:
		// Don't check error code here, we can live perfectly without it
		pData->opState = list_list;
		break;
	case list_list:
		if (code != 1)
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
			CDirectoryListing *pListing = m_pTransferSocket->m_pDirectoryListingParser->Parse(m_CurrentPath);

			CDirectoryCache cache;
			cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);
			
			CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(pListing);
			m_pEngine->AddNotification(pNotification);

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

	switch (m_pCurOpData->opId)
	{
	case cmd_list:
		return ListSend(prevResult);
	case cmd_connect:
		return Logon();
	case cmd_private:
		return ChangeDirSend();
	case cmd_transfer:
		return FileTransferSend(prevResult);
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
	}

	virtual ~CChangeDirOpData()
	{
	}

	CServerPath path;
	wxString subDir;

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
			error = true;
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

CFileTransferOpData::CFileTransferOpData()
{
	opId = cmd_transfer;
	pFile = 0;
	resume = false;
	totalSize = leftSize = -1;
	tryAbsolutePath = false;
	bTriedPasv = bTriedActive = false;
	fileSize = -1;
	waitAsyncRequest = false;
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
	filetransfer_rest,
	filetransfer_transfer,
	filetransfer_waitfinish,
	filetransfer_waittransferpre,
	filetransfer_waittransfer,
	filetransfer_waitsocket
};

int CFtpControlSocket::FileTransfer(const wxString localFile, const CServerPath &remotePath, const wxString &remoteFile, bool download)
{
	LogMessage(Debug_Verbose, _T("CFtpControlSocket::FileTransfer()"));
	
	if (download)
	{
		wxString filename = remotePath.GetPath() + remoteFile;
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
			
	pData->bPasv = m_pEngine->GetOptions()->GetOptionVal(OPTION_USEPASV) != 0;
	pData->opState = filetransfer_waitcwd;

	int res = ChangeDir(remotePath);
	if (res != FZ_REPLY_OK)
		return res;

	pData->opState = filetransfer_waitlist;

	CDirectoryListing listing;
	CDirectoryCache cache;
	bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
	if (!found)
	{
		res = List();
		if (res != FZ_REPLY_OK)
			return res;

		pData->opState = filetransfer_type;

		res = CheckOverwriteFile();
		if (res != FZ_REPLY_OK)
			return res;
	}
	else
	{
		bool differentCase = false;

		for (unsigned int i = 0; i < listing.m_entryCount; i++)
		{
			if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
			{
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
			res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
		}
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
			wxLongLong size = 0;
			const wxChar *buf = str.c_str();
			while (*buf)
			{
				if (*buf < '0' || *buf > '9')
					break;

				size *= 10;
				size += *buf - '0';
				buf++;
			}
			if (!*buf)
				pData->totalSize = size;
		}
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
			pData->port = number; //add ms byte of server socket
			pData->port += 256 * number;
			pData->host = temp.Left(i);
			pData->host.Replace(_T(","), _T("."));
		}

		if (pData->resume && pData->download)
			pData->opState = filetransfer_rest;
		else
			pData->opState = filetransfer_transfer;

		pData->pFile = new wxFile;
		if (pData->download)
		{
			if (pData->resume)
			{
				if (!pData->pFile->Open(pData->localFile, wxFile::write_append))
				{
					LogMessage(::Error, _("Failed to open \"%s\" for appending / writing"), pData->localFile.c_str());
					error = true;
				}
			}
			else
			{
				if (!pData->pFile->Open(pData->localFile, wxFile::write))
				{
					LogMessage(::Error, _("Failed to open \"%s\" for writing"), pData->localFile.c_str());
					error = true;
				}
			}

			wxString remotefile = pData->remoteFile;

			if (pData->fileSize != -1)
			{
				pData->totalSize -= pData->fileSize;
			}

			CDirectoryListing listing;
			CDirectoryCache cache;
			bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
			if (found)
			{
				for (unsigned int i = 0; i < listing.m_entryCount; i++)
				{
					if (listing.m_pEntries[i].name == remotefile)
					{
						if (pData->totalSize == -1)
						{
							wxLongLong size = listing.m_pEntries[i].size;
							if (size >= 0)
								pData->totalSize = size;
						}
					}
				}
			}
			
			pData->leftSize = pData->totalSize;
		}
		else
		{
			if (!pData->pFile->Open(pData->localFile, wxFile::read))
			{
				LogMessage(::Error, _("Failed to open \"%s\" for reading"), pData->localFile.c_str());
				error = true;
			}
			
			pData->totalSize = pData->pFile->Length();
			if (pData->resume)
			{
				wxString remotefile = pData->remoteFile;

				if (pData->fileSize != -1)
				{
					pData->leftSize -= pData->fileSize;
					if (pData->leftSize < 0)
						pData->leftSize = 0;
				}

				CDirectoryListing listing;
				CDirectoryCache cache;
				bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
				if (found)
				{
					for (unsigned int i = 0; i < listing.m_entryCount; i++)
					{
						if (listing.m_pEntries[i].name == remotefile)
						{
							wxLongLong size = listing.m_pEntries[i].size;
							if (size >= 0 && pData->leftSize == -1)
								pData->leftSize -= size;
							break;
						}
					}

					wxLongLong offset = pData->totalSize - pData->leftSize;
					// Assume native 64 bit type exists
					if (pData->pFile->Seek(offset.GetValue(), wxFromStart) == wxInvalidOffset)
					{
						LogMessage(::Error, _("Could not seek to offset %s within file"), offset.ToString().c_str());
						error = true;
					}
				}
			}
			else
				pData->leftSize = pData->totalSize;
		}
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
			pData->leftSize = pData->totalSize - pData->pFile->Length();
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
			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
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
			if (!found)
			{
				int res = List();
				if (res != FZ_REPLY_OK)
					return res;

				pData->opState = filetransfer_type;

				res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
			else
			{
				bool differentCase = false;

				for (unsigned int i = 0; i < listing.m_entryCount; i++)
				{
					if (!listing.m_pEntries[i].name.CmpNoCase(pData->remoteFile))
					{
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
		if (pData->tryAbsolutePath)
			cmd += pData->remotePath.GetPath();
		cmd += pData->remoteFile;
		break;
	case filetransfer_mdtm:
		cmd = _T("MDTM ");
		if (pData->tryAbsolutePath)
			cmd += pData->remotePath.GetPath();
		cmd += pData->remoteFile;
		break;
	case filetransfer_type:
		if (pData->binary)
			cmd = _T("TYPE I");
		else
			cmd = _T("TYPE A");
		break;
	case filetransfer_port_pasv:
		m_pTransferSocket = new CTransferSocket(m_pEngine, this, pData->download ? download : upload);
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
	case filetransfer_rest:
		if (!pData->pFile)
		{
			LogMessage(::Debug_Warning, _T("Can't send REST command, can't get local file length since pData->pFile is null"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		cmd = _T("REST ") + ((wxLongLong)pData->pFile->Length()).ToString();
		break;
	case filetransfer_transfer:
		cmd = pData->download ? _T("RETR ") : _T("STOR ");
		if (pData->tryAbsolutePath)
			cmd += pData->remotePath.GetPath();
		cmd += pData->remoteFile;

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
	
	if (reason)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return;
	}

	if (GetCurrentCommandId() == cmd_list)
	{
		if (!m_pCurOpData)
			return;
			
		CListOpData *pData = static_cast<CListOpData *>(m_pCurOpData);
		if (pData->opState < list_list || pData->opState == list_waitlist || pData->opState == list_waitlistpre)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
			ResetOperation(FZ_REPLY_ERROR);
			return;
		}

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
				CDirectoryListing *pListing = m_pTransferSocket->m_pDirectoryListingParser->Parse(m_CurrentPath);

				CDirectoryCache cache;
				cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);
	
				CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(pListing);
				m_pEngine->AddNotification(pNotification);

				ResetOperation(FZ_REPLY_OK);
			}
			break;
		default:
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state"));
			ResetOperation(FZ_REPLY_ERROR);
		}
	}
	else if (GetCurrentCommandId() == cmd_transfer)
	{
		if (!m_pCurOpData)
			return;
			
		CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);
		if (pData->opState < filetransfer_transfer || pData->opState == filetransfer_waittransfer || pData->opState == filetransfer_waittransferpre)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Call to TransferEnd at unusual time"));
			ResetOperation(FZ_REPLY_ERROR);
			return;
		}

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
						pNotification->remoteSize = size;
				}
				if (!pData->fileTime.IsValid())
				{
					if (listing.m_pEntries[i].hasDate)
					{
						pNotification->remoteTime.Set(listing.m_pEntries[i].date.day, (enum wxDateTime::Month)listing.m_pEntries[i].date.month, listing.m_pEntries[i].date.year);
						if (pNotification->remoteTime.IsValid() && listing.m_pEntries[i].hasTime)
						{
							pNotification->remoteTime.SetHour(listing.m_pEntries[i].time.hour);
							pNotification->remoteTime.SetMinute(listing.m_pEntries[i].time.minute);
						}
					}
				}
			}
		}
	}


	pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
	pData->waitAsyncRequest = true;

	m_pEngine->AddNotification(pNotification);

	return true;
}

bool CFtpControlSocket::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	switch (pNotification->GetRequestID())
	{
	case reqId_fileexists:
		{
			if (!m_pCurOpData || m_pCurOpData->opId != cmd_transfer)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No or invalid operation in progress, ignoring request reply"), pNotification->GetRequestID());
				return false;
			}
	
			CFileTransferOpData *pData = static_cast<CFileTransferOpData *>(m_pCurOpData);

			if (!pData->waitAsyncRequest)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Not waiting for request reply, ignoring request reply"), pNotification->GetRequestID());
				return false;
			}

			pData->waitAsyncRequest = false;
			
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
						wxString filename = pData->remotePath.GetPath() + pData->remoteFile;
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
				else if (pData->download && pFileExistsNotification->remoteSize != -1)
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
					wxString filename = pData->remotePath.GetPath() + pData->remoteFile;
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
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown request %d"), pNotification->GetRequestID());
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return false;
	}

	return true;
}
