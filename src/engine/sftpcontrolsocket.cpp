#include "FileZilla.h"
#include "sftpcontrolsocket.h"
#include <wx/process.h>
#include <wx/txtstrm.h>
#include "directorycache.h"
#include "directorylistingparser.h"

class CSftpFileTransferOpData : public CFileTransferOpData
{
public:
	CSftpFileTransferOpData()
	{
	}

	CFileTransferCommand::t_transferSettings transferSettings;
};

enum filetransferStates
{
	filetransfer_init = 0,
	filetransfer_waitcwd,
	filetransfer_waitlist,
	filetransfer_transfer
};

extern const wxEventType fzEVT_SFTP;
typedef void (wxEvtHandler::*fzSftpEventFunction)(CSftpEvent&);

#define EVT_SFTP(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
		fzEVT_SFTP, -1, -1, \
		(wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( fzSftpEventFunction, &fn ), \
		(wxObject *) NULL \
	),

DEFINE_EVENT_TYPE(fzEVT_SFTP);

CSftpEvent::CSftpEvent(sftpEventTypes type, const wxString& text)
	: wxEvent(wxID_ANY, fzEVT_SFTP)
{
	m_type = type;
	m_text[0] = text;
	m_reqType = sftpReqUnknown;
}

CSftpEvent::CSftpEvent(sftpEventTypes type, sftpRequestTypes reqType, const wxString& text1, const wxString& text2 /*=_T("")*/, const wxString& text3 /*=_T("")*/, const wxString& text4 /*=_T("")*/)
	: wxEvent(wxID_ANY, fzEVT_SFTP)
{
	wxASSERT(type == sftpRequest || reqType == sftpReqUnknown);
	m_type = type;

	m_reqType = reqType;

	m_text[0] = text1;
	m_text[1] = text2;
	m_text[2] = text3;
	m_text[3] = text4;
}

BEGIN_EVENT_TABLE(CSftpControlSocket, CControlSocket)
EVT_SFTP(CSftpControlSocket::OnSftpEvent)
EVT_END_PROCESS(wxID_ANY, CSftpControlSocket::OnTerminate)
END_EVENT_TABLE();

class CSftpInputThread : public wxThread
{
public:
	CSftpInputThread(CSftpControlSocket* pOwner, wxProcess* pProcess)
		: wxThread(wxTHREAD_JOINABLE), m_pProcess(pProcess),
		  m_pOwner(pOwner)
	{

	}

	virtual ~CSftpInputThread()
	{
	}

	bool Init()
	{
		if (Create() != wxTHREAD_NO_ERROR)
			return false;

		Run();

		return true;
	}
	
protected:
	virtual ExitCode Entry()
	{
		wxInputStream* pInputStream = m_pProcess->GetInputStream();
		char eventType;

		while(!pInputStream->Eof())
		{
			pInputStream->Read(&eventType, 1);
			if (pInputStream->LastRead() != 1)
				break;

			eventType -= '0';
			
			switch(eventType)
			{
			case sftpReply:
			case sftpListentry:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					CSftpEvent evt((sftpEventTypes)eventType, text);
					wxPostEvent(m_pOwner, evt);
				}
				break;
			case sftpError:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					m_pOwner->LogMessage(::Error, text);
				}
				break;
			case sftpVerbose:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					m_pOwner->LogMessage(Debug_Info, text);
				}
				break;
			case sftpStatus:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					m_pOwner->LogMessage(Status, text);
				}
				break;
			case sftpDone:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					CSftpEvent evt((sftpEventTypes)eventType, text);
					wxPostEvent(m_pOwner, evt);
				}
				break;
			case sftpRequest:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					int requestType = text[0] - '0';
					if (requestType == sftpReqHostkey || requestType == sftpReqHostkeyChanged)
					{
						wxString strPort = textStream.ReadLine();
						if (pInputStream->LastRead() <= 0 || strPort == _T(""))
							goto loopexit;
						long port = 0;
						if (!strPort.ToLong(&port))
							goto loopexit;
						wxString fingerprint = textStream.ReadLine();
						if (pInputStream->LastRead() <= 0 || fingerprint == _T(""))
							goto loopexit;

						m_pOwner->SendRequest(new CHostKeyNotification(text.Mid(1), port, fingerprint, requestType == sftpReqHostkeyChanged));
					}
					else if (requestType == sftpReqPassword)
					{
						CSftpEvent evt(sftpRequest, sftpReqPassword, text.Mid(1));
						wxPostEvent(m_pOwner, evt);
					}
				}
				break;
			default:
				{
					char tmp[2];
					tmp[0] = eventType + '0';
					tmp[1] = 0;
					m_pOwner->LogMessage(Debug_Info, _T("Unknown eventType: %s"), tmp);
				}
				break;
			}
		}
