#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

class CState;
class CLocalListView : public wxListCtrl
{
public:
	CLocalListView(wxWindow* parent, wxWindowID id, CState *pState);
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

	wxString m_dir;

	struct t_fileData
	{
		wxString name;
		bool dir;
		int icon;
		int size;
		wxString fileType;
		bool hasTime;
		wxDateTime lastModified;
	};

	std::vector<t_fileData> m_fileData;
	std::vector<int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

	wxImageList *m_pImageList;

	CState *m_pState;

	DECLARE_EVENT_TABLE()
};

#endif
