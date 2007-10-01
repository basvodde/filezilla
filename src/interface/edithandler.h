#ifndef __EDITHANDLER_H__
#define __EDITHANDLER_H__

// Handles all aspects about remote file viewing/editing

class CQueueView;
class CEditHandler : protected wxEvtHandler
{
public:
	enum fileState
	{
		unknown = -1,
		edit,
		download,
		upload,
		upload_and_remove,
		removing
	};

	static CEditHandler* Create();
	static CEditHandler* Get();

	wxString GetLocalDirectory();

	// This tries to deletes all temporary files.
	// If files are locked, they won't be removed though
	void Release();

	enum fileState GetFileState(const wxString& fileName);

	// Returns the number of files in given state
	int GetFileCount(enum fileState state) const;

	// Adds the file that doesn't exist yet. (Has to be in unknown state)
	// The initial state will be download
	bool AddFile(const wxString& fileName, const CServerPath& remotePath, const CServer& server);

	// Tries to unedit and remove file
	bool Remove(const wxString& fileName);
	bool RemoveAll(bool force);

	void FinishTransfer(bool successful, const wxString& fileName);

	void CheckForModifications();

	void SetQueue(CQueueView* pQueue) { m_pQueue = pQueue; }

	// Checks if file can be opened. One of these conditions has to be true:
	// - Filetype association of system has to exist
	// - Custom association for that filetype
	// - Default editor set
	bool CanOpen(const wxString& fileName);

protected:
	CEditHandler();
	virtual ~CEditHandler() { }

	static CEditHandler* m_pEditHandler;

	wxString m_localDir;

	struct t_fileData
	{
		wxString name;
		fileState state;
		wxDateTime modificationTime;
		CServerPath remotePath;
		CServer server;
	};

	bool StartEditing(t_fileData &data);

	wxString GetSystemOpenCommand(const wxString& file);

	void SetTimerState();

	std::list<t_fileData> m_fileDataList;

	std::list<t_fileData>::iterator GetFile(const wxString& fileName);
	std::list<t_fileData>::const_iterator GetFile(const wxString& fileName) const;

	CQueueView* m_pQueue;

	wxTimer m_timer;

	void RemoveTemporaryFiles(const wxString& temp);

#ifdef __WXMSW__
	HANDLE m_lockfile_handle;
#endif

	DECLARE_EVENT_TABLE()
	void OnTimerEvent(wxTimerEvent& event);
};

#endif //__EDITHANDLER_H__
