#include "FileZilla.h"
#include "RemoteListView.h"
#include "state.h"

#ifdef __WXMSW__
#include "shellapi.h"
#include "commctrl.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CRemoteListView, wxListCtrl)
//	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CRemoteListView::OnItemActivated)
	EVT_LIST_COL_CLICK(wxID_ANY, CRemoteListView::OnColumnClicked) 
END_EVENT_TABLE()

#ifdef __WXMSW__
	// Required wxImageList extension
	class wxImageListMsw : public wxImageList
	{
	public:
		wxImageListMsw() : wxImageList() {};
		wxImageListMsw(WXHIMAGELIST hList) { m_hImageList = hList; };
		~wxImageListMsw() { m_hImageList = 0; };
		HIMAGELIST GetHandle() const { return (HIMAGELIST)m_hImageList; };
	};
#endif

CRemoteListView::CRemoteListView(wxWindow* parent, wxWindowID id, CState *pState)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxSUNKEN_BORDER)
{
	m_pState = pState;

	m_pImageList = 0;
	InsertColumn(0, _("Filename"));
	InsertColumn(1, _("Filesize"), wxLIST_FORMAT_RIGHT);
	InsertColumn(2, _("Filetype"));
	InsertColumn(3, _("Date"), wxLIST_FORMAT_LEFT, 50);
	InsertColumn(4, _("Time"), wxLIST_FORMAT_LEFT, 50);
	InsertColumn(5, _("Permissions"));
	InsertColumn(6, _("Owner / Group"));


	m_sortColumn = 0;
	m_sortDirection = 0;

	GetImageList();

#ifdef __WXMSW__
	// Initialize imagelist for list header
	m_pHeaderImageList = new wxImageListMsw(ImageList_Create(8, 8, ILC_MASK, 3, 3));
	ImageList_SetBkColor(m_pHeaderImageList->GetHandle(), CLR_NONE);

	wxBitmap bmp;
	
	extern wxString resourcePath;
	bmp.LoadFile(resourcePath + _T("/resources/empty.xpm"), wxBITMAP_TYPE_XPM);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(resourcePath + _T("/resources/up.xpm"), wxBITMAP_TYPE_XPM);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(resourcePath + _T("/resources/down.xpm"), wxBITMAP_TYPE_XPM);
	m_pHeaderImageList->Add(bmp);

	HWND hWnd = (HWND)GetHandle();
	if (!hWnd)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);
	if (!header)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	char buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());
#endif
}

CRemoteListView::~CRemoteListView()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;
#endif
	FreeImageList();
}

void CRemoteListView::GetImageList()
{
	if (m_pImageList)
		return;

#ifdef __WXMSW__
	SHFILEINFO shFinfo;	
	int m_nStyle = 0;
	wxChar buffer[MAX_PATH + 10];
	if (!GetWindowsDirectory(buffer, MAX_PATH))
#ifdef _tcscpy
		_tcscpy(buffer, _T("C:\\"));
#else
		strcpy(buffer, _T("C:\\"));
#endif

	m_pImageList = new wxImageListMsw((WXHIMAGELIST)SHGetFileInfo( buffer,
							  0,
							  &shFinfo,
							  sizeof( shFinfo ),
							  SHGFI_SYSICONINDEX |
							  ((m_nStyle)?SHGFI_ICON:SHGFI_SMALLICON) ));
#else
	m_pImageList = new wxImageList(16, 16);
	//TODO: Fill with icons
#endif
	SetImageList(m_pImageList, wxIMAGE_LIST_SMALL);
}

