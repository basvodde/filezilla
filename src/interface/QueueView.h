#ifndef __QUEUEVIEW_H__
#define __QUEUEVIEW_H__

#define PRIORITY_COUNT 5
enum QueuePriority
{
	priority_lowest = 0,
	priority_low = 1,
	priority_normal = 2,
	priority_high = 3,
	priority_highest = 4,
};

enum QueueItemType
{
	QueueItemType_Server,
	QueueItemType_File,
	QueueItemType_Folder,
	QueueItemType_Status
};

enum ItemState
{
	ItemState_Wait,
	ItemState_Active,
	ItemState_Complete,
	ItemState_Error
};

struct t_newEntry
{
	wxString localFile;
	wxString remoteFile;
	wxLongLong size;
};

class CQueueItem
{
public:
	virtual ~CQueueItem();

	int Expand(bool recursive = false);
	void Collapse(bool recursive);
	bool IsExpanded() const { return m_expanded; }

	virtual void SetPriority(enum QueuePriority priority);

	void AddChild(CQueueItem* pItem);
	unsigned int GetVisibleCount() const { return m_visibleOffspring; }
	CQueueItem* GetChild(unsigned int item);
	CQueueItem* GetParent() { return m_parent; }
	const CQueueItem* GetParent() const { return m_parent; }
	virtual bool RemoveChild(CQueueItem* pItem); // Removes a child item with is somewhere in the tree of children.
	CQueueItem* GetTopLevelItem();
	const CQueueItem* GetTopLevelItem() const;
	int GetItemIndex() const; // Return the visible item index relative to the topmost parent item.

	virtual enum QueueItemType GetType() const = 0;

protected:
	CQueueItem();
	wxString GetIndent();

	CQueueItem* m_parent;

	std::vector<CQueueItem*> m_children;
	int m_visibleOffspring; // Visible offspring over all expanded sublevels
	bool m_expanded;
	wxString m_indent;
};

class CFileItem;
class CServerItem : public CQueueItem
{
public:
	CServerItem(const CServer& server);
	virtual ~CServerItem();
	virtual enum QueueItemType GetType() const { return QueueItemType_Server; }

	const CServer& GetServer() const;
	wxString GetName() const;

	void AddFileItemToList(CFileItem* pItem);

	CFileItem* GetIdleChild(bool immadiateOnly);
	virtual bool RemoveChild(CQueueItem* pItem); // Removes a child item with is somewhere in the tree of children
	wxLongLong GetTotalSize(bool& partialSizeInfo) const;

	void QueueImmediateFiles();

	int m_activeCount;

protected:
	CServer m_server;

	// array of item lists, sorted by priority. Used by scheduler to find
	// next file to transfer
	// First index specifies whether the item is queued (0) or immediate (1)
	std::list<CFileItem*> m_fileList[2][PRIORITY_COUNT];
};

class CFolderItem : public CQueueItem
{
public:
	CFolderItem(CServerItem* parent, bool queued, bool download, const wxString& localPath, const CServerPath& remotePath);
	virtual ~CFolderItem() { delete m_pDir; }

	virtual enum QueueItemType GetType() const { return QueueItemType_Folder; }
	wxString GetLocalPath() const { return m_localPath; }
	CServerPath GetRemotePath() const { return m_remotePath; }
	bool Download() const { return m_download; }
	bool Queued() const { return m_queued; }
	int GetCount() const { return m_count; }

	wxString m_statusMessage;

	bool m_queued;

	int m_count;

	struct t_dirPair
	{
		wxString localPath;
		CServerPath remotePath;
	};
	std::list<t_dirPair> m_dirsToCheck;

	wxString m_currentLocalPath;
	CServerPath m_currentRemotePath;

	// Upload members
	wxDir* m_pDir;

protected:
	wxString m_localPath;
	CServerPath m_remotePath;
	bool m_download;
};

class CStatusItem : public CQueueItem
{
public:
	CStatusItem() { m_expanded = true; }
	~CStatusItem() {}

	virtual enum QueueItemType GetType() const { return QueueItemType_Status; }
};

class CFileItem : public CQueueItem
{
public:
	CFileItem(CServerItem* parent, bool queued, bool download, const wxString& localFile,
			const wxString& remoteFile, const CServerPath& remotePath, wxLongLong size);
	CFileItem(CFolderItem* parent, bool queued, bool download, const wxString& localFile,
			const wxString& remoteFile, const CServerPath& remotePath, wxLongLong size);
	virtual ~CFileItem();

