#include "FileZilla.h"
#include "sftpcontrolsocket.h"
#include <wx/process.h>
#include <wx/txtstrm.h>
#include "directorycache.h"
#include "directorylistingparser.h"
#include "pathcache.h"
#include "servercapabilities.h"
#include <wx/tokenzr.h>
#include "local_filesys.h"
#include "proxy.h"

class CSftpFileTransferOpData : public CFileTransferOpData
{
public:
	CSftpFileTransferOpData()
	{
	}
};

enum filetransferStates
{
	filetransfer_init = 0,
	filetransfer_waitcwd,
	filetransfer_waitlist,
	filetransfer_mtime,
	filetransfer_transfer,
	filetransfer_chmtime
};

struct sftp_message
{
	sftpEventTypes type;
	wxString text;
	sftpRequestTypes reqType;
};

DECLARE_EVENT_TYPE(fzEVT_SFTP, -1)
DEFINE_EVENT_TYPE(fzEVT_SFTP)

BEGIN_EVENT_TABLE(CSftpControlSocket, CControlSocket)
EVT_COMMAND(wxID_ANY, fzEVT_SFTP, CSftpControlSocket::OnSftpEvent)
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
		m_criticalSection.Enter();
		for (std::list<sftp_message*>::iterator iter = m_sftpMessages.begin(); iter != m_sftpMessages.end(); iter++)
			delete *iter;
		m_criticalSection.Leave();
	}

	bool Init()
	{
		if (Create() != wxTHREAD_NO_ERROR)
			return false;

		Run();

		return true;
	}

	void GetMessages(std::list<sftp_message*>& messages)
	{
		m_criticalSection.Enter();
		messages.swap(m_sftpMessages);
		m_criticalSection.Leave();
	}

