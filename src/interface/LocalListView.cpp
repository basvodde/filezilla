#include "FileZilla.h"
#include "LocalListView.h"
#include "state.h"
#include "QueueView.h"

#ifdef __WXMSW__
#include <shellapi.h>
#include <commctrl.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CLocalListView, wxListCtrl)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CLocalListView::OnItemActivated)
	EVT_LIST_COL_CLICK(wxID_ANY, CLocalListView::OnColumnClicked)
	EVT_CONTEXT_MENU(CLocalListView::OnContextMenu)
	// Map both ID_UPLOAD and ID_ADDTOQUEUE to OnMenuUpload, code is identical
	EVT_MENU(XRCID("ID_UPLOAD"), CLocalListView::OnMenuUpload)
	EVT_MENU(XRCID("ID_ADDTOQUEUE"), CLocalListView::OnMenuUpload)
	EVT_MENU(XRCID("ID_MKDIR"), CLocalListView::OnMenuMkdir)
	EVT_MENU(XRCID("ID_DELETE"), CLocalListView::OnMenuDelete)
	EVT_MENU(XRCID("ID_RENAME"), CLocalListView::OnMenuRename)
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

CLocalListView::CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueue)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxSUNKEN_BORDER)
{
	m_pState = pState;
	m_pQueue = pQueue;

	m_pImageList = 0;
	InsertColumn(0, _("Filename"));
	InsertColumn(1, _("Filesize"));
	InsertColumn(2, _("Filetype"));
	InsertColumn(3, _("Last modified"), wxLIST_FORMAT_LEFT, 100);

	m_sortColumn = 0;
	m_sortDirection = 0;

	GetImageList();

#ifdef __WXMSW__
	// Initialize imagelist for list header
	m_pHeaderImageList = new wxImageListMsw(ImageList_Create(8, 8, ILC_MASK, 3, 3));
	ImageList_SetBkColor(m_pHeaderImageList->GetHandle(), CLR_NONE);

	wxBitmap bmp;
	
	extern wxString resourcePath;
	bmp.LoadFile(resourcePath + _T("empty.xpm"), wxBITMAP_TYPE_XPM);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(resourcePath + _T("up.xpm"), wxBITMAP_TYPE_XPM);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(resourcePath + _T("down.xpm"), wxBITMAP_TYPE_XPM);
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

	TCHAR buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());
#endif
}

CLocalListView::~CLocalListView()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;
#endif
	FreeImageList();
}

void CLocalListView::DisplayDir(wxString dirname)
{
	m_dir = dirname;

	m_fileData.clear();
	m_indexMapping.clear();

	t_fileData data;
	data.dir = true;
	data.icon = -2;
	data.name = _T("..");
	data.size = -1;
	data.hasTime = 0;
	m_fileData.push_back(data);
	m_indexMapping.push_back(0);

#ifdef __WXMSW__
	if (dirname == _T("\\"))
	{
		DisplayDrives();
	}
	else
#endif
	{
		wxDir dir(dirname);
		wxString file;
		bool found = dir.GetFirst(&file);
		int num = 1;
		while (found)
		{
			wxFileName fn(dirname + file);
			t_fileData data;
			data.dir = fn.DirExists();
			data.icon = -2;
			data.name = fn.GetFullName();
	
			wxStructStat buf;
			int result;
			result = wxStat(fn.GetFullPath(), &buf);

			if (!result)
			{
				data.hasTime = true;
				data.lastModified = wxDateTime(buf.st_mtime);
			}
			else              
				data.hasTime = false;

			if (data.dir)
				data.size = -1;
			else
				data.size = result ? -1 : buf.st_size;

			m_fileData.push_back(data);
			m_indexMapping.push_back(num);
			num++;

			found = dir.GetNext(&file);
		}
		SetItemCount(num);
	}

	SortList();
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CLocalListView *>(this) and modify
// the instance.
wxString CLocalListView::OnGetItemText(long item, long column) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return _T("");

	if (!column)
		return data->name;
	else if (column == 1)
	{
		if (data->size < 0)
			return _T("");
		else
			return wxLongLong(data->size).ToString();
	}
	else if (column == 2 && item)
	{
		if (data->fileType == _T(""))
			data->fileType = pThis->GetType(data->name, data->dir);

		return data->fileType;
	}
	else if (column == 3)
	{
		if (!data->hasTime)
			return _T("");

		return data->lastModified.Format(_T("%c"));
	}
	return _T("");
}

#ifndef __WXMSW__
// This function converts to the right size with the given background colour
wxBitmap PrepareIcon(wxIcon icon, wxColour colour)
{
	wxBitmap bmp(icon.GetWidth(), icon.GetHeight());
	wxMemoryDC dc;
	dc.SelectObject(bmp);
	dc.SetPen(wxPen(colour));
	dc.SetBrush(wxBrush(colour));
	dc.DrawRectangle(0, 0, icon.GetWidth(), icon.GetHeight());
	dc.DrawIcon(icon, 0, 0);
	dc.SelectObject(wxNullBitmap);
	
	wxImage img = bmp.ConvertToImage();
	img.SetMask();
	img.Rescale(16, 16);
	return img;
}
#endif

