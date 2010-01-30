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
	STATECHANGE_REMOTE_LINKNOTDIR,
	STATECHANGE_LOCAL_DIR,

	// data contains name (excluding path) of file to refresh
	STATECHANGE_LOCAL_REFRESH_FILE,

	STATECHANGE_APPLYFILTER,

	STATECHANGE_REMOTE_IDLE,
	STATECHANGE_SERVER,

	STATECHANGE_SYNC_BROWSE,
	STATECHANGE_COMPARISON,

	/* Global notifications */
	STATECHANGE_QUEUEPROCESSING,
	STATECHANGE_NEWCONTEXT, /* New context created */
	STATECHANGE_CHANGEDCONTEXT, /* Currently active context changed */
	STATECHANGE_REMOVECONTEXT, /* Right before deleting a context */

	STATECHANGE_MAX
};

class CDirectoryListing;
class CFileZillaEngine;
class CCommandQueue;
class CMainFrame;
class CStateEventHandler;
class CRemoteDataObject;
class CRecursiveOperation;
class CComparisonManager;

class CState;
class CContextManager
{
	friend class CState;
public:
	// If current_only is set, only notifications from the current (at the time
	// of notification emission) context is dispatched to the handler.
	void RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification, bool current_only, bool blockable);
	void UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification);

	size_t HandlerCount(enum t_statechange_notifications notification) const;

	CState* CreateState(CMainFrame* pMainFrame);
	void DestroyState(CState* pState);
	void DestroyAllStates();

	CState* GetCurrentContext();
	const std::vector<CState*>* GetAllStates() { return &m_contexts; }

	static CContextManager* Get();

	void NotifyAllHandlers(enum t_statechange_notifications notification, const wxString& data = _T(""), const void* data2 = 0);
	void NotifyGlobalHandlers(enum t_statechange_notifications notification, const wxString& data = _T(""), const void* data2 = 0);

	void SetCurrentContext(CState* pState);

protected:
	CContextManager();

	std::vector<CState*> m_contexts;
	int m_current_context;

	struct t_handler
	{
		CStateEventHandler* pHandler;
		bool blockable;
		bool current_only;
	};
	std::list<t_handler> m_handlers[STATECHANGE_MAX];

	void NotifyHandlers(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2, bool blocked);

	static CContextManager m_the_context_manager;
};

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

	bool Connect(const CServer& server, const CServerPath& path = CServerPath());
	bool Disconnect();

	bool ChangeRemoteDir(const CServerPath& path, const wxString& subdir = _T(""), int flags = 0, bool ignore_busy = false);
	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing, bool modified = false);
	CSharedPointer<const CDirectoryListing> GetRemoteDir() const;
	const CServerPath GetRemotePath() const;

	const CServer* GetServer() const;
	wxString GetTitle() const;

	void RefreshLocal();
	void RefreshLocalFile(wxString file);
	void LocalDirCreated(const CLocalPath& path);

	bool RefreshRemote();

	void RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification, bool blockable = true);
	void UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification);

	void BlockHandlers(enum t_statechange_notifications notification);
	void UnblockHandlers(enum t_statechange_notifications notification);

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;
	CComparisonManager* GetComparisonManager() { return m_pComparisonManager; }

	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const wxString& subdir, bool queueOnly);
	void UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const CServerPath& path, bool queueOnly);
	void HandleDroppedFiles(const wxFileDataObject* pFileDataObject, const CLocalPath& path, bool copy);
	bool DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, const CLocalPath& path, bool queueOnly = false);

	static bool RecursiveCopy(CLocalPath source, const CLocalPath& targte);

	bool IsRemoteConnected() const;
	bool IsRemoteIdle() const;

	CRecursiveOperation* GetRecursiveOperationHandler() { return m_pRecursiveOperation; }

	void NotifyHandlers(enum t_statechange_notifications notification, const wxString& data = _T(""), const void* data2 = 0);

	bool SuccessfulConnect() const { return m_successful_connect; }
	void SetSuccessfulConnect() { m_successful_connect = true; }

	void ListingFailed(int error);
	void LinkIsNotDir(const CServerPath& path, const wxString& subdir);

	bool SetSyncBrowse(bool enable, const CServerPath& assumed_remote_root = CServerPath());
	bool GetSyncBrowse() const { return !m_sync_browse.local_root.empty(); }

	CServer GetLastServer() const { return m_last_server; }
	CServerPath GetLastServerPath() const { return m_last_path; }
	void SetLastServer(const CServer& server, const CServerPath& path)
	{ m_last_server = server; m_last_path = path; }

	bool GetSecurityInfo(CCertificateNotification *& pInfo);
	bool GetSecurityInfo(CSftpEncryptionNotification *& pInfo);
	void SetSecurityInfo(CCertificateNotification const& info);
	void SetSecurityInfo(CSftpEncryptionNotification const& info);
protected:
	void SetServer(const CServer* server);

	CLocalPath m_localDir;
	CSharedPointer<const CDirectoryListing> m_pDirectoryListing;

	CServer* m_pServer;
	wxString m_title;
	bool m_successful_connect;

	CServer m_last_server;
	CServerPath m_last_path;

	CMainFrame* m_pMainFrame;

	CRecursiveOperation* m_pRecursiveOperation;

	CComparisonManager* m_pComparisonManager;

	struct t_handler
	{
		CStateEventHandler* pHandler;
		bool blockable;
	};
	std::list<t_handler> m_handlers[STATECHANGE_MAX];
	bool m_blocked[STATECHANGE_MAX];

	CLocalPath GetSynchronizedDirectory(CServerPath remote_path);
	CServerPath GetSynchronizedDirectory(CLocalPath local_path);

	struct _sync_browse
	{
		CLocalPath local_root;
		CServerPath remote_root;
		bool is_changing;
		bool compare;
	} m_sync_browse;

	CCertificateNotification* m_pCertificate;
	CSftpEncryptionNotification* m_pSftpEncryptionInfo;
};

class CStateEventHandler
{
public:
	CStateEventHandler(CState* pState);
	virtual ~CStateEventHandler();

	CState* m_pState;

	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2) = 0;
};

#endif
