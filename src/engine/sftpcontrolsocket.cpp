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

int CSftpEvent::GetNumber() const
{
	long number = 0;
	m_text[0].ToLong(&number);
	return number;
}

BEGIN_EVENT_TABLE(CSftpControlSocket, CControlSocket)
EVT_SFTP(CSftpControlSocket::OnSftpEvent)
EVT_END_PROCESS(wxID_ANY, CSftpControlSocket::OnTerminate)
END_EVENT_TABLE();

class CSftpInputThread : public wxThreadEx
{
public:
	CSftpInputThread(CSftpControlSocket* pOwner, wxProcess* pProcess)
		: wxThreadEx(wxTHREAD_JOINABLE), m_pProcess(pProcess),
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
			case sftpRequestPreamble:
			case sftpRequestInstruction:
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
			case sftpRecv:
				m_pOwner->SetActive(true);
				break;
			case sftpSend:
				m_pOwner->SetActive(false);
				break;
			case sftpRead:
			case sftpWrite:
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					CSftpEvent evt((sftpEventTypes)eventType, text);
					wxPostEvent(m_pOwner, evt);
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
	DoClose();
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
		ProcessReply(true, event.GetText());
		break;
	case sftpStatus:
	case sftpError:
	case sftpVerbose:
		wxFAIL_MSG(_T("given notification codes should have been handled by thread"));
		break;
	case sftpDone:
		{
			ProcessReply(event.GetText() == _T("1"));
			break;
		}
	case sftpRequestPreamble:
		m_requestPreamble = event.GetText();
		break;
	case sftpRequestInstruction:
		m_requestInstruction = event.GetText();
		break;
	case sftpRequest:
		switch(event.GetRequestType())
		{
		case sftpReqPassword:
			if (m_pCurrentServer->GetLogonType() == INTERACTIVE)
			{
				CInteractiveLoginNotification *pNotification = new CInteractiveLoginNotification;
				pNotification->server = *m_pCurrentServer;
				if (m_requestPreamble != _T(""))
					pNotification->challenge += m_requestPreamble + _T("\n");
				if (m_requestInstruction != _T(""))
					pNotification->challenge += m_requestInstruction + _T("\n");
				if (event.GetText() != _T("Password:"))
					pNotification->challenge += event.GetText();
				pNotification->requestNumber = m_pEngine->GetNextAsyncRequestNumber();
				m_pEngine->AddNotification(pNotification);
			}
			else
			{
				const wxString pass = m_pCurrentServer->GetPass();
				wxString show = _T("Pass: ");
				show.Append('*', pass.Length());
				Send(pass, show);
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
	case sftpRead:
	case sftpWrite:
		UpdateTransferStatus(event.GetNumber());
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

	CControlSocket::DoClose();

	m_pInputThread->Wait();
	delete m_pInputThread;
	m_pInputThread = 0;
	m_pid = 0;
	delete m_pProcess;
	m_pProcess = 0;
}

bool CSftpControlSocket::Send(wxString cmd, const wxString& show /*=_T("")*/)
{
	if (show != _T(""))
		LogMessage(Command, show);
	else
		LogMessage(Command, cmd);

	// Check for newlines in command
	// a command like "ls\nrm foo/bar" is dangerous
	if (cmd.Find('\n') != -1 ||
		cmd.Find('\r') != -1)
	{
		LogMessage(Debug_Warning, _T("Command containing newline characters, aborting"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return false;
	}

	cmd += _T("\n");

	const wxCharBuffer str = cmd.mb_str();
	if (!m_pProcess)
		return false;

	wxOutputStream* pStream = m_pProcess->GetOutputStream();
	if (!pStream)
		return false;

	unsigned int len = strlen(str);
	if (pStream->Write(str, len).LastWrite() != len)
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
	const enum RequestId requestId = pNotification->GetRequestID();
	switch(requestId)
	{
	case reqId_fileexists:
		{
			if (!m_pCurOpData || m_pCurOpData->opId != cmd_transfer)
			{
				LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("No or invalid operation in progress, ignoring request reply %f"), pNotification->GetRequestID());
				return false;
			}

			CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);

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
					bool found = cache.Lookup(listing, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath);
					if (found)
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
		}
		break;
	case reqId_hostkey:
	case reqId_hostkeyChanged:
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
		}
		break;
	case reqId_interactiveLogin:
		{
			CInteractiveLoginNotification *pInteractiveLoginNotification = reinterpret_cast<CInteractiveLoginNotification *>(pNotification);
		
			if (!pInteractiveLoginNotification->passwordSet)
			{
				DoClose(FZ_REPLY_CANCELED);
				return false;
			}
			const wxString pass = pInteractiveLoginNotification->server.GetPass();
			m_pCurrentServer->SetUser(m_pCurrentServer->GetUser(), pass);
			wxString show = _T("Pass: ");
			show.Append('*', pass.Length());
			Send(pass, show);
		}
		break;
	}

	return true;
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

	if (!m_pCurrentServer)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurrenServer == 0"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
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

int CSftpControlSocket::ListParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ListParseResponse(%s)"), reply.c_str());

	if (!successful)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

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
		pData->pParser = new CDirectoryListingParser(m_pEngine, this, m_pCurrentServer->GetType());
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

int CSftpControlSocket::ChangeDirParseResponse(bool successful, const wxString& reply)
{
	if (!successful || !m_pCurOpData)
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
		cmd = _T("cd ") + QuoteFilename(pData->path.GetPath());
		m_CurrentPath.Clear();
		break;
	case cwd_cwd_subdir:
		if (pData->subDir == _T(""))
		{
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
		else
			cmd = _T("cd ") + QuoteFilename(pData->subDir);
		m_CurrentPath.Clear();
		break;
	}

	if (cmd != _T(""))
		if (!Send(cmd))
			return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::ProcessReply(bool successful, const wxString& reply /*=_T("")*/)
{
	enum Command commandId = GetCurrentCommandId();
	switch (commandId)
	{
	case cmd_connect:
		if (successful)
			return ResetOperation(FZ_REPLY_OK);
		else
			return ResetOperation(FZ_REPLY_ERROR);
	case cmd_list:
		return ListParseResponse(successful, reply);
	case cmd_transfer:
		return FileTransferParseResponse(successful, reply);
	case cmd_private:
		return ChangeDirParseResponse(successful, reply);
	case cmd_mkdir:
		return MkdirParseResponse(successful, reply);
	case cmd_delete:
		return DeleteParseResponse(successful, reply);
	case cmd_removedir:
		return RemoveDirParseResponse(successful, reply);
	case cmd_chmod:
		return ChmodParseResponse(successful, reply);
	case cmd_rename:
		return RenameParseResponse(successful, reply);
	default:
		LogMessage(Debug_Warning, _T("No action for parsing replies to command %d"), (int)commandId);
		return ResetOperation(FZ_REPLY_INTERNALERROR);
	}
}

int CSftpControlSocket::ResetOperation(int nErrorCode)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ResetOperation(%d)"), nErrorCode);

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
	case cmd_transfer:
		return FileTransferSend(prevResult);
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
	{
		InitTransferStatus(pData->remoteFileSize, pData->resume ? pData->localFileSize : 0);
		cmd += _T("get ");
		cmd += QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath)) + _T(" ");
		cmd += QuoteFilename(pData->localFile);
	}
	else
	{
		InitTransferStatus(pData->localFileSize, pData->resume ? pData->remoteFileSize : 0);
		cmd += _T("put ");
		cmd += QuoteFilename(pData->localFile) + _T(" ");
		cmd += QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath));
	}
	SetTransferStatusStartTime();

	pData->transferInitiated = true;
	if (!Send(cmd))
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::FileTransferParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("FileTransferParseResponse()"));

	if (!successful)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);

	if (pData->opState != filetransfer_transfer)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("  Called at improper time: opState == %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CSftpControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	if (m_pInputThread)
	{
		wxThreadEx* pThread = m_pInputThread;
		m_pInputThread = 0;
		wxProcess::Kill(m_pid, wxSIGKILL);
		m_inDestructor = true;
		if (pThread)
		{
			pThread->Wait();
			delete pThread;
		}
		if (!m_termindatedInDestructor)
			m_pProcess->Detach();
		else
		{
			delete m_pProcess;
			m_pProcess = 0;
		}
	}
	return CControlSocket::DoClose(nErrorCode);
}

