#include "FileZilla.h"
#include "ftpcontrolsocket.h"

CFtpControlSocket::CFtpControlSocket(CFileZillaEngine *pEngine) : CControlSocket(pEngine)
{
	m_ReceiveBuffer.Alloc(2000);
}

CFtpControlSocket::~CFtpControlSocket()
{
	delete m_pCurOpData;
}

#define BUFFERSIZE 4096
void CFtpControlSocket::OnReceive(wxSocketEvent &event)
{
	LogMessage(__TFILE__, __LINE__, this, Debug_Verbose, _T("OnReceive()"));

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

	for (int i=0; i < numread; i++)
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
	}

	virtual ~CLogonOpData()
	{
	}

	int logonSequencePos;
	int logonType;
};

void CFtpControlSocket::Logon()
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
		return;
	}
	if (!m_nOpState)
	{
		m_nOpState = 1;
		nCommand = logonseq[pData->logonType][0];
	}
	else if (m_nOpState == 1)
	{
		pData->logonSequencePos = logonseq[pData->logonType][pData->logonSequencePos + code - 1];

		switch(pData->logonSequencePos)
		{
		case ER: // ER means summat has gone wrong
			DoClose();
			return;
		case LO: //LO means we are logged on
			LogMessage(Status, _("Connected"));
			ResetOperation(FZ_REPLY_OK);
			return;
		}

		nCommand = logonseq[pData->logonType][pData->logonSequencePos];
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
	int len = strlen(buffer);
	return CControlSocket::Send(buffer, len);
}
