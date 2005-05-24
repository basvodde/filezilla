#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

class CState;
class CQueueView;

#include "systemimagelist.h"

class CLocalListView : public wxListCtrl, CSystemImageList
{
public:
	CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueue);
	virtual ~CLocalListView();

	bool DisplayDir(wxString dirname);

protected:
	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

#ifdef __WXMSW__
	void DisplayDrives();
#endif
	wxString GetType(wxString name, bool dir);

	struct t_fileData
	{
		wxString name;
		bool dir;
		int icon;
		wxLongLong size;
		wxString fileType;
		bool hasTime;
		wxDateTime lastModified;
	};

	bool IsItemValid(unsigned int item) const;
	t_fileData *GetData(unsigned int item);

	void SortList(int column = -1, int direction = -1);
	void QSortList(const unsigned int dir, unsigned int anf, unsigned int ende, int (*comp)(CLocalListView *pList, unsigned int index, t_fileData &refData));

	static int CmpName(CLocalListView *pList, unsigned int index, t_fileData &refData);
	static int CmpType(CLocalListView *pList, unsigned int index, t_fileData &refData);
	static int CmpSize(CLocalListView *pList, unsigned int index, t_fileData &refData);

	int FindItemWithPrefix(const wxString& prefix, int start);

	wxString m_dir;

	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

#ifdef __WXMSW__
	wxImageListEx *m_pHeaderImageList;
#endif

	CState *m_pState;
	CQueueView *m_pQueue;

	int m_sortColumn;
	int m_sortDirection;

	wxDateTime m_lastKeyPress;
	wxString m_prefix;

	// Event handlers
	DECLARE_EVENT_TABLE();
	void OnItemActivated(wxListEvent& event);
	void OnColumnClicked(wxListEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuUpload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnBeginLabelEdit(wxListEvent& event);
	void OnEndLabelEdit(wxListEvent& event);
};

#endif
