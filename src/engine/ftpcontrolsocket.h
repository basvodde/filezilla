#ifndef __FTPCONTROLSOCKET_H__
#define __FTPCONTROLSOCKET_H__

#include "logging_private.h"
#include "ControlSocket.h"

class CTransferSocket;
class CFtpControlSocket : public CControlSocket
{
public:
	CFtpControlSocket(CFileZillaEngine *pEngine);
	virtual ~CFtpControlSocket();
	virtual void TransferEnd(int reason);

protected:
	virtual int ResetOperation(int nErrorCode);

	virtual bool List(CServerPath path = CServerPath(), wxString subDir = _T(""));
	bool ParsePwdReply(wxString reply);

	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);

	virtual bool Send(wxString str);

	void ParseResponse();
	int GetReplyCode() const;

	void Logon();

	wxString m_ReceiveBuffer;
	wxString m_MultilineResponseCode;

	CTransferSocket *m_pTransferSocket;
};

#endif
