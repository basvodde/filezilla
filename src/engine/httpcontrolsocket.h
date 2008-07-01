#ifndef __HTTPCONTROLSOCKET_H__
#define __HTTPCONTROLSOCKET_H__

class CHttpOpData;
class CHttpControlSocket : public CRealControlSocket
{
public:
	CHttpControlSocket(CFileZillaEnginePrivate *pEngine);
	virtual ~CHttpControlSocket();

protected:
	virtual int ContinueConnect(const wxIPV4address *address);
	virtual bool Connected() { return m_pCurrentServer != 0; }

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);
	virtual int SendNextCommand();
	virtual int ParseSubcommandResult(int prevResult);

	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings);
	virtual int FileTransferSend();
	virtual int FileTransferParseResponse(char* p, unsigned int len);
	virtual int FileTransferSubcommandResult(int prevResult);

	int InternalConnect(wxString host, unsigned short port);
	int DoInternalConnect();

	virtual void OnConnect();
	virtual void OnClose();
	virtual void OnReceive();

	virtual int ResetOperation(int nErrorCode);
	
	virtual void ResetHttpData(CHttpOpData* pData);

	int ParseHeader(CHttpOpData* pData);
	int OnChunkedData(CHttpOpData* pData);

	int ProcessData(char* p, int len);

	// IP address of server
	wxIPV4address* m_pAddress;

	char* m_pRecvBuffer;
	unsigned int m_recvBufferPos;
	static const unsigned int m_recvBufferLen = 4096;

	CHttpOpData* m_pHttpOpData;
};

#endif //__HTTPCONTROLSOCKET_H__