protected:

	void SendMessage(sftp_message* message)
	{
		bool sendEvent;

		m_criticalSection.Enter();
		sendEvent = m_sftpMessages.empty();
		m_sftpMessages.push_back(message);
		m_criticalSection.Leave();

		wxCommandEvent evt(fzEVT_SFTP, wxID_ANY);
		if (sendEvent)
			wxPostEvent(m_pOwner, evt);
	}

	wxString ReadLine(wxInputStream* pInputStream, bool &error)
	{
		int read = 0;
		const int buffersize = 4096;
		char buffer[buffersize];

		while(!pInputStream->Eof())
		{
			char c;
			pInputStream->Read(&c, 1);
			if (pInputStream->LastRead() != 1)
			{
				if (pInputStream->Eof())
					m_pOwner->LogMessage(Debug_Warning, _T("Unexpected EOF."));
				else
					m_pOwner->LogMessage(Debug_Warning, _T("Uknown input stream error"));
				error = true;
				return _T("");
			}

			if (c == '\n')
				break;

			if (read == buffersize - 1)
			{
				// Cap string length
				continue;
			}

			buffer[read++] = c;
		}
		if (pInputStream->Eof())
		{
			m_pOwner->LogMessage(Debug_Warning, _T("Unexpected EOF."));
			error = true;
			return _T("");
		}

		if (read && buffer[read - 1] == '\r')
			read--;

		buffer[read] = 0;

		const wxString line = m_pOwner->ConvToLocal(buffer);
		if (read && line == _T(""))
		{
			m_pOwner->LogMessage(::Error, _T("Failed to convert reply to local character set."));
			error = true;
		}

		return line;
	}

	virtual ExitCode Entry()
	{
		wxInputStream* pInputStream = m_pProcess->GetInputStream();
		char eventType;

		bool error = false;
		while (!pInputStream->Eof() && !error)
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
			case sftpDone:
			case sftpError:
			case sftpVerbose:
			case sftpStatus:
			case sftpKexAlgorithm:
			case sftpKexHash:
			case sftpCipherClientToServer:
			case sftpCipherServerToClient:
			case sftpMacClientToServer:
			case sftpMacServerToClient:
			case sftpHostkey:
				{
					sftp_message* message = new sftp_message;
					message->type = (sftpEventTypes)eventType;
					message->text = ReadLine(pInputStream, error).c_str();
					if (error)
					{
						delete message;
						goto loopexit;
					}
					SendMessage(message);
				}
				break;
			case sftpRequest:
				{
					const wxString& line = ReadLine(pInputStream, error);
					if (error)
						goto loopexit;
					int requestType = line[0] - '0';
					if (requestType == sftpReqHostkey || requestType == sftpReqHostkeyChanged)
					{
						const wxString& strPort = ReadLine(pInputStream, error);
						if (error)
							goto loopexit;
						long port = 0;
						if (!strPort.ToLong(&port))
							goto loopexit;
						const wxString& fingerprint = ReadLine(pInputStream, error);
						if (error)
							goto loopexit;

						m_pOwner->SendAsyncRequest(new CHostKeyNotification(line.Mid(1), port, fingerprint, requestType == sftpReqHostkeyChanged));
					}
					else if (requestType == sftpReqPassword)
					{
						sftp_message* message = new sftp_message;
						message->type = (sftpEventTypes)eventType;
						message->reqType = sftpReqPassword;
						message->text = line.Mid(1).c_str();
						SendMessage(message);
					}
				}
				break;
			case sftpRecv:
			case sftpSend:
			case sftpUsedQuotaRecv:
			case sftpUsedQuotaSend:
				{
					sftp_message* message = new sftp_message;
					message->type = (sftpEventTypes)eventType;
					SendMessage(message);
				}
				break;
			case sftpRead:
			case sftpWrite:
				{
					sftp_message* message = new sftp_message;
					message->type = (sftpEventTypes)eventType;
					message->text = ReadLine(pInputStream, error);
					if (pInputStream->LastRead() <= 0 || message->text == _T(""))
					{
						delete message;
						goto loopexit;
					}
					SendMessage(message);
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

	std::list<sftp_message*> m_sftpMessages;
	wxCriticalSection m_criticalSection;
};

class CSftpDeleteOpData : public COpData
{
public:
	CSftpDeleteOpData()
		: COpData(cmd_delete)
	{
		m_needSendListing = false;
		m_deleteFailed = false;
	}

	virtual ~CSftpDeleteOpData() { }

	CServerPath path;
	std::list<wxString> files;

	// Set to wxDateTime::UNow initially and after
	// sending an updated listing to the UI.
	wxDateTime m_time;

	bool m_needSendListing;

	// Set to true if deletion of at least one file failed
	bool m_deleteFailed;
};

CSftpControlSocket::CSftpControlSocket(CFileZillaEnginePrivate *pEngine) : CControlSocket(pEngine)
{
	m_useUTF8 = true;
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

enum connectStates
{
	connect_init,
	connect_proxy,
	connect_keys,
	connect_open
};

class CSftpConnectOpData : public COpData
{
public:
	CSftpConnectOpData()
		: COpData(cmd_connect)
	{
		pLastChallenge = 0;
		criticalFailure = false;
		pKeyFiles = 0;
	}

	virtual ~CSftpConnectOpData()
	{
		delete pKeyFiles;
		delete pLastChallenge;
	}

	wxString *pLastChallenge;
	bool criticalFailure;

	wxStringTokenizer* pKeyFiles;
};

int CSftpControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());
	SetWait(true);

	m_sftpEncryptionDetails = CSftpEncryptionNotification();

	delete m_pCSConv;
	if (server.GetEncodingType() == ENCODING_CUSTOM)
	{
		m_pCSConv = new wxCSConv(server.GetCustomEncoding());
		m_useUTF8 = false;
	}
	else
	{
		m_pCSConv = 0;
		m_useUTF8 = true;
	}


	if (m_pCurrentServer)
		delete m_pCurrentServer;
	m_pCurrentServer = new CServer(server);

	CSftpConnectOpData* pData = new CSftpConnectOpData;
	m_pCurOpData = pData;

	pData->opState = connect_init;

	wxStringTokenizer* pTokenizer = new wxStringTokenizer(m_pEngine->GetOptions()->GetOption(OPTION_SFTP_KEYFILES), _T("\n"), wxTOKEN_DEFAULT);
	if (!pTokenizer->HasMoreTokens())
		delete pTokenizer;
	else
		pData->pKeyFiles = pTokenizer;

	m_pProcess = new wxProcess(this);
	m_pProcess->Redirect();

	CRateLimiter::Get()->AddObject(this);

	wxString executable = m_pEngine->GetOptions()->GetOption(OPTION_FZSFTP_EXECUTABLE);
	if (executable == _T(""))
		executable = _T("fzsftp");
	LogMessage(Debug_Verbose, _T("Going to execute %s"), executable.c_str());

	m_pid = wxExecute(executable + _T(" -v"), wxEXEC_ASYNC, m_pProcess);
	if (!m_pid)
	{
		delete m_pProcess;
		m_pProcess = 0;
		DoClose();
		return FZ_REPLY_ERROR;
	}

	m_pInputThread = new CSftpInputThread(this, m_pProcess);
	if (!m_pInputThread->Init())
	{
		delete m_pInputThread;
		m_pInputThread = 0;
		m_pProcess->Detach();
		m_pProcess = 0;
		DoClose();
		return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::ConnectParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ConnectParseResponse(%s)"), reply.c_str());

	if (!successful)
	{
		DoClose(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpConnectOpData *pData = static_cast<CSftpConnectOpData *>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData of wrong type"));
		DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	switch (pData->opState)
	{
	case connect_init:
		if (m_pEngine->GetOptions()->GetOptionVal(OPTION_PROXY_TYPE))
			pData->opState = connect_proxy;
		else if (pData->pKeyFiles)
			pData->opState = connect_keys;
		else
			pData->opState = connect_open;
		break;
	case connect_proxy:
		if (pData->pKeyFiles)
			pData->opState = connect_keys;
		else
			pData->opState = connect_open;
		break;
	case connect_keys:
		wxASSERT(pData->pKeyFiles);
		if (!pData->pKeyFiles->HasMoreTokens())
			pData->opState = connect_open;
		break;
	case connect_open:
		m_pEngine->AddNotification(new CSftpEncryptionNotification(m_sftpEncryptionDetails));
		ResetOperation(FZ_REPLY_OK);
		return FZ_REPLY_OK;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state: %d"), pData->opState);
        DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	return SendNextCommand();
}

int CSftpControlSocket::ConnectSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ConnectSend()"));
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpConnectOpData *pData = static_cast<CSftpConnectOpData *>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData of wrong type"));
		DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	bool res;
	switch (pData->opState)
	{
	case connect_proxy:
		{
			int type;
			switch (m_pEngine->GetOptions()->GetOptionVal(OPTION_PROXY_TYPE))
			{
			case CProxySocket::HTTP:
				type = 1;
				break;
			case CProxySocket::SOCKS5:
				type = 2;
				break;
			default:
				LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unsupported proxy type"));
				DoClose(FZ_REPLY_INTERNALERROR);
				return FZ_REPLY_ERROR;
			}

			wxString cmd = wxString::Format(_T("proxy %d \"%s\" %d"), type,
											m_pEngine->GetOptions()->GetOption(OPTION_PROXY_HOST),
											m_pEngine->GetOptions()->GetOptionVal(OPTION_PROXY_PORT));
			wxString user = m_pEngine->GetOptions()->GetOption(OPTION_PROXY_USER);
			if (user != _T(""))
				cmd += _T(" \"") + user + _T("\"");
			wxString pass = m_pEngine->GetOptions()->GetOption(OPTION_PROXY_PASS);
			if (pass != _T(""))
				cmd += _T(" \"") + pass + _T("\"");
			res = Send(cmd);
		}
		break;
	case connect_keys:
		res = Send(_T("keyfile \"") + pData->pKeyFiles->GetNextToken() + _T("\""));
		break;
	case connect_open:
		res = Send(wxString::Format(_T("open \"%s@%s\" %d"), m_pCurrentServer->GetUser().c_str(), m_pCurrentServer->GetHost().c_str(), m_pCurrentServer->GetPort()));
		break;
	default:
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Unknown op state: %d"), pData->opState);
        DoClose(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (res)
		return FZ_REPLY_WOULDBLOCK;
	else
		return FZ_REPLY_ERROR;
}

void CSftpControlSocket::OnSftpEvent(wxCommandEvent& event)
{
	if (!m_pCurrentServer)
		return;

	if (!m_pInputThread)
		return;

	std::list<sftp_message*> messages;
	m_pInputThread->GetMessages(messages);
	for (std::list<sftp_message*>::iterator iter = messages.begin(); iter != messages.end(); iter++)
	{
		if (!m_pInputThread)
		{
			delete *iter;
			continue;
		}

		sftp_message* message = *iter;

		switch (message->type)
		{
		case sftpReply:
			LogMessageRaw(Response, message->text);
			ProcessReply(true, message->text);
			break;
		case sftpStatus:
			LogMessageRaw(Status, message->text);
			break;
		case sftpError:
			LogMessageRaw(::Error, message->text);
			break;
		case sftpVerbose:
			LogMessageRaw(Debug_Info, message->text);
			break;
		case sftpDone:
			{
				ProcessReply(message->text == _T("1"));
				break;
			}
		case sftpRequestPreamble:
			m_requestPreamble = message->text;
			break;
		case sftpRequestInstruction:
			m_requestInstruction = message->text;
			break;
		case sftpRequest:
			switch(message->reqType)
			{
			case sftpReqPassword:
				if (!m_pCurOpData || m_pCurOpData->opId != cmd_connect)
				{
					LogMessage(Debug_Warning, _T("sftpReqPassword outside connect operation, ignoring."));
					break;
				}

				if (m_pCurrentServer->GetLogonType() == INTERACTIVE)
				{
					wxString challenge;
					if (m_requestPreamble != _T(""))
						challenge += m_requestPreamble + _T("\n");
					if (m_requestInstruction != _T(""))
						challenge += m_requestInstruction + _T("\n");
					if (message->text != _T("Password:"))
						challenge += message->text;
					CInteractiveLoginNotification *pNotification = new CInteractiveLoginNotification(challenge);
					pNotification->server = *m_pCurrentServer;

					SendAsyncRequest(pNotification);
				}
				else
				{
					CSftpConnectOpData *pData = reinterpret_cast<CSftpConnectOpData*>(m_pCurOpData);

					const wxString newChallenge = m_requestPreamble + _T("\n") + m_requestInstruction + message->text;

					if (pData->pLastChallenge)
					{
						// Check for same challenge. Will most likely fail as well, so abort early.
						if (*pData->pLastChallenge == newChallenge)
							LogMessage(::Error, _("Authentication failed."));
						else
							LogMessage(::Error, _("Server sent an additional login prompt. You need to use the interactive login type."));
						DoClose(FZ_REPLY_CRITICALERROR | FZ_REPLY_PASSWORDFAILED);
						return;
					}

					pData->pLastChallenge = new wxString(newChallenge);

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
			ListParseEntry(message->text);
			break;
		case sftpRead:
		case sftpWrite:
			{
				long number = 0;
				message->text.ToLong(&number);
				
				if (m_pTransferStatus && !m_pTransferStatus->madeProgress)
				{
					if (m_pCurOpData && m_pCurOpData->opId == cmd_transfer)
					{
						CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);
						if (pData->download)
						{
							if (number > 0)
								SetTransferStatusMadeProgress();
						}
						else
						{
							if (m_pTransferStatus->currentOffset > m_pTransferStatus->startOffset + 65565)
								SetTransferStatusMadeProgress();
						}
					}
				}

				UpdateTransferStatus(number);
			}
			break;
		case sftpRecv:
			SetActive(true);
			break;
		case sftpSend:
			SetActive(false);
			break;
		case sftpUsedQuotaRecv:
			OnQuotaRequest(CRateLimiter::inbound);
			break;
		case sftpUsedQuotaSend:
			OnQuotaRequest(CRateLimiter::outbound);
			break;
		case sftpKexAlgorithm:
			m_sftpEncryptionDetails.kexAlgorithm = message->text;
			break;
		case sftpKexHash:
			m_sftpEncryptionDetails.kexHash = message->text;
			break;
		case sftpCipherClientToServer:
			m_sftpEncryptionDetails.cipherClientToServer = message->text;
			break;
		case sftpCipherServerToClient:
			m_sftpEncryptionDetails.cipherServerToClient = message->text;
			break;
		case sftpMacClientToServer:
			m_sftpEncryptionDetails.macClientToServer = message->text;
			break;
		case sftpMacServerToClient:
			m_sftpEncryptionDetails.macServerToClient = message->text;
			break;
		case sftpHostkey:
			m_sftpEncryptionDetails.hostKey = message->text;
			break;
		default:
			wxFAIL_MSG(_T("given notification codes not handled"));
			break;
		}
		delete message;
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
	SetWait(true);

	if (show != _T(""))
		LogMessageRaw(Command, show);
	else
		LogMessageRaw(Command, cmd);

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

	return AddToStream(cmd);
}

bool CSftpControlSocket::AddToStream(const wxString& cmd, bool force_utf8 /*=false*/)
{
	const wxCharBuffer str = ConvToServer(cmd, force_utf8);

	if (!m_pProcess)
		return false;

	if (!str)
	{
		LogMessage(::Error, _("Could not convert command to server encoding"));
		return false;
	}

	wxOutputStream* pStream = m_pProcess->GetOutputStream();
	if (!pStream)
		return false;

	unsigned int len = strlen(str);
	if (pStream->Write(str, len).LastWrite() != len)
		return false;

	return true;
}

bool CSftpControlSocket::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	if (m_pCurOpData)
	{
		if (!m_pCurOpData->waitForAsyncRequest)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Not waiting for request reply, ignoring request reply %d"), pNotification->GetRequestID());
			return false;
		}
		m_pCurOpData->waitForAsyncRequest = false;
	}

	const enum RequestId requestId = pNotification->GetRequestID();
	switch(requestId)
	{
	case reqId_fileexists:
		{
			CFileExistsNotification *pFileExistsNotification = reinterpret_cast<CFileExistsNotification *>(pNotification);
			return SetFileExistsAction(pFileExistsNotification);
		}
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
			wxString show;
			if (requestId == reqId_hostkey)
				show = _("Trust new Hostkey: ");
			else
				show = _("Trust changed Hostkey: ");
			if (!pHostKeyNotification->m_trust)
			{
				Send(_T(""), show + _("No"));
				if (m_pCurOpData && m_pCurOpData->opId == cmd_connect)
				{
					CSftpConnectOpData *pData = static_cast<CSftpConnectOpData *>(m_pCurOpData);
					pData->criticalFailure = true;
				}
			}
			else if (pHostKeyNotification->m_alwaysTrust)
				Send(_T("y"), show + _("Yes"));
			else
				Send(_T("n"), show + _("Once"));
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
	default:
		LogMessage(Debug_Warning, _T("Unknown async request reply id: %d"), requestId);
		return false;
	}

	return true;
}

class CSftpListOpData : public COpData
{
public:
	CSftpListOpData()
		: COpData(cmd_list)
	{
		pParser = 0;
		mtime_index = 0;
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

	CDirectoryListing directoryListing;
	int mtime_index;
};

enum listStates
{
	list_init = 0,
	list_waitcwd,
	list_list,
	list_mtime
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

	return ParseSubcommandResult(FZ_REPLY_OK);
}

int CSftpControlSocket::ListParseResponse(bool successful, const wxString& reply)
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

	if (pData->opState == list_list)
	{
		if (!successful)
		{
			ResetOperation(FZ_REPLY_ERROR);
			return FZ_REPLY_ERROR;
		}

		if (!pData->pParser)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("pData->pParser is 0"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}

		pData->directoryListing = pData->pParser->Parse(m_CurrentPath);

		int res = ListCheckTimezoneDetection();
		if (res != FZ_REPLY_OK)
			return res;

		CDirectoryCache cache;
		cache.Store(pData->directoryListing, *m_pCurrentServer, pData->path, pData->subDir);

		m_pEngine->SendDirectoryListingNotification(m_CurrentPath, !pData->pNextOpData, true, false);

		ResetOperation(FZ_REPLY_OK);
		return FZ_REPLY_OK;
	}
	else if (pData->opState == list_mtime)
	{
		if (successful && reply != _T(""))
		{
			time_t seconds = 0;
			bool parsed = true;
			for (unsigned int i = 0; i < reply.Len(); i++)
			{
				wxChar c = reply[i];
				if (c < '0' || c > '9')
				{
					parsed = false;
					break;
				}
				seconds *= 10;
				seconds += c - '0';
			}
			if (parsed)
			{
				wxDateTime date = wxDateTime(seconds);
				if (date.IsValid())
				{
					date.MakeTimezone(wxDateTime::GMT0);
					wxASSERT(pData->directoryListing[pData->mtime_index].hasTime);
					wxDateTime listTime = pData->directoryListing[pData->mtime_index].time;
					listTime -= wxTimeSpan(0, m_pCurrentServer->GetTimezoneOffset(), 0);

					int serveroffset = (date - listTime).GetSeconds().GetLo();

					wxDateTime now = wxDateTime::Now();
					wxDateTime now_utc = now.ToTimezone(wxDateTime::GMT0);

					int localoffset = (now - now_utc).GetSeconds().GetLo();
					int offset = serveroffset + localoffset;

					LogMessage(Status, _("Timezone offsets: Server: %d seconds. Local: %d seconds. Difference: %d seconds."), -serveroffset, localoffset, offset);

					wxTimeSpan span(0, 0, offset);
					const int count = pData->directoryListing.GetCount();
					for (int i = 0; i < count; i++)
					{
						CDirentry& entry = pData->directoryListing[i];
						if (!entry.hasTime)
							continue;

						entry.time += span;
					}

					// TODO: Correct cached listings

					CServerCapabilities::SetCapability(*m_pCurrentServer, timezone_offset, yes, offset);
				}
			}
		}

		CDirectoryCache cache;
		cache.Store(pData->directoryListing, *m_pCurrentServer, pData->path, pData->subDir);

		m_pEngine->SendDirectoryListingNotification(m_CurrentPath, !pData->pNextOpData, true, false);

		ResetOperation(FZ_REPLY_OK);
		return FZ_REPLY_OK;
	}

	LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("ListParseResponse called at inproper time: %d"), pData->opState);
	ResetOperation(FZ_REPLY_INTERNALERROR);
	return FZ_REPLY_ERROR;
}

int CSftpControlSocket::ListParseEntry(const wxString& entry)
{
	if (!m_pCurOpData)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (m_pCurOpData->opId != cmd_list)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Listentry received, but current operation is not cmd_list"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
	if (!pData)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData of wrong type"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (pData->opState != list_list)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("ListParseResponse called at inproper time: %d"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (!pData->pParser)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("pData->pParser is 0"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_INTERNALERROR;
	}

	if (entry.Find('\r') != -1 || entry.Find('\n') != -1)
	{
		LogMessageRaw(RawList, entry);
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("Listing entry contains \\r at pos %d and \\n at pos %d. Please contect FileZilla team."), entry.Find('\r'), entry.Find('\n'));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_INTERNALERROR;
	}

	pData->pParser->AddLine(entry);

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::ListSubcommandResult(int prevResult)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ListSubcommandResult()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	if (pData->opState != list_waitcwd)
	{
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (prevResult != FZ_REPLY_OK)
	{
		ResetOperation(prevResult);
		return FZ_REPLY_ERROR;
	}

	if (!pData->refresh)
	{
		wxASSERT(!pData->pNextOpData);

		// Do a cache lookup now that we know the correct directory
		CDirectoryCache cache;

		int hasUnsureEntries;
		bool is_outdated = false;
		bool found = cache.DoesExist(*m_pCurrentServer, m_CurrentPath, _T(""), hasUnsureEntries, is_outdated);
		if (found)
		{
			if (!pData->path.IsEmpty() && pData->subDir != _T(""))
				cache.AddParent(*m_pCurrentServer, m_CurrentPath, pData->path, pData->subDir);

			// We're done if listins is recent and has no outdated entries
			if (!is_outdated && !hasUnsureEntries)
			{
				m_pEngine->SendDirectoryListingNotification(m_CurrentPath, true, false, false);

				ResetOperation(FZ_REPLY_OK);

				return FZ_REPLY_OK;
			}
		}
	}

	if (!pData->holdsLock)
	{
		if (!TryLockCache(lock_list, m_CurrentPath))
			return FZ_REPLY_WOULDBLOCK;
	}

	pData->opState = list_list;

	return SendNextCommand();
}

int CSftpControlSocket::ListSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ListSend()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	if (pData->opState == list_list)
	{
		pData->pParser = new CDirectoryListingParser(this, *m_pCurrentServer);
		pData->pParser->SetTimezoneOffset(GetTimezoneOffset());
		if (!Send(_T("ls")))
			return FZ_REPLY_ERROR;
		return FZ_REPLY_WOULDBLOCK;
	}
	else if (pData->opState == list_mtime)
	{
		LogMessage(Status, _("Calculating timezone offset of server..."));
		const wxString& name = pData->directoryListing[pData->mtime_index].name;
		wxString quotedFilename = QuoteFilename(pData->directoryListing.path.FormatFilename(name, true));
		if (!Send(_T("mtime ") + WildcardEscape(quotedFilename),
			_T("mtime ") + quotedFilename))
			return FZ_REPLY_ERROR;
		return FZ_REPLY_WOULDBLOCK;
	}

	LogMessage(Debug_Warning, _T("Unknown opStatein CSftpControlSocket::ListSend"));
	ResetOperation(FZ_REPLY_INTERNALERROR);
	return FZ_REPLY_ERROR;
}

class CSftpChangeDirOpData : public CChangeDirOpData
{
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

	CServerPath target;
	if (path.IsEmpty())
	{
		if (m_CurrentPath.IsEmpty())
			state = cwd_pwd;
		else
			return FZ_REPLY_OK;
	}
	else
	{
		if (subDir != _T(""))
		{
			// Check if the target is in cache already
			CPathCache cache;
			target = cache.Lookup(*m_pCurrentServer, path, subDir);
			if (!target.IsEmpty())
			{
				if (m_CurrentPath == target)
					return FZ_REPLY_OK;

				path = target;
				subDir = _T("");
				state = cwd_cwd;
			}
			else
			{
				// Target unknown, check for the parent's target
				target = cache.Lookup(*m_pCurrentServer, path, _T(""));
				if (m_CurrentPath == path || (!target.IsEmpty() && target == m_CurrentPath))
				{
					target.Clear();
					state = cwd_cwd_subdir;
				}
				else
					state = cwd_cwd;
			}
		}
		else
		{
			CPathCache cache;
			target = cache.Lookup(*m_pCurrentServer, path, _T(""));
			if (m_CurrentPath == path || (!target.IsEmpty() && target == m_CurrentPath))
				return FZ_REPLY_OK;
			state = cwd_cwd;
		}
	}

	CSftpChangeDirOpData *pData = new CSftpChangeDirOpData;
	pData->pNextOpData = m_pCurOpData;
	pData->opState = state;
	pData->path = path;
	pData->subDir = subDir;
	pData->target = target;

	if (pData->pNextOpData && pData->pNextOpData->opId == cmd_transfer &&
		!static_cast<CSftpFileTransferOpData *>(pData->pNextOpData)->download)
	{
		pData->tryMkdOnFail = true;
		wxASSERT(subDir == _T(""));
	}

	m_pCurOpData = pData;

	return SendNextCommand();
}

int CSftpControlSocket::ChangeDirParseResponse(bool successful, const wxString& reply)
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
		if (!successful || reply == _T(""))
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
		if (!successful)
		{
			// Create remote directory if part of a file upload
			if (pData->tryMkdOnFail)
			{
				pData->tryMkdOnFail = false;
				int res = Mkdir(pData->path);
				if (res != FZ_REPLY_OK)
					return res;
			}
			else
				error = true;
		}
		else if (reply == _T(""))
			error = true;
		else if (ParsePwdReply(reply))
		{
			CPathCache cache;
			cache.Store(*m_pCurrentServer, m_CurrentPath, pData->path);

			if (pData->subDir == _T(""))
			{
				ResetOperation(FZ_REPLY_OK);
				return FZ_REPLY_OK;
			}

			pData->target.Clear();
			pData->opState = cwd_cwd_subdir;
		}
		else
			error = true;
		break;
	case cwd_cwd_subdir:
		if (!successful || reply == _T(""))
			error = true;
		else if (ParsePwdReply(reply))
		{
			CPathCache cache;
			cache.Store(*m_pCurrentServer, m_CurrentPath, pData->path, pData->subDir);

			ResetOperation(FZ_REPLY_OK);
			return FZ_REPLY_OK;
		}
		else
			error = true;
		break;
	default:
		error = true;
		break;
	}

	if (error)
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	return SendNextCommand();
}

