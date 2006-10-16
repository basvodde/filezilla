#ifndef __STATE_H__
#define __STATE_H__

#define STATECHANGE_REMOTE_DIR			0x0001
#define STATECHANGE_REMOTE_DIR_MODIFIED	0x0002
#define STATECHANGE_REMOTE_RECV			0x0010
#define STATECHANGE_REMOTE_SEND			0x0020
#define STATECHANGE_LOCAL_DIR			0x0100
#define STATECHANGE_APPLYFILTER			0x1000

class CDirectoryListing;
class CFileZillaEngine;
class CCommandQueue;
class CMainFrame;
class CStateEventHandler;
class CState
{
	friend class CCommandQueue;
public:
	CState(CMainFrame* pMainFrame);
	~CState();

	bool CreateEngine();
	void DestroyEngine();

	wxString GetLocalDir() const;
	bool SetLocalDir(wxString dir, wxString *error = 0);

	bool Connect(const CServer& server, bool askBreak, const CServerPath& path = CServerPath());

	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing, bool modified = false);
	const CDirectoryListing *GetRemoteDir() const;
	const CServerPath GetRemotePath() const;
	
	const CServer* GetServer() const;

	void RefreshLocal();

	void ApplyCurrentFilter();

	void RegisterHandler(CStateEventHandler* pHandler);
	void UnregisterHandler(CStateEventHandler* pHandler);

	static CState* GetState();

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;

protected:
	void SetServer(const CServer* server);
	void NotifyHandlers(unsigned int event);

	wxString m_localDir;
	const CDirectoryListing *m_pDirectoryListing;

	CServer* m_pServer;

	CMainFrame* m_pMainFrame;

	std::list<CStateEventHandler*> m_handlers;
};

class CStateEventHandler
{
public:
	CStateEventHandler(CState* pState, unsigned int eventMask);
	virtual ~CStateEventHandler();

	CState* m_pState;

	int m_eventMask;
	
	virtual void OnStateChange(unsigned int event) = 0;
};

#endif