loopexit:

		return (ExitCode)Close();
	};

	int Close()
	{
		return 0;
	}

	wxProcess* m_pProcess;
	CSftpControlSocket* m_pOwner;
};

CSftpControlSocket::CSftpControlSocket(CFileZillaEnginePrivate *pEngine) : CControlSocket(pEngine)
{
	m_pProcess = 0;
	m_pInputThread = 0;
	m_pid = 0;
	m_inDestructor = false;
	m_termindatedInDestructor = false;
}

CSftpControlSocket::~CSftpControlSocket()
{
	if (m_pInputThread)
	{
		wxProcess::Kill(m_pid, wxSIGKILL);
		m_inDestructor = true;
		if (m_pInputThread)
		{
			m_pInputThread->Wait();
			delete m_pInputThread;
		}
		if (!m_termindatedInDestructor)
			m_pProcess->Detach();
		else
			delete m_pProcess;
	}
}

int CSftpControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());

	if (m_pCurrentServer)
		delete m_pCurrentServer;
	m_pCurrentServer = new CServer(server);

	m_pProcess = new wxProcess(this);
	m_pProcess->Redirect();

	m_pid = wxExecute(_T("fzsftp -v"), wxEXEC_ASYNC, m_pProcess);
	if (!m_pid)
	{
		delete m_pProcess;
		m_pProcess = 0;
		LogMessage(::Error, _("fzsftp could not be started"));
		return FZ_REPLY_ERROR;
	}
	
	m_pInputThread = new CSftpInputThread(this, m_pProcess);
	if (!m_pInputThread->Init())
	{
		delete m_pInputThread;
		m_pInputThread = 0;
		m_pProcess->Detach();
		m_pProcess = 0;
		return FZ_REPLY_ERROR;
	}

	wxString str;
	Send(wxString::Format(_T("open %s@%s %d"), server.GetUser().c_str(), server.GetHost().c_str(), server.GetPort()));

	return FZ_REPLY_WOULDBLOCK;
}

void CSftpControlSocket::OnSftpEvent(CSftpEvent& event)
{
	if (!m_pCurrentServer)
		return;

	switch (event.GetType())
	{
	case sftpReply:
		LogMessage(Response, event.GetText());
		ProcessReply(event.GetText());
		break;
	case sftpStatus:
	case sftpError:
	case sftpVerbose:
		wxFAIL_MSG(_T("given notification codes should have been handled by thread"));
		break;
	case sftpDone:
		{
			bool successful = event.GetText() == _T("1");
			if (successful)
				ProcessReply(_T(""));
			else
				ResetOperation(FZ_REPLY_ERROR);
			break;
		}
	case sftpRequest:
		switch(event.GetRequestType())
		{
		case sftpReqPassword:
			if (m_pCurrentServer->GetLogonType() == INTERACTIVE)
			{
				CInteractiveLoginNotification *pNotification = new CInteractiveLoginNotification;
				pNotification->server = *m_pCurrentServer;
				pNotification->challenge = event.GetText();
				pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
				m_pEngine->AddNotification(pNotification);
			}
			else
			{
				Send(m_pCurrentServer->GetPass());
			}
			break;
		default:
			wxFAIL_MSG(_T("given notification codes should have been handled by thread"));
			break;
		}
		break;
	case sftpListentry:
		ListParseEntry(event.GetText());
		break;
	default:
		wxFAIL_MSG(_T("given notification codes not handled"));
		break;
	}
}

