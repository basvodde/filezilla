#ifndef __ASYNCHOSTRESOLVER_H__
#define __ASYNCHOSTRESOLVER_H__

class CControlSocket;
class CAsyncHostResolver : public wxThread
{
public:
	CAsyncHostResolver(CFileZillaEngine *pOwner, wxString hostname);
	virtual ~CAsyncHostResolver();

	wxIPV4address m_Address;
	
	CFileZillaEngine *m_pOwner;

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
