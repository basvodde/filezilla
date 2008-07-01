#ifndef __EXTERNALIPRESOLVER_H__
#define __EXTERNALIPRESOLVER_H__

class fzExternalIPResolveEvent : public wxEvent
{
public:
	fzExternalIPResolveEvent(int id = wxID_ANY);
	virtual wxEvent *Clone() const;
};

typedef void (wxEvtHandler::*fzExternalIPResolveEventFunction)(fzExternalIPResolveEvent&);

extern const wxEventType fzEVT_EXTERNALIPRESOLVE;
#define EVT_FZ_EXTERNALIPRESOLVE(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_EXTERNALIPRESOLVE, id, -1, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( fzExternalIPResolveEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

class CSocket;
class CSocketEvent;
class CExternalIPResolver : public wxEvtHandler
{
public:
	CExternalIPResolver(wxEvtHandler* handler, int id = wxID_ANY);
	virtual ~CExternalIPResolver();

	bool Done() const { return m_done; }
	bool Successful() const { return m_ip != _T(""); }
	wxString GetIP() const { return m_ip; }

	void GetExternalIP(const wxString& address = _T(""), bool force = false);

protected:

	void Close(bool successful);

	wxString m_address;
	unsigned long m_port;
	wxEvtHandler* m_handler;
	int m_id;

	bool m_done;

	static wxString m_ip;
	static bool m_checked;

	wxString m_data;

	CSocket *m_pSocket;

	DECLARE_EVENT_TABLE();
	void OnSocketEvent(CSocketEvent& event);

	void OnConnect(int error);
	void OnClose();
	void OnReceive();
	void OnHeader();
	void OnData(char* buffer, unsigned int len);
	void OnChunkedData();
	void OnSend();

	char* m_pSendBuffer;
	unsigned int m_sendBufferPos;

	char* m_pRecvBuffer;
	unsigned int m_recvBufferPos;

	static const unsigned int m_recvBufferLen = 4096;

	// HTTP data
	void ResetHttpData(bool resetRedirectCount);
	bool m_gotHeader;
	int m_responseCode;
	wxString m_responseString;
	wxString m_location;
	int m_redirectCount;

	enum transferEncodings
	{
		identity,
		chunked,
		unknown
	};

	enum transferEncodings m_transferEncoding;

	struct t_chunkData
	{
		bool getTrailer;
		bool terminateChunk;
		wxLongLong size;
	} m_chunkData;

	bool m_finished;
};

#endif //__EXTERNALIPRESOLVER_H__