void CSftpControlSocket::OnTerminate(wxProcessEvent& event)
{
	// Check if we're inside the destructor, if so, return, all cleanup will be
	// done there.
	if (m_inDestructor)
	{
		m_termindatedInDestructor = true;
		return;
	}

	if (!m_pInputThread)
	{
		event.Skip();
		return;
	}

	m_pInputThread->Wait();
	delete m_pInputThread;
	m_pInputThread = 0;
	m_pid = 0;
	delete m_pProcess;
	m_pProcess = 0;
}

bool CSftpControlSocket::Send(const wxString& cmd)
{
	LogMessage(Command, cmd);
	const char* str = cmd.mb_str();
	if (!m_pProcess)
		return false;

	wxOutputStream* pStream = m_pProcess->GetOutputStream();
	if (!pStream)
		return false;

	unsigned int len = strlen(str);
	if (pStream->Write(str, len).LastWrite() != len)
		return false;

	if (pStream->Write("\n", 1).LastWrite() != 1)
		return false;

	return true;
}

bool CSftpControlSocket::SendRequest(CAsyncRequestNotification *pNotification)
{
	pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
	m_pEngine->AddNotification(pNotification);

	return true;
}

bool CSftpControlSocket::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	if (pNotification->GetRequestID() == reqId_hostkey || pNotification->GetRequestID() == reqId_hostkeyChanged)
	{
		if (GetCurrentCommandId() != cmd_connect ||
			!m_pCurrentServer)
		{
			LogMessage(Debug_Info, _T("SetAsyncRequestReply called to wrong time"));
			return false;
		}

		CHostKeyNotification *pHostKeyNotification = reinterpret_cast<CHostKeyNotification *>(pNotification);
		if (!pHostKeyNotification->m_trust)
			Send(_T(""));
		else if (pHostKeyNotification->m_alwaysTrust)
			Send(_T("y"));
		else
			Send(_T("n"));

		return true;
	}
	else if (pNotification->GetRequestID() == reqId_interactiveLogin)
	{
		CInteractiveLoginNotification *pInteractiveLoginNotification = reinterpret_cast<CInteractiveLoginNotification *>(pNotification);
		
		m_pCurrentServer->SetUser(m_pCurrentServer->GetUser(), pInteractiveLoginNotification->server.GetPass());
		Send(m_pCurrentServer->GetPass());
	}

	return false;
}

class CSftpListOpData : public COpData
{
public:
	CSftpListOpData()
	{
		opId = cmd_list;

		transferEndReason = 0;
		pParser = 0;
	}

	virtual ~CSftpListOpData()
	{
		delete pParser;
	}

	CDirectoryListingParser* pParser;

	CServerPath path;
	wxString subDir;

	// Set to true to get a directory listing even if a cache
	// lookup can be made after finding out true remote directory
	bool refresh;

	int transferEndReason;
};

enum listStates
{
	list_init = 0,
	list_waitcwd,
	list_list
};


int CSftpControlSocket::List(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/, bool refresh /*=false*/)
{
	LogMessage(Status, _("Retrieving directory listing..."));

	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("List called from other command"));
	}
	CSftpListOpData *pData = new CSftpListOpData;
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

	pData->opState = list_list;

	return ListSend();
}

int CSftpControlSocket::ListParseResponse(const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ListParseResponse(%s)"), reply.c_str());

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData of wrong type"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (pData->opState != list_list)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("ListParseResponse called at inproper time: %s"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!pData->pParser)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("pData->pParser is 0"));
		return FZ_REPLY_INTERNALERROR;
	}

	CDirectoryListing *pListing = pData->pParser->Parse(m_CurrentPath);

	CDirectoryCache cache;
	cache.Store(*pListing, *m_pCurrentServer, pData->path, pData->subDir);

	SendDirectoryListing(pListing);
	m_pEngine->ResendModifiedListings();

	ResetOperation(FZ_REPLY_OK);

	return FZ_REPLY_ERROR;
}

