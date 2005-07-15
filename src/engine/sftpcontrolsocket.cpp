#include "FileZilla.h"
#include "sftpcontrolsocket.h"
#include <wx/process.h>
#include <wx/txtstrm.h>

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
				{
					wxTextInputStream textStream(*pInputStream);
					wxString text = textStream.ReadLine();
					if (pInputStream->LastRead() <= 0 || text == _T(""))
						goto loopexit;
					m_pOwner->LogMessage(Response, text);
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
	Send(wxString::Format(_T("open %s@%s %d"), server.GetUser().c_str(), server.GetHost().c_str(), server.GetPort()).mb_str());

	return FZ_REPLY_WOULDBLOCK;
}

void CSftpControlSocket::OnSftpEvent(CSftpEvent& event)
{
	switch (event.GetType())
	{
	case sftpReply:
	case sftpStatus:
	case sftpError:
	case sftpVerbose:
		wxFAIL_MSG(_T("given notification codes should have been handled by thread"));
		break;
	case sftpDone:
		{
			bool successful = event.GetText() == _T("1");
			if (successful)
				ResetOperation(FZ_REPLY_OK);
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
				Send(m_pCurrentServer->GetPass().mb_str());
			}
			break;
		default:
			wxFAIL_MSG(_T("given notification codes should have been handled by thread"));
			break;
		}
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

bool CSftpControlSocket::Send(const char* str)
{
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
			Send("");
		else if (pHostKeyNotification->m_alwaysTrust)
			Send("y");
		else
			Send("n");

		return true;
	}
	else if (pNotification->GetRequestID() == reqId_interactiveLogin)
	{
		CInteractiveLoginNotification *pInteractiveLoginNotification = reinterpret_cast<CInteractiveLoginNotification *>(pNotification);
		
		m_pCurrentServer->SetUser(m_pCurrentServer->GetUser(), pInteractiveLoginNotification->server.GetPass());
		Send(m_pCurrentServer->GetPass().mb_str());
	}

	return false;
}