void CRemoteListView::FreeImageList()
{
	if (!m_pImageList)
		return;

#ifdef __WXMSW__
	wxImageListMsw *pList = reinterpret_cast<wxImageListMsw *>(m_pImageList);
	delete pList;
#else
	delete m_pImageList;
#endif

	m_pImageList = 0;
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CRemoteListView *>(this) and modify
// the instance.
wxString CRemoteListView::OnGetItemText(long item, long column) const
{
	CRemoteListView *pThis = const_cast<CRemoteListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return _T("");

	if (!column)
	{
		if (!data->pDirEntry)
			return _T("..");
		else
            return data->pDirEntry->name;
	}
	if (!item)
		return _T(""); //.. has no attributes

	if (column == 1)
	{
		if (data->pDirEntry->dir || data->pDirEntry->size < 0)
			return _T("");
		else
			return data->pDirEntry->size.ToString();
	}
	else if (column == 2)
	{
		if (data->fileType == _T(""))
			data->fileType = pThis->GetType(data->pDirEntry->name, data->pDirEntry->dir);

		return data->fileType;
	}
	else if (column == 3)
	{
		if (!data->pDirEntry->hasDate)
			return _T("");

		return wxString::Format(_T("%04d-%02d-%02d"), data->pDirEntry->date.year, data->pDirEntry->date.month, data->pDirEntry->date.day);
	}
	else if (column == 4)
	{
		if (!data->pDirEntry->hasTime)
			return _T("");

		return wxString::Format(_T("%02d:%02d"), data->pDirEntry->time.hour, data->pDirEntry->time.minute);
	}
	else if (column == 5)
		return data->pDirEntry->permissions;
	else if (column == 6)
		return data->pDirEntry->ownerGroup;
	return _T("");
}

// See comment to OnGetItemText
int CRemoteListView::OnGetItemImage(long item) const
{
    CRemoteListView *pThis = const_cast<CRemoteListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return -1;
	int &icon = data->icon;
	if (icon == -2)
	{
#ifdef __WXMSW__
		wxString path;
		bool bDir;
		if (!data->pDirEntry)
		{
			path = _T("{B97D3074-1830-4b4a-9D8A-17A38B074052}");
			bDir = true;
		}
		else
		{
			path = data->pDirEntry->name;
			bDir = data->pDirEntry->dir;
		}
		icon = -1;

		SHFILEINFO shFinfo;
		memset(&shFinfo, 0, sizeof(SHFILEINFO));
		if (SHGetFileInfo(path,
			bDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof(SHFILEINFO),
			SHGFI_ICON | SHGFI_USEFILEATTRIBUTES))
		{
			icon = shFinfo.iIcon;
			// we only need the index from the system image ctrl
			DestroyIcon( shFinfo.hIcon );
		}
#else
		icon = -1;
#endif
	}	
	return icon;
}

CRemoteListView::t_fileData* CRemoteListView::GetData(unsigned int item)
{
	if (!IsItemValid(item))
		return 0;

	return &m_fileData[m_indexMapping[item]];
}

bool CRemoteListView::IsItemValid(unsigned int item) const
{
	if (item > m_indexMapping.size())
		return false;

	unsigned int index = m_indexMapping[item];
	if (index > m_fileData.size())
		return false;

	return true;
}

void CRemoteListView::SetDirectoryListing(CDirectoryListing *pDirectoryListing)
{
	m_pDirectoryListing = pDirectoryListing;

	m_fileData.clear();
	m_indexMapping.clear();

	t_fileData data;
	data.icon = -2;
	data.pDirEntry = 0;
	m_fileData.push_back(data);
	m_indexMapping.push_back(0);

	for (int i = 0; i < m_pDirectoryListing->m_entryCount; i++)
	{
		t_fileData data;
		data.icon = -2;
		data.pDirEntry = &m_pDirectoryListing->m_pEntries[i];
		m_fileData.push_back(data);
		m_indexMapping.push_back(i + 1);
	}

	SetItemCount(m_fileData.size());

	SortList();
}

void CRemoteListView::SortList(int column /*=-1*/, int direction /*=-1*/)
{
	if (column != -1)
	{
#ifdef __WXMSW__
		if (column != m_sortColumn && m_pHeaderImageList)
		{
			HWND hWnd = (HWND)GetHandle();
			HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

			char buffer[100];
			HDITEM item;
			item.mask = HDI_TEXT | HDI_FORMAT;
			item.pszText = buffer;
			item.cchTextMax = 99;
			SendMessage(header, HDM_GETITEM, m_sortColumn, (LPARAM)&item);
			item.mask |= HDI_IMAGE;
			item.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
			item.iImage = 0;
			SendMessage(header, HDM_SETITEM, m_sortColumn, (LPARAM)&item);
		}
#endif
		m_sortColumn = column;
	}
	if (direction != -1)
		m_sortDirection = direction;

#ifdef __WXMSW__
	if (m_pHeaderImageList)
	{
		HWND hWnd = (HWND)GetHandle();
		HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

		char buffer[100];
		HDITEM item;
		item.mask = HDI_TEXT | HDI_FORMAT;
		item.pszText = buffer;
		item.cchTextMax = 99;
		SendMessage(header, HDM_GETITEM, m_sortColumn, (LPARAM)&item);
		item.mask |= HDI_IMAGE;
		item.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
		item.iImage = m_sortDirection ? 2 : 1;
		SendMessage(header, HDM_SETITEM, m_sortColumn, (LPARAM)&item);
	}
#endif

	if (GetItemCount() < 3)
		return;

	if (!m_sortColumn)
		QSortList(m_sortDirection, 1, m_fileData.size() - 1, CmpName);
	else if (m_sortColumn == 1)
		QSortList(m_sortDirection, 1, m_fileData.size() - 1, CmpSize);
	else if (m_sortColumn == 2)
		QSortList(m_sortDirection, 1, m_fileData.size() - 1, CmpType);
	RefreshItems(1, m_fileData.size() - 1);
}

void CRemoteListView::OnColumnClicked(wxListEvent &event)
{
	int col = event.GetColumn();
	if (col == -1)
		return;

	int dir;
	if (col == m_sortColumn)
		dir = m_sortDirection ? 0 : 1;
	else
		dir = m_sortDirection;
	
	SortList(col, dir);
}

void CRemoteListView::QSortList(const unsigned int dir, unsigned int anf, unsigned int ende, int (*comp)(CRemoteListView *pList, unsigned int index, t_fileData &refData))
{
	unsigned int l = anf;
	unsigned int r = ende;
	const unsigned int ref = (l + r) / 2;
	t_fileData &refData = m_fileData[m_indexMapping[ref]];
	do
    {
		if (!dir)
		{
			while ((comp(this, l, refData) < 0) && (l<ende)) l++;
			while ((comp(this, r, refData) > 0) && (r>anf)) r--;
		}
		else
		{
			while ((comp(this, l, refData) > 0) && (l<ende)) l++;
			while ((comp(this, r, refData) < 0) && (r>anf)) r--;
		}
		if (l<=r)
		{
			unsigned int tmp = m_indexMapping[l];
			m_indexMapping[l] = m_indexMapping[r];
			m_indexMapping[r] = tmp;
			l++;
			r--;
		}
    } 
	while (l<=r);

	if (anf<r) QSortList(dir, anf, r, comp);
	if (l<ende) QSortList(dir, l, ende, comp);
}

int CRemoteListView::CmpName(CRemoteListView *pList, unsigned int index, t_fileData &refData)
{
	const t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (data.pDirEntry->dir)
	{
		if (!refData.pDirEntry->dir)
			return -1;
	}
	else if (refData.pDirEntry->dir)
		return 1;

	return data.pDirEntry->name.CmpNoCase(refData.pDirEntry->name);
}

wxString CRemoteListView::GetType(wxString name, bool dir)
{
	wxString type;
#ifdef __WXMSW__
	wxString ext = wxFileName(name).GetExt();
	ext.MakeLower();
	std::map<wxString, wxString>::iterator typeIter = m_fileTypeMap.find(ext);
	if (typeIter != m_fileTypeMap.end())
		type = typeIter->second;
	else
	{
		SHFILEINFO shFinfo;		
		memset(&shFinfo,0,sizeof(SHFILEINFO));
		if (SHGetFileInfo(name,
			dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof(shFinfo),
			SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
		{
			type = shFinfo.szTypeName;
			if (type == _T(""))
			{
				type = ext;
				type.MakeUpper();
				if (!type.IsEmpty())
				{
					type += _T("-");
					type += _("file");
				}
				else
					type = _("File");
			}
			else
			{
				if (!dir && ext != _T(""))
					m_fileTypeMap[ext.MakeLower()] = type;
			}
		}
		else
		{
			type = ext;
			type.MakeUpper();
			if (!type.IsEmpty())
			{
				type += _T("-");
				type += _("file");
			}
			else
				type = _("File");
		}
	}
#else
	type = dir ? _("Folder") : _("File");
#endif
	return type;
}

int CRemoteListView::CmpType(CRemoteListView *pList, unsigned int index, t_fileData &refData)
{
	t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (refData.fileType == _T(""))
		refData.fileType = pList->GetType(refData.pDirEntry->name, refData.pDirEntry->dir);
	if (data.fileType == _T(""))
		data.fileType = pList->GetType(data.pDirEntry->name, data.pDirEntry->dir);

	if (data.pDirEntry->dir)
	{
		if (!refData.pDirEntry->dir)
			return -1;
	}
	else if (refData.pDirEntry->dir)
		return 1;

	int res = data.fileType.CmpNoCase(refData.fileType);
	if (res)
		return res;
	
	return data.pDirEntry->name.CmpNoCase(refData.pDirEntry->name);
}

int CRemoteListView::CmpSize(CRemoteListView *pList, unsigned int index, t_fileData &refData)
{
	t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (data.pDirEntry->dir)
	{
		if (!refData.pDirEntry->dir)
			return -1;
		else
			return data.pDirEntry->name.CmpNoCase(refData.pDirEntry->name);
	}
	else if (refData.pDirEntry->dir)
		return 1;

	if (data.pDirEntry->size == -1)
	{
		if (refData.pDirEntry->size == -1)
			return 0;
		else
			return -1;
	}
	else if (refData.pDirEntry->size == -1)
		return 1;

	wxLongLong res = data.pDirEntry->size - refData.pDirEntry->size;
	if (res > 0)
		return 1;
	else if (res < 0)
		return -1;

	return data.pDirEntry->name.CmpNoCase(refData.pDirEntry->name);

}

