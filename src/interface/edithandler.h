#ifndef __EDITHANDLER_H__
#define __EDITHANDLER_H__

#include "dialogex.h"

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
		upload_and_remove_failed,
		removing
	};

	enum fileType
	{
		none = -1,
		local,
		remote
	};

	static CEditHandler* Create();
	static CEditHandler* Get();

	wxString GetLocalDirectory();

	// This tries to deletes all temporary files.
	// If files are locked, they won't be removed though
	void Release();

	enum fileState GetFileState(const wxString& fileName) const; // Local files
	enum fileState GetFileState(const wxString& fileName, const CServerPath& remotePath, const CServer& server) const; // Remote files

	// Returns the number of files in given state
	// pServer may be set only if state isn't unknown
	int GetFileCount(enum fileType type, enum fileState state, const CServer* pServer = 0) const;

	// Adds the file that doesn't exist yet. (Has to be in unknown state)
	// The initial state will be download
	bool AddFile(enum fileType type, wxString& fileName, const CServerPath& remotePath, const CServer& server);

	// Tries to unedit and remove file
	bool Remove(const wxString& fileName); // Local files
	bool Remove(const wxString& fileName, const CServerPath& remotePath, const CServer& server); // Remote files
	bool RemoveAll(bool force);
	bool RemoveAll(enum fileState state, const CServer* pServer = 0);

	void FinishTransfer(bool successful, const wxString& fileName);
	void FinishTransfer(bool successful, const wxString& fileName, const CServerPath& remotePath, const CServer& server);

	void CheckForModifications(
#ifdef __WXMAC__
			bool emitEvent = false
#endif
		);

	void SetQueue(CQueueView* pQueue) { m_pQueue = pQueue; }

	/* Checks if file can be opened. One of these conditions has to be true:
	 * - Filetype association of system has to exist
	 * - Custom association for that filetype
	 * - Default editor set
	 *
	 * The dangerous argument will be set to true on some filetypes,
	 * e.g. executables.
	 */
	bool CanOpen(enum fileType type, const wxString& fileName, bool &dangerous);
	bool StartEditing(const wxString& file);
	bool StartEditing(const wxString& file, const CServerPath& remotePath, const CServer& server);
	
	struct t_fileData
	{
		wxString name; // The name of the file
		wxString file; // The actual local filename
		fileState state;
		wxDateTime modificationTime;
		CServerPath remotePath;
		CServer server;
	};

	const std::list<t_fileData>& GetFiles(enum fileType type) const { wxASSERT(type != none); return m_fileDataList[type]; }

	bool UploadFile(const wxString& file, bool unedit);
	bool UploadFile(const wxString& file, const CServerPath& remotePath, const CServer& server, bool unedit);

	wxString GetOpenCommand(const wxString& file);
	wxString GetSystemOpenCommand(const wxString& file);

protected:
	CEditHandler();
	virtual ~CEditHandler() { }

	static CEditHandler* m_pEditHandler;

	wxString m_localDir;

	bool StartEditing(enum fileType type, t_fileData &data);

	wxString GetCustomOpenCommand(const wxString& file);

	void SetTimerState();

	bool UploadFile(enum fileType type, std::list<t_fileData>::iterator iter, bool unedit);

	std::list<t_fileData> m_fileDataList[2];

	std::list<t_fileData>::iterator GetFile(const wxString& fileName);
	std::list<t_fileData>::const_iterator GetFile(const wxString& fileName) const;
	std::list<t_fileData>::iterator GetFile(const wxString& fileName, const CServerPath& remotePath, const CServer& server);
	std::list<t_fileData>::const_iterator GetFile(const wxString& fileName, const CServerPath& remotePath, const CServer& server) const;

	CQueueView* m_pQueue;

	wxTimer m_timer;

	void RemoveTemporaryFiles(const wxString& temp);

	wxString GetTemporaryFile(wxString name);
	wxString TruncateFilename(const wxString path, const wxString& name, int max);

#ifdef __WXMSW__
	HANDLE m_lockfile_handle;
#else
	int m_lockfile_descriptor;
#endif

	DECLARE_EVENT_TABLE()
	void OnTimerEvent(wxTimerEvent& event);
#ifdef __WXMAC__
	void OnChangedFileEvent(wxCommandEvent& event);
#endif
};

class CEditHandlerStatusDialog : protected wxDialogEx
{
public:
	CEditHandlerStatusDialog(wxWindow* parent);

	virtual int ShowModal();

protected:
	void SetCtrlState();

	CEditHandler::t_fileData* GetDataFromItem(int item, CEditHandler::fileType &type);

	wxWindow* m_pParent;

	DECLARE_EVENT_TABLE()
	void OnSelectionChanged(wxListEvent& event);
	void OnUnedit(wxCommandEvent& event);
	void OnUpload(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
};

#endif //__EDITHANDLER_H__