int CSftpControlSocket::ChangeDirSubcommandResult(int WXUNUSED(prevResult))
{
	LogMessage(Debug_Verbose, _("CSftpControlSocket::ChangeDirSubcommandResult()"));

	return SendNextCommand();
}

int CSftpControlSocket::ChangeDirSend()
{
	LogMessage(Debug_Verbose, _("CSftpControlSocket::ChangeDirSend()"));

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
		if (pData->tryMkdOnFail && !pData->holdsLock)
		{
			if (IsLocked(lock_mkdir, pData->path))
			{
				// Some other engine is already creating this directory or
				// performing an action that will lead to its creation
				pData->tryMkdOnFail = false;
			}
			if (!TryLockCache(lock_mkdir, pData->path))
				return FZ_REPLY_WOULDBLOCK;
		}
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
		return ConnectParseResponse(successful, reply);
	case cmd_list:
		return ListParseResponse(successful, reply);
	case cmd_transfer:
		return FileTransferParseResponse(successful, reply);
	case cmd_cwd:
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

	if (m_pCurOpData && m_pCurOpData->opId == cmd_connect)
	{
		CSftpConnectOpData *pData = static_cast<CSftpConnectOpData *>(m_pCurOpData);
		if (pData->opState == connect_init && (nErrorCode & FZ_REPLY_CANCELED) != FZ_REPLY_CANCELED)
			LogMessage(::Error, _("fzsftp could not be started"));
		if (pData->criticalFailure)
			nErrorCode |= FZ_REPLY_CRITICALERROR;
	}
	if (m_pCurOpData && m_pCurOpData->opId == cmd_delete && !(nErrorCode & FZ_REPLY_DISCONNECTED))
	{
		CSftpDeleteOpData *pData = static_cast<CSftpDeleteOpData *>(m_pCurOpData);
		if (pData->m_needSendListing)
			m_pEngine->SendDirectoryListingNotification(pData->path, false, true, false);
	}

	return CControlSocket::ResetOperation(nErrorCode);
}

