#ifndef __STATE_H__
#define __STATE_H__

#define STATECHANGE_REMOTE_DIR			0x0001
#define STATECHANGE_REMOTE_DIR_MODIFIED	0x0002
#define STATECHANGE_REMOTE_RECV			0x0010
#define STATECHANGE_REMOTE_SEND			0x0020
#define STATECHANGE_LOCAL_DIR			0x0100

// data contains name (excluding path) of file to refresh
#define STATECHANGE_LOCAL_REFRESH_FILE	0x0200

#define STATECHANGE_APPLYFILTER			0x1000

class CDirectoryListing;
class CFileZillaEngine;
class CCommandQueue;
class CMainFrame;
class CStateEventHandler;
class CRemoteDataObject;
class CState
{
	friend class CCommandQueue;
public:
	CState(CMainFrame* pMainFrame);
	~CState();

	bool CreateEngine();
	void DestroyEngine();

	wxString GetLocalDir() const;
	static wxString Canonicalize(wxString oldDir, wxString newDir, wxString *error = 0);
	bool SetLocalDir(wxString dir, wxString *error = 0);

	// These functions only operate on the path syntax, they don't
	// check the actual filesystem properties.
	// Passed directory should be in canonical form.
	static bool LocalDirHasParent(const wxString& dir);
	static bool LocalDirIsWriteable(const wxString& dir);

	bool Connect(const CServer& server, bool askBreak, const CServerPath& path = CServerPath());

	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing, bool modified = false);
	const CDirectoryListing *GetRemoteDir() const;
	const CServerPath GetRemotePath() const;
	
	const CServer* GetServer() const;

	void RefreshLocal();
	void RefreshLocalFile(wxString file);

	void ApplyCurrentFilter();

	void RegisterHandler(CStateEventHandler* pHandler);
	void UnregisterHandler(CStateEventHandler* pHandler);

	static CState* GetState();

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;

	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const wxString& subdir, bool queueOnly);
	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const CServerPath& path, bool queueOnly);
	void HandleDroppedFiles(const wxFileDataObject* pFileDataObject, wxString path, bool copy);
	bool DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, wxString path, bool queueOnly = false);

	static bool RecursiveCopy(wxString source, wxString target);

	bool IsConnected() const;
	bool IsIdle() const;

protected:
	void SetServer(const CServer* server);
	void NotifyHandlers(unsigned int event, const wxString& data = _T(""));

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
	
	virtual void OnStateChange(unsigned int event, const wxString& data) = 0;
};

#endif
