#ifndef __REMOTELISTVIEW_H__
#define __REMOTELISTVIEW_H__

class CState;
class CCommandQueue;

#ifdef __WXMSW__
class wxImageListMsw;
#endif

class CRemoteListView : public wxListCtrl
{
public:
	CRemoteListView(wxWindow* parent, wxWindowID id, CState *pState, CCommandQueue *pCommandQueue);
	virtual ~CRemoteListView();

	void SetDirectoryListing(CDirectoryListing *pDirectoryListing);

protected:
	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

	wxString GetType(wxString name, bool dir);

	// Event handlers
	void OnItemActivated(wxListEvent &event);
	void OnColumnClicked(wxListEvent &event);

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

	CDirectoryListing *m_pDirectoryListing;
	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

	void GetImageList();
	void FreeImageList();

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