int CSftpControlSocket::SendNextCommand()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::SendNextCommand()"));
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
	case cmd_connect:
		return ConnectSend();
	case cmd_list:
		return ListSend();
	case cmd_transfer:
		return FileTransferSend();
	case cmd_cwd:
		return ChangeDirSend();
	case cmd_mkdir:
		return MkdirSend();
	case cmd_rename:
		return RenameSend();
	case cmd_chmod:
		return ChmodSend();
	case cmd_delete:
		return DeleteSend();
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

	CSftpFileTransferOpData *pData = new CSftpFileTransferOpData;
	m_pCurOpData = pData;

	pData->localFile = localFile;
	pData->remotePath = remotePath;
	pData->remoteFile = remoteFile;
	pData->download = download;
	pData->transferSettings = transferSettings;

	wxLongLong size;
	bool isLink;
	if (CLocalFileSystem::GetFileInfo(pData->localFile, isLink, &size, 0, 0) == CLocalFileSystem::file)
		pData->localFileSize = size.GetValue();

	pData->opState = filetransfer_waitcwd;

	if (pData->remotePath.GetType() == DEFAULT)
		pData->remotePath.SetType(m_pCurrentServer->GetType());

	int res = ChangeDir(pData->remotePath);
	if (res != FZ_REPLY_OK)
		return res;

	return ParseSubcommandResult(FZ_REPLY_OK);
}

