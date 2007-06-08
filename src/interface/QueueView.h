#ifndef __QUEUEVIEW_H__
#define __QUEUEVIEW_H__

#include <set>
#include "dndobjects.h"

struct t_newEntry
{
	wxString localFile;
	wxString remoteFile;
	CServerPath remotePath;
	wxLongLong size;
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
		list,
		mkdir,
		askpassword
	} state;

	CFileItem* pItem;
	CServer lastServer;
	CStatusLineCtrl* pStatusLineCtrl;
};

class CMainFrame;
class CStatusLineCtrl;
class CFolderProcessingThread;
class CAsyncRequestQueue;
class wxAuiNotebookEx;

class CQueueView : public CQueueViewBase
{
	friend class CFolderProcessingThread;
	friend class CQueueViewDropTarget;
public:
	CQueueView(wxAuiNotebookEx* parent, wxWindowID id, CMainFrame* pMainFrame, CAsyncRequestQueue* pAsyncRequestQueue);
	virtual ~CQueueView();
	
	bool QueueFile(const bool queueOnly, const bool download, const wxString& localFile, const wxString& remoteFile,
				const CServerPath& remotePath, const CServer& server, const wxLongLong size);
	bool QueueFiles(const bool queueOnly, const wxString& localPath, const CRemoteDataObject& dataObject);
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
	void SetDefaultFileExistsAction(int action, const enum TransferDirection direction);

	// Tries to refresh the current remote directory listing
	// if there's an idle engine connected to the current server of
	// the primary connection.
	void TryRefreshListings();

	void UpdateItemSize(CFileItem* pItem, wxLongLong size);

	void RemoveAll();

protected:

	void AdvanceQueue();
	bool TryStartNextTransfer();
	bool ProcessFolderItems(int type = -1);
	void ProcessUploadFolderItems();

	void ProcessReply(t_EngineData& engineData, COperationNotification* pNotification);
	void SendNextCommand(t_EngineData& engineData);
	void ResetEngine(t_EngineData& data, const bool removeFileItem);
	void ResetItem(CFileItem* item);
	
	bool RemoveItem(CQueueItem* item);

	// Stops processing of given item
	// Returns true on success, false if it would block
	bool StopItem(CFileItem* item); 
	bool StopItem(CServerItem* pServerItem); 
	
	void CheckQueueState();
	bool IncreaseErrorCount(t_EngineData& engineData);
	void UpdateStatusLinePositions();
	void CalculateQueueSize();
	void DisplayQueueSize();
	void DisplayNumberQueuedFiles();
	void SaveQueue();
	void LoadQueue();
	bool ShouldUseBinaryMode(wxString filename);

	t_EngineData* GetIdleEngine(const CServer* pServer = 0);

	std::vector<t_EngineData*> m_engineData;
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

	// Remember last top item in UpdateStatusLinePositions()
	int m_lastTopItem;

	int m_activeCount;
	int m_activeCountDown;
	int m_activeCountUp;
	int m_activeMode; // 0 inactive, 1 only immediate transfers, 2 all
	bool m_quit;

	wxLongLong m_totalQueueSize;
	int m_filesWithUnknownSize;
	int m_queuedFiles;

	CMainFrame* m_pMainFrame;
	CAsyncRequestQueue* m_pAsyncRequestQueue;

	std::list<wxString> m_asciiFiles;

	std::list<CFileZillaEngine*> m_waitingForPassword;

	DECLARE_EVENT_TABLE();

	void OnEngineEvent(wxEvent &event);
	void OnFolderThreadComplete(wxCommandEvent& event);
	void OnFolderThreadFiles(wxCommandEvent& event);
	void OnScrollEvent(wxScrollWinEvent& event);
	void OnUpdateStatusLines(wxCommandEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnFocusItemChanged(wxListEvent& event);
	
	// Context menu handlers
	void OnContextMenu(wxContextMenuEvent& event);
	void OnProcessQueue(wxCommandEvent& event);
	void OnStopAndClear(wxCommandEvent& event);
	void OnRemoveSelected(wxCommandEvent& event);
	void OnSetDefaultFileExistsAction(wxCommandEvent& event);

	void OnAskPassword(wxCommandEvent& event);

	wxAuiNotebookEx* m_pAuiNotebook;
};

#endif
