#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

class CState;
class CCommandQueue;
#ifdef __WXMSW__
class wxImageListMsw;
#endif
class CLocalListView : public wxListCtrl
{
public:
	CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CCommandQueue *pCommandQueue);
	virtual ~CLocalListView();

	void DisplayDir(wxString dirname);

protected:
	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

	void GetImageList();
	void FreeImageList();

#ifdef __WXMSW__
	void DisplayDrives();
#endif
	wxString GetType(wxString name, bool dir);

	// Event handlers
	void OnItemActivated(wxListEvent &event);
	void OnColumnClicked(wxListEvent &event);

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

	wxString m_dir;

	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

	wxImageList *m_pImageList;
#ifdef __WXMSW__
	wxImageListMsw *m_pHeaderImageList;
#endif

	CState *m_pState;
	CCommandQueue *m_pCommandQueue;

	int m_sortColumn;
	int m_sortDirection;

	DECLARE_EVENT_TABLE()
};

#endif