int CSftpControlSocket::ListParseEntry(const wxString& entry)
{
	LogMessage(RawList, entry);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("m_pCurOpData of wrong type"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (pData->opState != list_list)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("ListParseResponse called at inproper time: %s"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}


	if (!pData->pParser)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("pData->pParser is 0"));
		return FZ_REPLY_INTERNALERROR;
	}

	wxCharBuffer str = entry.mb_str();
	int len = strlen(str);
	char* buffer = new char[len + 1];
	strcpy(buffer, str);
	buffer[len] = '\n';
	pData->pParser->AddData(buffer, len + 1);

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::ListSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ListSend(%d)"), prevResult);

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
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

		pData->opState = list_list;
	}

	if (pData->opState == list_list)
	{
		pData->pParser = new CDirectoryListingParser(m_pEngine, m_pCurrentServer->GetType());
		Send(_T("ls"));
		return FZ_REPLY_WOULDBLOCK;
	}
	return FZ_REPLY_ERROR;
}

class CSftpChangeDirOpData : public COpData
{
public:
	CSftpChangeDirOpData()
	{
		opId = cmd_private;
		triedMkd = false;
	}

	virtual ~CSftpChangeDirOpData()
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
	cwd_cwd_subdir
};

int CSftpControlSocket::ChangeDir(CServerPath path /*=CServerPath()*/, wxString subDir /*=_T("")*/)
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

	CSftpChangeDirOpData *pData = new CSftpChangeDirOpData;
	pData->pNextOpData = m_pCurOpData;
	pData->opState = state;
	pData->path = path;
	pData->subDir = subDir;

	m_pCurOpData = pData;

	return ChangeDirSend();
}

int CSftpControlSocket::ChangeDirParseResponse(const wxString& reply)
{
	if (!m_pCurOpData)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
	CSftpChangeDirOpData *pData = static_cast<CSftpChangeDirOpData *>(m_pCurOpData);

	bool error = false;
	switch (pData->opState)
	{
	case cwd_pwd:
		if (reply == _T(""))
			error = true;
		if (ParsePwdReply(reply))
		{
			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else
			error = true;
		break;
	case cwd_cwd:
		if (reply == _T(""))
		{
			// Create remote directory if part of a file upload
			if (pData->pNextOpData && pData->pNextOpData->opId == cmd_transfer && 
				!static_cast<CSftpFileTransferOpData *>(pData->pNextOpData)->download && !pData->triedMkd)
			{
				pData->triedMkd = true;
				int res = Mkdir(pData->path, pData->path);
				if (res != FZ_REPLY_OK)
					return res;
			}
			else
				error = true;
		}
		else if (ParsePwdReply(reply))
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
		if (reply == _T(""))
			error = true;
		else if (ParsePwdReply(reply))
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

int CSftpControlSocket::ChangeDirSend()
{
	if (!m_pCurOpData)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}
	CSftpChangeDirOpData *pData = static_cast<CSftpChangeDirOpData *>(m_pCurOpData);

	wxString cmd;
	switch (pData->opState)
	{
	case cwd_pwd:
		cmd = _T("pwd");
		break;
	case cwd_cwd:
		cmd = _T("cd ") + pData->path.GetPath();
		m_CurrentPath.Clear();
		break;
	case cwd_cwd_subdir:
		if (pData->subDir == _T(""))
		{
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		else
			cmd = _T("cd ") + pData->subDir;
		m_CurrentPath.Clear();
		break;
	}

	if (cmd != _T(""))
		if (!Send(cmd))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::ProcessReply(const wxString& reply)
{
	enum Command commandId = GetCurrentCommandId();
	switch (commandId)
	{
	case cmd_connect:
		return ResetOperation(FZ_REPLY_OK);
	case cmd_list:
		return ListParseResponse(reply);
		break;
	case cmd_private:
		return ChangeDirParseResponse(reply);
		break;
	default:
		LogMessage(Debug_Warning, _T("No action for parsing replies to command %d"), (int)commandId);
		return ResetOperation(FZ_REPLY_INTERNALERROR);
	}
}

int CSftpControlSocket::ResetOperation(int nErrorCode)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ResetOperation(%d)"), nErrorCode);

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
		CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);
		if (!pData->download && pData->opState >= filetransfer_transfer)
		{
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->remotePath, pData->remoteFile, CDirectoryCache::file, (nErrorCode == FZ_REPLY_OK) ? pData->remoteFileSize : -1);

			m_pEngine->ResendModifiedListings();
		}
	}

	return CControlSocket::ResetOperation(nErrorCode);
}