int CSftpControlSocket::FileTransferSubcommandResult(int prevResult)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::FileTransferSubcommandResult()"));

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
			CDirentry entry;
			bool dirDidExist;
			bool matchedCase;
			CDirectoryCache cache;
			bool found = cache.LookupFile(entry, *m_pCurrentServer, pData->tryAbsolutePath ? pData->remotePath : m_CurrentPath, pData->remoteFile, dirDidExist, matchedCase);
			if (!found)
			{
				if (!dirDidExist)
					pData->opState = filetransfer_waitlist;
				else if (pData->download &&
					m_pEngine->GetOptions()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS))
				{
					pData->opState = filetransfer_mtime;
				}
				else
					pData->opState = filetransfer_transfer;
			}
			else
			{
				if (entry.unsure)
					pData->opState = filetransfer_waitlist;
				else
				{
					if (matchedCase)
					{
						pData->remoteFileSize = entry.size.GetLo() + ((wxFileOffset)entry.size.GetHi() << 32);
						if (entry.hasDate)
							pData->fileTime = entry.time;

						if (pData->download &&
							(!entry.hasDate || !entry.hasTime) &&
							m_pEngine->GetOptions()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS))
						{
							pData->opState = filetransfer_mtime;
						}
						else
							pData->opState = filetransfer_transfer;
					}
					else
						pData->opState = filetransfer_mtime;
				}
			}
			if (pData->opState == filetransfer_waitlist)
			{
				int res = List(CServerPath(), _T(""), true);
				if (res != FZ_REPLY_OK)
					return res;
				ResetOperation(FZ_REPLY_INTERNALERROR);
				return FZ_REPLY_ERROR;
			}
			else if (pData->opState == filetransfer_transfer)
			{
				int res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
		}
		else
		{
			pData->tryAbsolutePath = true;
			pData->opState = filetransfer_mtime;
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
					pData->opState = filetransfer_mtime;
				else if (pData->download &&
					m_pEngine->GetOptions()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS))
				{
					pData->opState = filetransfer_mtime;
				}
				else
					pData->opState = filetransfer_transfer;
			}
			else
			{
				if (matchedCase && !entry.unsure)
				{
					pData->remoteFileSize = entry.size.GetLo() + ((wxFileOffset)entry.size.GetHi() << 32);
					if (entry.hasDate)
						pData->fileTime = entry.time;

					if (pData->download &&
						(!entry.hasDate || !entry.hasTime) &&
						m_pEngine->GetOptions()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS))
					{
						pData->opState = filetransfer_mtime;
					}
					else
						pData->opState = filetransfer_transfer;
				}
				else
					pData->opState = filetransfer_mtime;
			}
			if (pData->opState == filetransfer_transfer)
			{
				int res = CheckOverwriteFile();
				if (res != FZ_REPLY_OK)
					return res;
			}
		}
		else
			pData->opState = filetransfer_mtime;
	}
	else
	{
		LogMessage(Debug_Warning, _T("  Unknown opState (%d)"), pData->opState);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	return SendNextCommand();
}

