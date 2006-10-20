#ifndef __QUEUEVIEW_H__
#define __QUEUEVIEW_H__

#define PRIORITY_COUNT 5
enum QueuePriority
{
	priority_lowest = 0,
	priority_low = 1,
	priority_normal = 2,
	priority_high = 3,
	priority_highest = 4
};

enum QueueItemType
{
	QueueItemType_Server,
	QueueItemType_File,
	QueueItemType_FolderScan,
	QueueItemType_Status
};

#define ITEMSTATE_COUNT 4
enum ItemState
{
	ItemState_Wait = 0,
	ItemState_Active = 1,
	ItemState_Complete = 2,
	ItemState_Error = 3
};

struct t_newEntry
{
	wxString localFile;
	wxString remoteFile;
	CServerPath remotePath;
	wxLongLong size;
};

enum AcceptedTransferDirection
{
	all,
	download,
	upload
};

class TiXmlElement;
class CQueueItem
{
public:
	virtual ~CQueueItem();

	virtual void SetPriority(enum QueuePriority priority);

	void AddChild(CQueueItem* pItem);
	unsigned int GetChildrenCount(bool recursive);
	CQueueItem* GetChild(unsigned int item, bool recursive = true);
	CQueueItem* GetParent() { return m_parent; }
	const CQueueItem* GetParent() const { return m_parent; }
	virtual bool RemoveChild(CQueueItem* pItem); // Removes a child item with is somewhere in the tree of children.
	virtual bool TryRemoveAll(); // Removes a inactive childrens, queues active children for removal.
								 // Returns true if item can be removed itself
	CQueueItem* GetTopLevelItem();
	const CQueueItem* GetTopLevelItem() const;
	int GetItemIndex() const; // Return the visible item index relative to the topmost parent item.
	virtual void SaveItem(TiXmlElement* pElement) const { }

	virtual enum QueueItemType GetType() const = 0;

protected:
	CQueueItem();
	wxString GetIndent();

	CQueueItem* m_parent;

	std::vector<CQueueItem*> m_children;
	int m_visibleOffspring; // Visible offspring over all sublevels
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

	CFileItem* GetIdleChild(bool immadiateOnly, enum AcceptedTransferDirection direction);
	virtual bool RemoveChild(CQueueItem* pItem); // Removes a child item with is somewhere in the tree of children
	wxLongLong GetTotalSize(int& filesWithUnknownSize) const;

	void QueueImmediateFiles();
	void QueueImmediateFile(CFileItem* pItem);

	virtual void SaveItem(TiXmlElement* pElement) const;

	void SetDefaultFileExistsAction(int action, const enum AcceptedTransferDirection direction);

	int m_activeCount;

protected:
	CServer m_server;

	// array of item lists, sorted by priority. Used by scheduler to find
	// next file to transfer
	// First index specifies whether the item is queued (0) or immediate (1)
	std::list<CFileItem*> m_fileList[2][PRIORITY_COUNT];
};

class CFolderScanItem : public CQueueItem
{
public:
	CFolderScanItem(CServerItem* parent, bool queued, bool download, const wxString& localPath, const CServerPath& remotePath);
	virtual ~CFolderScanItem() { delete m_pDir; }

	virtual enum QueueItemType GetType() const { return QueueItemType_FolderScan; }
	wxString GetLocalPath() const { return m_localPath; }
	CServerPath GetRemotePath() const { return m_remotePath; }
	bool Download() const { return m_download; }
	bool Queued() const { return m_queued; }
	int GetCount() const { return m_count; }
	virtual bool TryRemoveAll();

	wxString m_statusMessage;

	bool m_queued;
	volatile bool m_remove;
	bool m_active;

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

	int m_defaultFileExistsAction;

protected:
	wxString m_localPath;
	CServerPath m_remotePath;
	bool m_download;
};

class CStatusItem : public CQueueItem
{
public:
	CStatusItem() {}
	~CStatusItem() {}

	virtual enum QueueItemType GetType() const { return QueueItemType_Status; }
};

class CStatusLineCtrl;
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
		transfer,
		list
	} state;

	CFileItem* pItem;
	CServer lastServer;
	CStatusLineCtrl* pStatusLineCtrl;
};

