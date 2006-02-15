#ifndef __ASYNCHOSTRESOLVER_H__
#define __ASYNCHOSTRESOLVER_H__

class fzAsyncHostResolveEvent : public wxEvent
{
public:
	fzAsyncHostResolveEvent(int id = wxID_ANY);
	virtual wxEvent *Clone() const;
};

typedef void (wxEvtHandler::*fzAsyncHostResolveEventFunction)(fzAsyncHostResolveEvent&);

extern const wxEventType fzEVT_ASYNCHOSTRESOLVE;
#define EVT_FZ_ASYNCHOSTRESOLVE(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_ASYNCHOSTRESOLVE, id, -1, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( fzAsyncHostResolveEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

class CControlSocket;
class CAsyncHostResolver : public wxThread
{
public:
	CAsyncHostResolver(wxEvtHandler *pOwner, wxString hostname);
	virtual ~CAsyncHostResolver();

	wxIPV4address m_Address;
	
	wxEvtHandler *m_pOwner;

	void SetObsolete();

	bool Done() const;
	bool Obsolete() const;
	bool Successful() const;

protected:
	bool m_bDone;
	bool m_bObsolete;
	bool m_bSuccessful;
	void SendReply();
	virtual wxThread::ExitCode Entry();

	wxString m_Hostname;
};

#endif