// See comment to OnGetItemText
int CLocalListView::OnGetItemImage(long item) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return -1;
	int &icon = data->icon;
#ifdef __WXMSW__
	if (icon == -2)
	{
		wxString path;
		if (data->name == _T(".."))
			path = _T("alkjhgfdfghjjhgfdghuztxvbhzt");
		else
		{
			if (m_dir == _T("\\"))
				path = data->name + _T("\\");
			else
				path = m_dir + data->name;
		}
		icon = -1;

		SHFILEINFO shFinfo;
		memset(&shFinfo, 0, sizeof(SHFILEINFO));
		if (SHGetFileInfo( path,
			data->dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof( SHFILEINFO ),
			SHGFI_ICON | ((data->name == _T(".."))?SHGFI_USEFILEATTRIBUTES:0) ) )
		{
			icon = shFinfo.iIcon;
			// we only need the index from the system image ctrl
			DestroyIcon( shFinfo.hIcon );
		}
	}
#else
	if (icon == -2)
	{
		if (data->dir)
			icon = 1;
		else
			icon = 0;

		wxFileName fn(m_dir + data->name);
		wxString ext = fn.GetExt();

		wxFileType *pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
		if (pType)
		{
			wxIconLocation loc;
			if (pType->GetIcon(&loc) && loc.IsOk())
			{
				//wxLogNull *tmp = new wxLogNull;
				wxIcon newIcon(loc);
				
				if (newIcon.Ok())
				{
					wxBitmap bmp = PrepareIcon(newIcon, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
					int index = m_pImageList->Add(bmp);
					if (index > 0)
						icon = index;
					bmp = PrepareIcon(newIcon, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
					m_pImageList->Add(bmp);
				}
				//delete tmp;
			}
			delete pType;
		}
	}
	else if (icon > 1)
	{
		if (GetItemState(item, wxLIST_STATE_SELECTED))
			return icon + 1;
	}
#endif
	return icon;
}

void CLocalListView::GetImageList()
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

	m_pImageList = new wxImageListMsw((WXHIMAGELIST)SHGetFileInfo(buffer,
							  0,
							  &shFinfo,
							  sizeof( shFinfo ),
							  SHGFI_SYSICONINDEX |
							  ((m_nStyle)?SHGFI_ICON:SHGFI_SMALLICON) ));
#else
	m_pImageList = new wxImageList(16, 16);

	wxBitmap bmp;
	extern wxString resourcePath;
	
	wxLogNull *tmp = new wxLogNull;
	
	bmp.LoadFile(resourcePath + _T("16x16/file.png"), wxBITMAP_TYPE_PNG);
	m_pImageList->Add(bmp);

	bmp.LoadFile(resourcePath + _T("16x16/folder.png"), wxBITMAP_TYPE_PNG);
	m_pImageList->Add(bmp);

	delete tmp;
#endif
	SetImageList(m_pImageList, wxIMAGE_LIST_SMALL);
}

void CLocalListView::FreeImageList()
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

void CLocalListView::OnItemActivated(wxListEvent &event)
{
	long item = -1;
	
	int count = 0;
	bool back = false;

	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		count++;

		if (!item)
			back = true;
	}
	if (count > 1)
	{
		if (back)
		{
			wxBell();
			return;
		}

		wxCommandEvent cmdEvent;
		OnMenuUpload(cmdEvent);
		return;
	}

	item = event.GetIndex();
	
	t_fileData *data = GetData(item);
	if (!data)
		return;
	
	if (!item)
		m_pState->SetLocalDir(data->name);
	else
	{
		if (data->dir)
			m_pState->SetLocalDir(data->name);
		else
		{
			const CServer* pServer = m_pState->GetServer();
			if (!pServer)
			{
				wxBell();
				return;
			}

			CServerPath path = m_pState->GetRemotePath();
			if (path.IsEmpty())
			{
				wxBell();
				return;
			}

			wxFileName fn(m_dir, data->name);

			m_pQueue->QueueFile(false, false, fn.GetFullPath(), data->name, path, *pServer, data->size);
		}
	}
}