	virtual void SetPriority(enum QueuePriority priority);
	enum QueuePriority GetPriority() const;

	wxString GetLocalFile() const { return m_localFile; }
	wxString GetRemoteFile() const { return m_remoteFile; }
	CServerPath GetRemotePath() const { return m_remotePath; }
	wxLongLong GetSize() const { return m_size; }
	void SetSize(wxLongLong size) { m_size = size; }
	bool Download() const { return m_download; }
	bool Queued() const { return m_queued; }

	wxString GetIndent() { return m_indent; }

	enum ItemState GetItemState() const;
	void SetItemState(enum ItemState itemState);

	virtual enum QueueItemType GetType() const { return QueueItemType_File; }

	bool IsActive() const { return m_active; }
	void SetActive(bool active);

	bool m_queued;
	int m_errorCount;

	wxString m_statusMessage;

protected:
	enum QueuePriority m_priority;
	enum ItemState m_itemState;

	bool m_download;
	wxString m_localFile;
	wxString m_remoteFile;
	CServerPath m_remotePath;
	wxLongLong m_size;
	bool m_active;
};

class CMainFrame;
class CStatusLineCtrl;
class CFolderProcessingThread;
class CQueueView : public wxListCtrl
{
	friend class CFolderProcessingThread;
public:
	CQueueView(wxWindow* parent, wxWindowID id, CMainFrame* pMainFrame);
	virtual ~CQueueView();
	
	bool QueueFile(bool queueOnly, bool download, const wxString& localFile, const wxString& remoteFile,
				const CServerPath& remotePath, const CServer& server, wxLongLong size);
	bool QueueFiles(const std::list<t_newEntry> &entryList, bool queueOnly, bool download, const CServerPath& remotePath, const CServer& server);
	bool QueueFolder(bool queueOnly, bool download, const wxString& localPath, const CServerPath& remotePath, const CServer& server);
	
	bool IsEmpty() const;
	int IsActive() const { return m_activeMode; }
	bool SetActive(bool active = true);
	bool Quit();

	struct t_EngineData
	{
		CFileZillaEngine* pEngine;
		bool active;

		enum EngineDataState
		{
			none,
			cancel,
			disconnect,
			connect,
			transfer
		} state;
		
		CFileItem* pItem;
		CServer lastServer;
		CStatusLineCtrl* pStatusLineCtrl;
	};

protected:

	bool TryStartNextTransfer();
	bool ProcessFolderItems(int type = -1);
	void ProcessUploadFolderItems();
	
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

	CQueueItem* GetQueueItem(unsigned int item);
	CServerItem* GetServerItem(const CServer& server);
	int GetItemIndex(const CQueueItem* item);

	void ProcessReply(t_EngineData& engineData, COperationNotification* pNotification);
	void SendNextCommand(t_EngineData& engineData);
	void ResetEngine(t_EngineData& data, bool removeFileItem);
	void RemoveItem(CQueueItem* item);
	void ResetItem(CFileItem* item);
	void CheckQueueState();
	bool IncreaseErrorCount(t_EngineData& engineData);
	void UpdateStatusLinePositions();
	void UpdateQueueSize();

	std::vector<t_EngineData> m_engineData;
	std::vector<CServerItem*> m_serverList;	
	std::list<CStatusLineCtrl*> m_statusLineList;
	
	/*
	 * List of queued folders used to populate the queue.
	 * Index 0 for downloads, index 1 for uploads.
	 * For each type, only the first one can be active at any given time.
	 */
	std::list<CFolderItem*> m_queuedFolders[2];

	CFolderProcessingThread *m_pFolderProcessingThread;
	
	/*
	 * Don't update status line positions if m_waitStatusLineUpdate is true.
	 * This assures we are updating the status line positions only once,
	 * and not multiple times (for example inside a loop).
	 */
	bool m_waitStatusLineUpdate; 

	int m_itemCount;
	int m_activeCount;
	int m_activeMode; // 0 inactive, 1 only immediate transfers, 2 all
	bool m_quit;

	CMainFrame* m_pMainFrame;

	DECLARE_EVENT_TABLE();

	void OnEngineEvent(wxEvent &event);
	void OnFolderThreadComplete(wxEvent& event);
};

#endif