int CSftpControlSocket::SendNextCommand(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::SendNextCommand(%d)"), prevResult);
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
	case cmd_private:
		return ChangeDirSend();
	default:
		LogMessage(::Debug_Warning, __TFILE__, __LINE__, _T("Unknown opID (%d) in SendNextCommand"), m_pCurOpData->opId);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}

	return FZ_REPLY_ERROR;
}

int CSftpControlSocket::FileTransfer(const wxString localFile, const CServerPath &remotePath,
									const wxString &remoteFile, bool download,
									const CFileTransferCommand::t_transferSettings& transferSettings)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::FileTransfer(...)"));

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

	CSftpFileTransferOpData *pData = new CSftpFileTransferOpData;
	m_pCurOpData = pData;

	pData->localFile = localFile;
	pData->remotePath = remotePath;
	pData->remoteFile = remoteFile;
	pData->download = download;
	pData->transferSettings = transferSettings;

	pData->opState = filetransfer_waitcwd;

	if (pData->remotePath.GetType() == DEFAULT)
		pData->remotePath.SetType(m_pCurrentServer->GetType());

	wxStructStat buf;
	int result;
	result = wxStat(pData->localFile, &buf);
	if (!result)
		pData->localFileSize = buf.st_size;

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
					wxLongLong size = listing.m_pEntries[i].size;	
					pData->remoteFileSize = size.GetLo() + ((wxFileOffset)size.GetHi() << 32);
					differentCase = false;
					break;
				}
			}
		}
		if (!shouldList)
		{
			pData->opState = filetransfer_transfer;
			res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
		}
	}
	if (shouldList)
	{
		res = List();
		if (res != FZ_REPLY_OK)
			return res;

		pData->opState = filetransfer_transfer;

		res = CheckOverwriteFile();
		if (res != FZ_REPLY_OK)
			return res;
	}

	return FileTransferSend();
}

int CSftpControlSocket::FileTransferSend(int prevResult /*=FZ_REPLY_OK*/)
{
	LogMessage(Debug_Verbose, _T("FileTransferSend()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);

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
							wxLongLong size = listing.m_pEntries[i].size;	
							pData->remoteFileSize = size.GetLo() + ((wxFileOffset)size.GetHi() << 32);
							differentCase = false;
							break;
						}
					}
				}
				if (!shouldList)
				{
					pData->opState = filetransfer_transfer;
					int res = CheckOverwriteFile();
					if (res != FZ_REPLY_OK)
						return res;
				}
			}
			if (shouldList)
			{
				int res = List();
				if (res != FZ_REPLY_OK)
					return res;

				pData->opState = filetransfer_transfer;

				res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
		}
		else if (prevResult == FZ_REPLY_ERROR)
		{
			pData->tryAbsolutePath = true;
			pData->opState = filetransfer_transfer;

			int res = CheckOverwriteFile();
			if (res != FZ_REPLY_OK)
				return res;
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
			if (found)
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
			}
		}
		else if (prevResult != FZ_REPLY_ERROR)
		{
			ResetOperation(prevResult);
			return FZ_REPLY_ERROR;
		}

		pData->opState = filetransfer_transfer;

		int res = CheckOverwriteFile();
		if (res != FZ_REPLY_OK)
			return res;
	}

	wxString cmd;
	if (pData->resume)
		cmd = _T("re");
	if (pData->download)
		cmd += _T("get ");
	else
		cmd += _T("put");

	cmd += pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath);
	cmd += _T(" ") + pData->localFile;

	if (!Send(cmd))
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}
