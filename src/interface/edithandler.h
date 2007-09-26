#ifndef __EDITHANDLER_H__
#define __EDITHANDLER_H__

// Handles all aspects about remote file viewing/editing

class CQueueView;
class CEditHandler
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

	// Returns the number of files in either known state
	int GetFileCount() const;

	// Adds the file that doesn't exist yet. (Has to be in unknown state)
	// The initial state will be download
	bool AddFile(const wxString& fileName, const CServerPath& remotePath, const CServer& server);

	// Tries to unedit and remove file
	bool Remove(const wxString& fileName);
	bool RemoveAll(const wxString& fileName);

	void FinishTransfer(bool successful, const wxString& fileName);

	void CheckForModifications();

	void SetQueue(CQueueView* pQueue) { m_pQueue = pQueue; }

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

	std::list<t_fileData> m_fileDataList;

	std::list<t_fileData>::iterator GetFile(const wxString& fileName);
	std::list<t_fileData>::const_iterator GetFile(const wxString& fileName) const;

	CQueueView* m_pQueue;
};

#endif //__EDITHANDLER_H__