int CSftpControlSocket::FileTransferSend()
{
	LogMessage(Debug_Verbose, _T("FileTransferSend()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);

	if (pData->opState == filetransfer_transfer)
	{
		wxString cmd;
		if (pData->resume)
			cmd = _T("re");
		if (pData->download)
		{
			// Create local directory
			if (!pData->resume)
			{
				wxFileName fn(pData->localFile);
				wxFileName::Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
			}

			InitTransferStatus(pData->remoteFileSize, pData->resume ? pData->localFileSize : 0, false);
			cmd += _T("get ");
			cmd += QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath)) + _T(" ");

			wxString localFile = QuoteFilename(pData->localFile);
			wxString logstr = cmd;
			logstr += localFile;
			LogMessageRaw(Command, logstr);

			if (!AddToStream(cmd) || !AddToStream(localFile + _T("\n"), true))
			{
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}
		else
		{
			InitTransferStatus(pData->localFileSize, pData->resume ? pData->remoteFileSize : 0, false);
			cmd += _T("put ");

			wxString logstr = cmd;
			wxString localFile = QuoteFilename(pData->localFile) + _T(" ");
			wxString remoteFile = QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath));

			logstr += localFile;
			logstr += remoteFile;
			LogMessageRaw(Command, logstr);

			if (!AddToStream(cmd) || !AddToStream(localFile, true) ||
				!AddToStream(remoteFile + _T("\n")))
			{
				ResetOperation(FZ_REPLY_ERROR);
				return FZ_REPLY_ERROR;
			}
		}
		SetTransferStatusStartTime();

		pData->transferInitiated = true;
	}
	else if (pData->opState == filetransfer_mtime)
	{
		wxString quotedFilename = QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath));
		if (!Send(_T("mtime ") + WildcardEscape(quotedFilename),
			_T("mtime ") + quotedFilename))
			return FZ_REPLY_ERROR;
	}
	else if (pData->opState == filetransfer_chmtime)
	{
		wxASSERT(pData->fileTime.IsValid());
		if (pData->download)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("  filetransfer_chmtime during download"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}

		wxString quotedFilename = QuoteFilename(pData->remotePath.FormatFilename(pData->remoteFile, !pData->tryAbsolutePath));
		// Y2K38		
		time_t ticks = pData->fileTime.GetTicks(); // Already in UTC
		wxString seconds = wxString::Format(_T("%d"), (int)ticks);
		if (!Send(_T("chmtime ") + seconds + _T(" ") + WildcardEscape(quotedFilename),
			_T("chmtime ") + seconds + _T(" ") + quotedFilename))
			return FZ_REPLY_ERROR;
	}

	return FZ_REPLY_WOULDBLOCK;
}

