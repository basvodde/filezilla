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
	m_text = text;
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
			case sftpError:
			case sftpVerbose:
			case sftpStatus:
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
	m_pProcess = new wxProcess(this);
	m_pProcess->Redirect();

	m_pid = wxExecute(_T("psftp -v"), wxEXEC_ASYNC, m_pProcess);
	wxASSERT(m_pid);

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
		LogMessage(Response, event.GetText());
		break;
	case sftpStatus:
		LogMessage(Status, event.GetText());
		break;
	case sftpError:
		LogMessage(::Error, event.GetText());
		break;
	case sftpVerbose:
		LogMessage(Debug_Info, event.GetText());
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
	default:
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