void CSftpControlSocket::Cancel()
{
	if (GetCurrentCommandId() != cmd_none)
	{
		DoClose(FZ_REPLY_CANCELED);
	}
}

void CSftpControlSocket::SetActive(bool recv)
{
	m_pEngine->SetActive(recv);
}

enum mkdStates
{
	mkd_init = 0,
	mkd_findparent,
	mkd_mkdsub,
	mkd_cwdsub,
	mkd_tryfull
};

class CSftpMkdirOpData : public COpData
{
public:
	CSftpMkdirOpData()
	{
		opId = cmd_mkdir;
	}

	virtual ~CSftpMkdirOpData()
	{
	}

	CServerPath path;
	CServerPath currentPath;
	std::list<wxString> segments;
};

int CSftpControlSocket::Mkdir(const CServerPath& path, CServerPath start /*=CServerPath()*/)
{
	/* Directory creation works like this: First find a parent directory into
	 * which we can CWD, then create the subdirs one by one. If either part 
	 * fails, try MKD with the full path directly.
	 */

	if (!m_pCurOpData)
		LogMessage(Status, _("Creating directory '%s'..."), path.GetPath().c_str());

	CSftpMkdirOpData *pData = new CSftpMkdirOpData;
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

int CSftpControlSocket::MkdirParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::MkdirParseResonse"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpMkdirOpData *pData = static_cast<CSftpMkdirOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	bool error = false;
	switch (pData->opState)
	{
	case mkd_findparent:
		if (successful)
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
		if (successful)
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
		if (successful)
		{
			m_CurrentPath = pData->currentPath;
			pData->opState = mkd_mkdsub;
		}
		else
			pData->opState = mkd_tryfull;
		break;
	case mkd_tryfull:
		if (!successful)
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

int CSftpControlSocket::MkdirSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::MkdirSend"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpMkdirOpData *pData = static_cast<CSftpMkdirOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	bool res;
	switch (pData->opState)
	{
	case mkd_findparent:
	case mkd_cwdsub:
		m_CurrentPath.Clear();
		res = Send(_T("cd ") + QuoteFilename(pData->currentPath.GetPath()));
		break;
	case mkd_mkdsub:
		res = Send(_T("mkdir ") + QuoteFilename(pData->segments.front()));
		break;
	case mkd_tryfull:
		res = Send(_T("mkdir ") + QuoteFilename(pData->path.GetPath()));
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

wxString CSftpControlSocket::QuoteFilename(wxString filename)
{
	filename.Replace(_T("\""), _T("\"\""));
	return _T("\"") + filename + _T("\"");
}

class CSftpDeleteOpData : public COpData
	{
	public:
		CSftpDeleteOpData() { opId = cmd_delete; }
		virtual ~CSftpDeleteOpData() { }

		CServerPath path;
		wxString file;
	};

int CSftpControlSocket::Delete(const CServerPath& path /*=CServerPath()*/, const wxString& file /*=_T("")*/)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::Delete"));
	wxASSERT(!m_pCurOpData);
	CSftpDeleteOpData *pData = new CSftpDeleteOpData();
	m_pCurOpData = pData;
	pData->path = path;
	pData->file = file;

	wxString filename = path.FormatFilename(file);
	if (filename == _T(""))
	{
		LogMessage(::Error, wxString::Format(_T("Filename cannot be constructed for folder %s and filename %s"), path.GetPath().c_str(), file.c_str()));
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, path, file, false);

	if (!Send(_T("rm ") + filename))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::DeleteParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::DeleteParseResponse"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!successful)
		return ResetOperation(FZ_REPLY_ERROR);

	CSftpDeleteOpData *pData = static_cast<CSftpDeleteOpData *>(m_pCurOpData);

	CDirectoryCache cache;
	cache.RemoveFile(*m_pCurrentServer, pData->path, pData->file);
	m_pEngine->ResendModifiedListings();

	return ResetOperation(FZ_REPLY_OK);
}

class CSftpRemoveDirOpData : public COpData
{
public:
	CSftpRemoveDirOpData() { opId = cmd_removedir; }
	virtual ~CSftpRemoveDirOpData() { }

	CServerPath path;
	wxString subDir;
};

int CSftpControlSocket::RemoveDir(const CServerPath& path /*=CServerPath()*/, const wxString& subDir /*=_T("")*/)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::RemoveDir"));

	wxASSERT(!m_pCurOpData);
	CSftpRemoveDirOpData *pData = new CSftpRemoveDirOpData();
	m_pCurOpData = pData;
	pData->path = path;
	pData->subDir = subDir;

	CServerPath fullPath = pData->path;
		
	if (!fullPath.AddSegment(subDir))
	{
		LogMessage(::Error, wxString::Format(_T("Path cannot be constructed for folder %s and subdir %s"), path.GetPath().c_str(), subDir.c_str()));
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, path, subDir, false, CDirectoryCache::dir);

	if (!Send(_T("rmdir ") + QuoteFilename(fullPath.GetPath())))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::RemoveDirParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::RemoveDirParseResponse"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!successful)
		ResetOperation(FZ_REPLY_ERROR);

	CSftpRemoveDirOpData *pData = static_cast<CSftpRemoveDirOpData *>(m_pCurOpData);
	CDirectoryCache cache;
	cache.RemoveDir(*m_pCurrentServer, pData->path, pData->subDir);
	m_pEngine->ResendModifiedListings();

	return ResetOperation(FZ_REPLY_OK);
}

class CSftpChmodOpData : public COpData
{
public:
	CSftpChmodOpData(const CChmodCommand& command)
		: m_cmd(command)
	{
		opId = cmd_chmod;
		m_useAbsolute = false;
	}

	virtual ~CSftpChmodOpData() { }

	CChmodCommand m_cmd;
	bool m_useAbsolute;
};

enum chmodStates
{
	chmod_init = 0,
	chmod_chmod
};

int CSftpControlSocket::Chmod(const CChmodCommand& command)
{
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData not empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	LogMessage(Status, _("Set permissions of '%s' to '%s'"), command.GetPath().FormatFilename(command.GetFile()).c_str(), command.GetPermission().c_str());

	CSftpChmodOpData *pData = new CSftpChmodOpData(command);
	pData->opState = chmod_chmod;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetPath());
	if (res != FZ_REPLY_OK)
		return res;
	
	return ChmodSend();
}

