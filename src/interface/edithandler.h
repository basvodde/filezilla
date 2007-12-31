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

	enum fileState GetFileState(enum fileType type, const wxString& fileName) const;

	// Returns the number of files in given state
	int GetFileCount(enum fileType type, enum fileState state) const;

	// Adds the file that doesn't exist yet. (Has to be in unknown state)
	// The initial state will be download
	bool AddFile(enum fileType type, const wxString& fileName, const CServerPath& remotePath, const CServer& server);

	// Tries to unedit and remove file
	bool Remove(enum fileType type, const wxString& fileName);
	bool RemoveAll(bool force);

	void FinishTransfer(bool successful, enum fileType type, const wxString& fileName);

	void CheckForModifications();

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
	bool StartEditing(enum fileType type, const wxString& file);

	struct t_fileData
	{
		wxString name;
		fileState state;
		wxDateTime modificationTime;
		CServerPath remotePath;
		CServer server;
	};

	const std::list<t_fileData>& GetFiles(enum fileType type) const { wxASSERT(type != none); return m_fileDataList[type]; }

	bool UploadFile(enum fileType type, const wxString& fileName, bool unedit);

	wxString GetSystemOpenCommand(const wxString& file);

protected:
	CEditHandler();
	virtual ~CEditHandler() { }

	static CEditHandler* m_pEditHandler;

	wxString m_localDir;

	bool StartEditing(enum fileType type, t_fileData &data);

	wxString GetOpenCommand(const wxString& file);
	wxString GetCustomOpenCommand(const wxString& file);

	void SetTimerState();

	std::list<t_fileData> m_fileDataList[2];

	std::list<t_fileData>::iterator GetFile(enum fileType type, const wxString& fileName);
	std::list<t_fileData>::const_iterator GetFile(enum fileType type, const wxString& fileName) const;

	CQueueView* m_pQueue;

	wxTimer m_timer;

	void RemoveTemporaryFiles(const wxString& temp);

#ifdef __WXMSW__
	HANDLE m_lockfile_handle;
#else
	int m_lockfile_descriptor;
#endif

	DECLARE_EVENT_TABLE()
	void OnTimerEvent(wxTimerEvent& event);
};

class CEditHandlerStatusDialog : protected wxDialogEx
{
public:
	CEditHandlerStatusDialog(wxWindow* parent);

	virtual int ShowModal();

protected:
	void SetCtrlState();

	wxWindow* m_pParent;

	DECLARE_EVENT_TABLE()
	void OnSelectionChanged(wxListEvent& event);
	void OnUnedit(wxCommandEvent& event);
	void OnUpload(wxCommandEvent& event);
	void OnUploadAndUnedit(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
};

#endif //__EDITHANDLER_H__
