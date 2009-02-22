#ifndef __STATE_H__
#define __STATE_H__

#include "local_path.h"

enum t_statechange_notifications
{
	STATECHANGE_NONE, // Used to unregister all notifications

	STATECHANGE_REMOTE_DIR,
	STATECHANGE_REMOTE_DIR_MODIFIED,
	STATECHANGE_REMOTE_RECV,
	STATECHANGE_REMOTE_SEND,
	STATECHANGE_LOCAL_DIR,

	// data contains name (excluding path) of file to refresh
	STATECHANGE_LOCAL_REFRESH_FILE,

	STATECHANGE_APPLYFILTER,

	STATECHANGE_REMOTE_IDLE,
	STATECHANGE_SERVER,

	STATECHANGE_QUEUEPROCESSING,
	STATECHANGE_SYNC_BROWSE,

	STATECHANGE_MAX
};

class CDirectoryListing;
class CFileZillaEngine;
class CCommandQueue;
class CMainFrame;
class CStateEventHandler;
class CRemoteDataObject;
class CRecursiveOperation;

class CState
{
	friend class CCommandQueue;
public:
	CState(CMainFrame* pMainFrame);
	~CState();

	bool CreateEngine();
	void DestroyEngine();

	CLocalPath GetLocalDir() const;
	bool SetLocalDir(const wxString& dir, wxString *error = 0);

	// Returns a file:// URL
	static wxString GetAsURL(const wxString& dir);

	bool Connect(const CServer& server, bool askBreak, const CServerPath& path = CServerPath());

	bool ChangeRemoteDir(const CServerPath& path, const wxString& subdir = _T(""), int flags = 0);
	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing, bool modified = false);
	const CDirectoryListing *GetRemoteDir() const;
	const CServerPath GetRemotePath() const;

	const CServer* GetServer() const;

	void RefreshLocal();
	void RefreshLocalFile(wxString file);
	void LocalDirCreated(const CLocalPath& path);

	void RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification);
	void UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification);

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;

	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const wxString& subdir, bool queueOnly);
	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const CServerPath& path, bool queueOnly);
	void HandleDroppedFiles(const wxFileDataObject* pFileDataObject, const CLocalPath& path, bool copy);
	bool DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, const CLocalPath& path, bool queueOnly = false);

	static bool RecursiveCopy(CLocalPath source, const CLocalPath& targte);

	bool IsRemoteConnected() const;
	bool IsRemoteIdle() const;

	CRecursiveOperation* GetRecursiveOperationHandler() { return m_pRecursiveOperation; }

	void NotifyHandlers(enum t_statechange_notifications notification, const wxString& data = _T(""));

	bool SuccessfulConnect() const { return m_successful_connect; }
	void SetSuccessfulConnect() { m_successful_connect = true; }

	void ListingFailed(int error);
	void LinkIsNotDir(const CServerPath& path, const wxString& subdir);

	bool SetSyncBrowse(bool enable, const CServerPath& assumed_remote_root = CServerPath());
	bool GetSyncBrowse() const { return !m_sync_browse.local_root.empty(); }

protected:
	void SetServer(const CServer* server);

	CLocalPath m_localDir;
	const CDirectoryListing *m_pDirectoryListing;

	CServer* m_pServer;
	bool m_successful_connect;

	CMainFrame* m_pMainFrame;

	CRecursiveOperation* m_pRecursiveOperation;

	std::list<CStateEventHandler*> m_handlers[STATECHANGE_MAX];

	CLocalPath GetSynchronizedDirectory(CServerPath remote_path);
	CServerPath GetSynchronizedDirectory(CLocalPath local_path);

	struct _sync_browse
	{
		CLocalPath local_root;
		CServerPath remote_root;
		bool is_changing;
	} m_sync_browse;
};

class CStateEventHandler
{
public:
	CStateEventHandler(CState* pState);
	virtual ~CStateEventHandler();

	CState* m_pState;

	virtual void OnStateChange(enum t_statechange_notifications notification, const wxString& data) = 0;
};

#endif
