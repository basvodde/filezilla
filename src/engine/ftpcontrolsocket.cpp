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
	LogMessage(__FILE__, __LINE__, this, Debug_Verbose, _T("OnReceive()"));

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
						//TODO: Reply handling
					}
				}
				// start of new multi-line
				else if (m_ReceiveBuffer.GetChar(3) == '-')
				{
					// DDD<SP> is the end of a multi-line response
					m_MultilineResponseCode = m_ReceiveBuffer.Left(3)+' ';
				}
				else
				{
					//TODO: Reply handling
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

	ResetOperation(FZ_REPLY_OK);
}