class CFileItem : public CQueueItem
{
public:
	CFileItem(CServerItem* parent, bool queued, bool download, const wxString& localFile,
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
	void SetItemState(const enum ItemState itemState);

	virtual enum QueueItemType GetType() const { return QueueItemType_File; }

	bool IsActive() const { return m_active; }
	void SetActive(bool active);
	
	virtual void SaveItem(TiXmlElement* pElement) const;

	virtual bool TryRemoveAll(); // Removes a inactive childrens, queues active children for removal.
								 // Returns true if item can be removed itself


	bool m_queued;
	int m_errorCount;
	bool m_remove;

	wxString m_statusMessage;

	CFileTransferCommand::t_transferSettings m_transferSettings;

	t_EngineData* m_pEngineData;

	int m_defaultFileExistsAction;

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
class CAsyncRequestQueue;

class CQueueView : public wxListCtrl
{
	friend class CFolderProcessingThread;
public:
	CQueueView(wxWindow* parent, wxWindowID id, CMainFrame* pMainFrame, CAsyncRequestQueue* pAsyncRequestQueue);
	virtual ~CQueueView();
	
	bool QueueFile(const bool queueOnly, const bool download, const wxString& localFile, const wxString& remoteFile,
				const CServerPath& remotePath, const CServer& server, const wxLongLong size);
	bool QueueFiles(const std::list<t_newEntry> &entryList, bool queueOnly, bool download, CServerItem* pServerItem, const int defaultFileExistsAction);
	bool QueueFolder(bool queueOnly, bool download, const wxString& localPath, const CServerPath& remotePath, const CServer& server);
	
	bool IsEmpty() const;
	int IsActive() const { return m_activeMode; }
	bool SetActive(const bool active = true);
	bool Quit();
	
	// If the settings are changed, this function will recalculate some
	// data like the list of ascii file types
	void SettingsChanged();

	// This sets the default file exists action for all files currently in queue.
	// This includes queued folders which are yet to be processed
	void SetDefaultFileExistsAction(int action, const enum AcceptedTransferDirection direction);

	// Tries to refresh the current remote directory listing
	// if there's an idle engine connected to the current server of
	// the primary connection.
	void TryRefreshListings();

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
	void ResetEngine(t_EngineData& data, const bool removeFileItem);
	void ResetItem(CFileItem* item);
	
	void RemoveItem(CQueueItem* item);
	void RemoveAll();

	// Stops processing of given item
	// Returns true on success, false if it would block
	bool StopItem(CFileItem* item); 
	bool StopItem(CServerItem* pServerItem); 
	
	void CheckQueueState();
	bool IncreaseErrorCount(t_EngineData& engineData);
	void UpdateStatusLinePositions();
	void CalculateQueueSize();
	void DisplayQueueSize();
	void SaveQueue();
	void LoadQueue();
	bool ShouldUseBinaryMode(wxString filename);

	t_EngineData* GetIdleEngine(const CServer* pServer = 0);

	std::vector<t_EngineData*> m_engineData;
	std::vector<CServerItem*> m_serverList;	
	std::list<CStatusLineCtrl*> m_statusLineList;
	
	/*
	 * List of queued folders used to populate the queue.
	 * Index 0 for downloads, index 1 for uploads.
	 * For each type, only the first one can be active at any given time.
	 */
	std::list<CFolderScanItem*> m_queuedFolders[2];

	CFolderProcessingThread *m_pFolderProcessingThread;
	
	/*
	 * Don't update status line positions if m_waitStatusLineUpdate is true.
	 * This assures we are updating the status line positions only once,
	 * and not multiple times (for example inside a loop).
	 */
	bool m_waitStatusLineUpdate;

	int m_itemCount;
	int m_activeCount;
	int m_activeCountDown;
	int m_activeCountUp;
	int m_activeMode; // 0 inactive, 1 only immediate transfers, 2 all
	bool m_quit;

	bool m_allowBackgroundErase;

	wxLongLong m_totalQueueSize;
	int m_filesWithUnknownSize;

	CMainFrame* m_pMainFrame;
	CAsyncRequestQueue* m_pAsyncRequestQueue;

	std::list<wxString> m_asciiFiles;

	DECLARE_EVENT_TABLE();

	void OnEngineEvent(wxEvent &event);
	void OnFolderThreadComplete(wxCommandEvent& event);
	void OnFolderThreadFiles(wxCommandEvent& event);
	void OnScrollEvent(wxScrollWinEvent& event);
	void OnUpdateStatusLines(wxCommandEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	
	// Context menu handlers
	void OnContextMenu(wxContextMenuEvent& event);
	void OnProcessQueue(wxCommandEvent& event);
	void OnStopAndClear(wxCommandEvent& event);
	void OnRemoveSelected(wxCommandEvent& event);
	void OnSetDefaultFileExistsAction(wxCommandEvent& event);

	void OnEraseBackground(wxEraseEvent& event);
};

#endif