int CSftpControlSocket::ChmodParseResponse(bool successful, const wxString& reply)
{
	CSftpChmodOpData *pData = static_cast<CSftpChmodOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!successful)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CSftpControlSocket::ChmodSend(int prevResult /*=FZ_REPLY_OK*/)
{
	CSftpChmodOpData *pData = static_cast<CSftpChmodOpData*>(m_pCurOpData);
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
		{
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetPath(), pData->m_cmd.GetFile(), false, CDirectoryCache::unknown);
		}
		res = Send(_T("chmod ") + pData->m_cmd.GetPermission() + _T(" ") + QuoteFilename(pData->m_cmd.GetPath().FormatFilename(pData->m_cmd.GetFile(), !pData->m_useAbsolute)));
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

class CSftpRenameOpData : public COpData
{
public:
	CSftpRenameOpData(const CRenameCommand& command)
		: m_cmd(command)
	{
		opId = cmd_rename;
		m_useAbsolute = false;
	}

	virtual ~CSftpRenameOpData() { }

	CRenameCommand m_cmd;
	bool m_useAbsolute;
};

enum renameStates
{
	rename_init = 0,
	rename_rename
};

int CSftpControlSocket::Rename(const CRenameCommand& command)
{
	if (m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData not empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	LogMessage(Status, _("Renaming '%s' to '%s'"), command.GetFromPath().FormatFilename(command.GetFromFile()).c_str(), command.GetToPath().FormatFilename(command.GetToFile()).c_str());

	CSftpRenameOpData *pData = new CSftpRenameOpData(command);
	pData->opState = rename_rename;
	m_pCurOpData = pData;

	int res = ChangeDir(command.GetFromPath());
	if (res != FZ_REPLY_OK)
		return res;
	
	return RenameSend();
}

int CSftpControlSocket::RenameParseResponse(bool successful, const wxString& reply)
{
	CSftpRenameOpData *pData = static_cast<CSftpRenameOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!successful)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.Rename(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile(), pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile());

	m_pEngine->ResendModifiedListings();

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CSftpControlSocket::RenameSend(int prevResult /*=FZ_REPLY_OK*/)
{
	CSftpRenameOpData *pData = static_cast<CSftpRenameOpData*>(m_pCurOpData);
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
	case rename_rename:
		{
			CDirectoryCache cache;
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile(), false);
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile(), false);
		}

		res = Send(_T("mv ") + QuoteFilename(pData->m_cmd.GetFromPath().FormatFilename(pData->m_cmd.GetFromFile(), !pData->m_useAbsolute))
					+ _T(" ")
					+ QuoteFilename(pData->m_cmd.GetToPath().FormatFilename(pData->m_cmd.GetToFile(), !pData->m_useAbsolute && pData->m_cmd.GetFromPath() == pData->m_cmd.GetToPath())));
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
