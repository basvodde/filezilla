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
	bool ListParseResponse();
	bool ListSend();

	bool ChangeDir(CServerPath path = CServerPath(), wxString subDir = _T(""));
	bool ChangeDirParseResponse();
	bool ChangeDirSend();

	bool ParsePwdReply(wxString reply);

	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);

	virtual bool Send(wxString str);

	void ParseResponse();
	bool SendNextCommand();

	int GetReplyCode() const;

	bool Logon();

	wxString m_ReceiveBuffer;
	wxString m_MultilineResponseCode;

	CTransferSocket *m_pTransferSocket;
};

#endif