int CSftpControlSocket::FileTransferParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("FileTransferParseResponse()"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpFileTransferOpData *pData = static_cast<CSftpFileTransferOpData *>(m_pCurOpData);

	if (pData->opState == filetransfer_transfer)
	{
		if (!successful)
		{
			ResetOperation(FZ_REPLY_ERROR);
			return FZ_REPLY_ERROR;
		}

		if (m_pEngine->GetOptions()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS))
		{
			wxFileName fn(pData->localFile);
			if (fn.FileExists())
			{
				if (pData->download)
				{
					if (pData->fileTime.IsValid())
						fn.SetTimes(&pData->fileTime, &pData->fileTime, 0);
				}
				else
				{
					pData->fileTime = fn.GetModificationTime();
					if (pData->fileTime.IsValid())
					{
						pData->opState = filetransfer_chmtime;
						return SendNextCommand();
					}
				}
			}
		}
	}
	else if (pData->opState == filetransfer_mtime)
	{
		if (successful && reply != _T(""))
		{
			time_t seconds = 0;
			bool parsed = true;
			for (unsigned int i = 0; i < reply.Len(); i++)
			{
				wxChar c = reply[i];
				if (c < '0' || c > '9')
				{
					parsed = false;
					break;
				}
				seconds *= 10;
				seconds += c - '0';
			}
			if (parsed)
			{
				wxDateTime fileTime = wxDateTime(seconds);
				if (fileTime.IsValid())
					pData->fileTime = fileTime;
			}
		}
		pData->opState = filetransfer_transfer;
		int res = CheckOverwriteFile();
		if (res != FZ_REPLY_OK)
			return res;

		return SendNextCommand();
	}
	else if (pData->opState == filetransfer_chmtime)
	{
		if (pData->download)
		{
			LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("  filetransfer_chmtime during download"));
			ResetOperation(FZ_REPLY_INTERNALERROR);
			return FZ_REPLY_ERROR;
		}
	}
	else
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
	CRateLimiter::Get()->RemoveObject(this);

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

int CSftpControlSocket::Mkdir(const CServerPath& path)
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

int CSftpControlSocket::MkdirParseResponse(bool successful, const wxString& reply)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::MkdirParseResonse"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CMkdirOpData *pData = static_cast<CMkdirOpData *>(m_pCurOpData);
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
		else if (pData->currentPath == pData->commonParent)
			pData->opState = mkd_tryfull;
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
			cache.UpdateFile(*m_pCurrentServer, pData->currentPath, pData->segments.front(), true, CDirectoryCache::dir);
			m_pEngine->SendDirectoryListingNotification(pData->currentPath, false, true, false);

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
		break;
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

	CMkdirOpData *pData = static_cast<CMkdirOpData *>(m_pCurOpData);
	LogMessage(Debug_Debug, _T("  state = %d"), pData->opState);

	if (!pData->holdsLock)
	{
		if (!TryLockCache(lock_mkdir, pData->path))
			return FZ_REPLY_WOULDBLOCK;
	}

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

int CSftpControlSocket::Delete(const CServerPath& path, const std::list<wxString>& files)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::Delete"));
	wxASSERT(!m_pCurOpData);
	CSftpDeleteOpData *pData = new CSftpDeleteOpData();
	m_pCurOpData = pData;
	pData->path = path;
	pData->files = files;

	// CFileZillaEnginePrivate should have checked this already
	wxASSERT(!files.empty());

	return SendNextCommand();
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

	CSftpDeleteOpData *pData = static_cast<CSftpDeleteOpData *>(m_pCurOpData);

	if (!successful)
		pData->m_deleteFailed = true;
	else
	{
		const wxString& file = pData->files.front();

		CDirectoryCache cache;
		cache.RemoveFile(*m_pCurrentServer, pData->path, file);

		wxDateTime now = wxDateTime::UNow();
		if (now.IsValid() && pData->m_time.IsValid() && (now - pData->m_time).GetSeconds() >= 1)
		{
			m_pEngine->SendDirectoryListingNotification(pData->path, false, true, false);
			pData->m_time = now;
			pData->m_needSendListing = false;
		}
		else
			pData->m_needSendListing = true;
	}

	pData->files.pop_front();

	if (!pData->files.empty())
		return SendNextCommand();

	return ResetOperation(pData->m_deleteFailed ? FZ_REPLY_ERROR : FZ_REPLY_OK);
}

int CSftpControlSocket::DeleteSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::DeleteSend"));

	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty m_pCurOpData"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}
	CSftpDeleteOpData *pData = static_cast<CSftpDeleteOpData *>(m_pCurOpData);

	const wxString& file = pData->files.front();
	if (file == _T(""))
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty filename"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	wxString filename = pData->path.FormatFilename(file);
	if (filename == _T(""))
	{
		LogMessage(::Error, _("Filename cannot be constructed for directory %s and filename %s"), pData->path.GetPath().c_str(), file.c_str());
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, pData->path, file);

	if (!Send(_T("rm ") + WildcardEscape(QuoteFilename(filename)),
			  _T("rm ") + QuoteFilename(filename)))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_WOULDBLOCK;
}

class CSftpRemoveDirOpData : public COpData
{
public:
	CSftpRemoveDirOpData()
		: COpData(cmd_removedir)
	{
	}

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
		LogMessage(::Error, _("Path cannot be constructed for directory %s and subdir %s"), path.GetPath().c_str(), subDir.c_str());
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.InvalidateFile(*m_pCurrentServer, path, subDir);

	CPathCache pathCache;
	pathCache.InvalidatePath(*m_pCurrentServer, pData->path, pData->subDir);