#ifdef __WXMSW__
void CLocalListView::DisplayDrives()
{
	TCHAR  szDrives[128];
	LPTSTR pDrive;

	GetLogicalDriveStrings( sizeof(szDrives), szDrives );
	
	pDrive = szDrives;
	int count = 1;
	while(*pDrive)
	{
		wxString path = pDrive;
		t_fileData data;
		if (path.Right(1) == _T("\\"))
			path.Truncate(path.Length() - 1);

		data.name = path;
		data.dir = true;
		data.icon = -2;
		data.size = -1;
		data.hasTime = false;
		
		m_fileData.push_back(data);
		m_indexMapping.push_back(count);
#ifdef _tcslen
		pDrive += _tcslen(pDrive) + 1;
#else
		pDrive += strlen(pDrive) + 1;
#endif
		count++;
	}
	SetItemCount(count);
}
#endif

wxString CLocalListView::GetType(wxString name, bool dir)
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
		wxString path;
#ifdef __WXMSW__
		if (m_dir == _T("\\"))
			path = name + _T("\\");
		else
#endif
			path = m_dir + name;
		SHFILEINFO shFinfo;		
		memset(&shFinfo,0,sizeof(SHFILEINFO));
		if (SHGetFileInfo(path,
			dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof(shFinfo),
			SHGFI_TYPENAME))
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

CLocalListView::t_fileData* CLocalListView::GetData(unsigned int item)
{
	if (!IsItemValid(item))
		return 0;

	return &m_fileData[m_indexMapping[item]];
}

bool CLocalListView::IsItemValid(unsigned int item) const
{
	if (item > m_indexMapping.size())
		return false;

	unsigned int index = m_indexMapping[item];
	if (index > m_fileData.size())
		return false;

	return true;
}

void CLocalListView::SortList(int column /*=-1*/, int direction /*=-1*/)
{
	if (column != -1)
	{
#ifdef __WXMSW__
		if (column != m_sortColumn && m_pHeaderImageList)
		{
			HWND hWnd = (HWND)GetHandle();
			HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

			wxChar buffer[100];
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

		wxChar buffer[100];
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

void CLocalListView::OnColumnClicked(wxListEvent &event)
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

void CLocalListView::QSortList(const unsigned int dir, unsigned int anf, unsigned int ende, int (*comp)(CLocalListView *pList, unsigned int index, t_fileData &refData))
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

int CLocalListView::CmpName(CLocalListView *pList, unsigned int index, t_fileData &refData)
{
	const t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (data.dir)
	{
		if (!refData.dir)
			return -1;
	}
	else if (refData.dir)
		return 1;

	return data.name.CmpNoCase(refData.name);
}

int CLocalListView::CmpType(CLocalListView *pList, unsigned int index, t_fileData &refData)
{
	t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (refData.fileType == _T(""))
		refData.fileType = pList->GetType(refData.name, refData.dir);
	if (data.fileType == _T(""))
		data.fileType = pList->GetType(data.name, data.dir);

	if (data.dir)
	{
		if (!refData.dir)
			return -1;
	}
	else if (refData.dir)
		return 1;

	int res = data.fileType.CmpNoCase(refData.fileType);
	if (res)
		return res;
	
	return data.name.CmpNoCase(refData.name);
}

int CLocalListView::CmpSize(CLocalListView *pList, unsigned int index, t_fileData &refData)
{
	t_fileData &data = pList->m_fileData[pList->m_indexMapping[index]];

	if (data.dir)
	{
		if (!refData.dir)
			return -1;
		else
			return data.name.CmpNoCase(refData.name);
	}
	else if (refData.dir)
		return 1;

	if (data.size == -1)
	{
		if (refData.size == -1)
			return 0;
		else
			return -1;
	}
	else if (refData.size == -1)
		return 1;

	wxLongLong res = data.size - refData.size;
	if (res > 0)
		return 1;
	else if (res < 0)
		return -1;

	return data.name.CmpNoCase(refData.name);

}

void CLocalListView::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_LOCALFILELIST"));
	if (!pMenu)
		return;

	PopupMenu(pMenu);
	delete pMenu;
}

void CLocalListView::OnMenuUpload(wxCommandEvent& event)
{
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
			return;
	}

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		t_fileData *data = GetData(item);
		if (!data)
			return;
	
		if (!item)
			m_pState->SetLocalDir(data->name);
		else
		{
			if (data->dir)
			{
				// TODO: Dir uploading
			}
			else
			{
				const CServer* pServer = m_pState->GetServer();
				if (!pServer)
				{
					wxBell();
					return;
				}

				CServerPath path = m_pState->GetRemotePath();
				if (path.IsEmpty())
				{
					wxBell();
					return;
				}

				wxFileName fn(m_dir, data->name);

				m_pQueue->QueueFile(event.GetId() == XRCID("ID_ADDTOQUEUE"), false, fn.GetFullPath(), data->name, path, *pServer, data->size);
			}
		}
	}
}

void CLocalListView::OnMenuMkdir(wxCommandEvent& event)
{
}

void CLocalListView::OnMenuDelete(wxCommandEvent& event)
{
}

void CLocalListView::OnMenuRename(wxCommandEvent& event)
{
}
