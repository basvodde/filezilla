#ifndef __REMOTELISTVIEW_H__
#define __REMOTELISTVIEW_H__

#include "systemimagelist.h"

class CState;
class CCommandQueue;
class CQueueView;
class CChmodDialog;
class CRemoteListView : public wxListCtrl, CSystemImageList
{
protected:
	enum OperationMode
	{
		recursive_none,
		recursive_download,
		recursive_addtoqueue,
		recursive_delete,
		recursive_chmod
	} m_operationMode;

public:
	CRemoteListView(wxWindow* parent, wxWindowID id, CState* pState, CCommandQueue* pCommandQueue, CQueueView* pQueue);
	virtual ~CRemoteListView();

	void SetDirectoryListing(CDirectoryListing *pDirectoryListing, bool modified = false);
	void ListingFailed();

	void StopRecursiveOperation();
	bool IsBusy() const { return m_operationMode != recursive_none; }

protected:
	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

	wxString GetType(wxString name, bool dir);

	int FindItemWithPrefix(const wxString& prefix, int start);

	struct t_fileData
	{
		CDirentry *pDirEntry;
		int icon;
		wxString fileType;
	};

	bool IsItemValid(unsigned int item) const;
	t_fileData *GetData(unsigned int item);

	void SortList(int column = -1, int direction = -1);
	void QSortList(const unsigned int dir, unsigned int anf, unsigned int ende, int (*comp)(CRemoteListView *pList, unsigned int index, t_fileData &refData));

	static int CmpName(CRemoteListView *pList, unsigned int index, t_fileData &refData);
	static int CmpType(CRemoteListView *pList, unsigned int index, t_fileData &refData);
	static int CmpSize(CRemoteListView *pList, unsigned int index, t_fileData &refData);

	// Processes the directory listing in case of a recursive operation
	void ProcessDirectoryListing();
	bool NextOperation();

	// Convert permissions from rwx style into an array.
	// Permission has to be at least 9 bytes long
	bool ConvertPermissions(const wxString rwx, char* permissions);

	CDirectoryListing *m_pDirectoryListing;
	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

	// Cache icon for directories, no need to calculate it multiple times
	int m_dirIcon;

#ifdef __WXMSW__
	wxImageListEx* m_pHeaderImageList;
#endif

	CState* m_pState;
	CCommandQueue* m_pCommandQueue;
	CQueueView* m_pQueue;

	int m_sortColumn;
	int m_sortDirection;

	// Variables for recursive operations
	CServerPath m_startDir;
	std::list<CServerPath> m_visitedDirs;

	struct t_newDir
	{
		CServerPath parent;
		wxString subdir;
		wxString localDir;
	};
	std::list<t_newDir> m_dirsToVisit;
	std::list<t_newDir> m_dirsToDelete;

	wxDateTime m_lastKeyPress;
	wxString m_prefix;

	CChmodDialog* m_pChmodDlg;

	DECLARE_EVENT_TABLE()
	void OnItemActivated(wxListEvent &event);
	void OnColumnClicked(wxListEvent &event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuDownload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnBeginLabelEdit(wxListEvent& event);
	void OnEndLabelEdit(wxListEvent& event);
	void OnMenuChmod(wxCommandEvent& event);
};

#endif