	m_pEngine->InvalidateCurrentWorkingDirs(fullPath);
	if (!Send(_T("rmdir ") + WildcardEscape(QuoteFilename(fullPath.GetPath())),
			  _T("rmdir ") + QuoteFilename(fullPath.GetPath())))
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
	{
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	CSftpRemoveDirOpData *pData = static_cast<CSftpRemoveDirOpData *>(m_pCurOpData);
	if (pData->path.IsEmpty())
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Info, _T("Empty pData->path"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	CDirectoryCache cache;
	cache.RemoveDir(*m_pCurrentServer, pData->path, pData->subDir);
	m_pEngine->SendDirectoryListingNotification(pData->path, false, true, false);

	return ResetOperation(FZ_REPLY_OK);
}

class CSftpChmodOpData : public COpData
{
public:
	CSftpChmodOpData(const CChmodCommand& command)
		: COpData(cmd_chmod), m_cmd(command)
	{
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

	return SendNextCommand();
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

int CSftpControlSocket::ChmodSubcommandResult(int prevResult)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ChmodSend()"));

	CSftpChmodOpData *pData = static_cast<CSftpChmodOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (prevResult != FZ_REPLY_OK)
		pData->m_useAbsolute = true;

	return SendNextCommand();
}

int CSftpControlSocket::ChmodSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ChmodSend()"));

	CSftpChmodOpData *pData = static_cast<CSftpChmodOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	bool res;
	switch (pData->opState)
	{
	case chmod_chmod:
		{
			CDirectoryCache cache;
			cache.UpdateFile(*m_pCurrentServer, pData->m_cmd.GetPath(), pData->m_cmd.GetFile(), false, CDirectoryCache::unknown);

			wxString quotedFilename = QuoteFilename(pData->m_cmd.GetPath().FormatFilename(pData->m_cmd.GetFile(), !pData->m_useAbsolute));

			res = Send(_T("chmod ") + pData->m_cmd.GetPermission() + _T(" ") + WildcardEscape(quotedFilename),
					   _T("chmod ") + pData->m_cmd.GetPermission() + _T(" ") + quotedFilename);
		}
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
		: COpData(cmd_rename), m_cmd(command)
	{
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

	return SendNextCommand();
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

	const CServerPath& fromPath = pData->m_cmd.GetFromPath();
	const CServerPath& toPath = pData->m_cmd.GetToPath();

	CDirectoryCache cache;
	cache.Rename(*m_pCurrentServer, fromPath, pData->m_cmd.GetFromFile(), toPath, pData->m_cmd.GetToFile());

	m_pEngine->SendDirectoryListingNotification(fromPath, false, true, false);
	if (fromPath != toPath)
		m_pEngine->SendDirectoryListingNotification(toPath, false, true, false);

	ResetOperation(FZ_REPLY_OK);
	return FZ_REPLY_OK;
}

int CSftpControlSocket::RenameSubcommandResult(int prevResult)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::RenameSubcommandResult()"));

	CSftpRenameOpData *pData = static_cast<CSftpRenameOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	if (prevResult != FZ_REPLY_OK)
		pData->m_useAbsolute = true;

	return SendNextCommand();
}

int CSftpControlSocket::RenameSend()
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::RenameSend()"));

	CSftpRenameOpData *pData = static_cast<CSftpRenameOpData*>(m_pCurOpData);
	if (!pData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("m_pCurOpData empty"));
		ResetOperation(FZ_REPLY_INTERNALERROR);
		return FZ_REPLY_ERROR;
	}

	bool res;
	switch (pData->opState)
	{
	case rename_rename:
		{
			CDirectoryCache cache;
			bool wasDir = false;
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile(), &wasDir);
			cache.InvalidateFile(*m_pCurrentServer, pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile());

			wxString fromQuoted = QuoteFilename(pData->m_cmd.GetFromPath().FormatFilename(pData->m_cmd.GetFromFile(), !pData->m_useAbsolute));
			wxString toQuoted = QuoteFilename(pData->m_cmd.GetToPath().FormatFilename(pData->m_cmd.GetToFile(), !pData->m_useAbsolute && pData->m_cmd.GetFromPath() == pData->m_cmd.GetToPath()));

			CPathCache pathCache;
			pathCache.InvalidatePath(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile());
			pathCache.InvalidatePath(*m_pCurrentServer, pData->m_cmd.GetToPath(), pData->m_cmd.GetToFile());

			if (wasDir)
			{
				// Need to invalidate current working directories
				CServerPath path = pathCache.Lookup(*m_pCurrentServer, pData->m_cmd.GetFromPath(), pData->m_cmd.GetFromFile());
				if (path.IsEmpty())
				{
					path = pData->m_cmd.GetFromPath();
					path.AddSegment(pData->m_cmd.GetFromFile());
				}
				m_pEngine->InvalidateCurrentWorkingDirs(path);
			}

			res = Send(_T("mv ") + WildcardEscape(fromQuoted) + _T(" ") + toQuoted,
					   _T("mv ") + fromQuoted + _T(" ") + toQuoted);
		}
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

wxString CSftpControlSocket::WildcardEscape(const wxString& file)
{
	// see src/putty/wildcard.c

	wxString escapedFile;
	escapedFile.Alloc(file.Len());
	for (unsigned int i = 0; i < file.Len(); i++)
	{
		const wxChar& c = file[i];
		switch (c)
		{
		case '[':
		case ']':
		case '*':
		case '?':
		case '\\':
			escapedFile.Append('\\');
			break;
		default:
			break;
		}
		escapedFile.Append(c);
	}
	return escapedFile;
}

void CSftpControlSocket::OnRateAvailable(enum CRateLimiter::rate_direction direction)
{
	OnQuotaRequest(direction);
}

void CSftpControlSocket::OnQuotaRequest(enum CRateLimiter::rate_direction direction)
{
	wxLongLong bytes = GetAvailableBytes(direction);
	if (bytes > 0)
	{
		int b;
		if (bytes > INT_MAX)
			b = INT_MAX;
		else
			b = bytes.GetLo();
		AddToStream(wxString::Format(_T("-%d%d\n"), (int)direction, b));
		UpdateUsage(direction, b);
	}
	else if (bytes < 0)
		AddToStream(wxString::Format(_T("-%d-\n"), (int)direction));
	else
		Wait(direction);
}


int CSftpControlSocket::ParseSubcommandResult(int prevResult)
{
	LogMessage(Debug_Verbose, _T("CSftpControlSocket::ParseSubcommandResult(%d)"), prevResult);
	if (!m_pCurOpData)
	{
		LogMessage(__TFILE__, __LINE__, this, Debug_Warning, _T("ParseSubcommandResult called without active operation"));
		ResetOperation(FZ_REPLY_ERROR);
		return FZ_REPLY_ERROR;
	}

	switch (m_pCurOpData->opId)
	{
	case cmd_cwd:
		return ChangeDirSubcommandResult(prevResult);
	case cmd_list:
		return ListSubcommandResult(prevResult);
	case cmd_transfer:
		return FileTransferSubcommandResult(prevResult);
	case cmd_rename:
		return RenameSubcommandResult(prevResult);
	case cmd_chmod:
		return ChmodSubcommandResult(prevResult);
	default:
		LogMessage(__TFILE__, __LINE__, this, ::Debug_Warning, _T("Unknown opID (%d) in ParseSubcommandResult"), m_pCurOpData->opId);
		ResetOperation(FZ_REPLY_INTERNALERROR);
		break;
	}

	return FZ_REPLY_ERROR;
}

int CSftpControlSocket::ListCheckTimezoneDetection()
{
	wxASSERT(m_pCurOpData);

	CSftpListOpData *pData = static_cast<CSftpListOpData *>(m_pCurOpData);

	if (CServerCapabilities::GetCapability(*m_pCurrentServer, timezone_offset) == unknown)
	{
		const int count = pData->directoryListing.GetCount();
		for (int i = 0; i < count; i++)
		{
			if (!pData->directoryListing[i].hasTime)
				continue;

			pData->opState = list_mtime;
			pData->mtime_index = i;
			return SendNextCommand();
		}
	}

	return FZ_REPLY_OK;
}
