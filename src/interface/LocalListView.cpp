#include "FileZilla.h"
#include "LocalListView.h"
#include "state.h"
#include <sys/stat.h>

#ifdef __WXMSW__
#include "shellapi.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CLocalListView, wxListCtrl)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CLocalListView::OnItemActivated)
END_EVENT_TABLE()

CLocalListView::CLocalListView(wxWindow* parent, wxWindowID id, CState *pState)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxSUNKEN_BORDER)
{
	m_pState = pState;

	m_pImageList = 0;
	InsertColumn(0, _("Filname"));
	InsertColumn(1, _("Filesize"));
	InsertColumn(2, _("Filetype"));
	InsertColumn(3, _("Last modified"));
}

CLocalListView::~CLocalListView()
{
	FreeImageList();
}

void CLocalListView::DisplayDir(wxString dirname)
{
	m_dir = dirname;

	m_fileData.clear();
	m_indexMapping.clear();

	FreeImageList();
	GetImageList();

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
		return;
	}
#endif

	wxDir dir(dirname);
	wxString file;
	bool found = dir.GetFirst(&file);
	int num = 1;
	while (found)
	{
		wxFileName fn(file);
		t_fileData data;
		data.dir = wxDir::Exists(file + _T("/"));
		data.icon = -2;
		data.name = fn.GetFullName();

		wxStructStat buf;
		int result;
		result = wxStat(file, &buf);
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

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CLocalListView *>(this) and modify
// the instance.
wxString CLocalListView::OnGetItemText(long item, long column) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData &data = pThis->m_fileData[item];

	if (!column)
		return data.name;
	else if (column == 1)
	{
		if (data.size < 0)
			return _T("");
		else
			return wxString::Format(_T("%d"), data.size);
	}
	else if (column == 2 && item)
	{
		if (data.fileType == _T(""))
			data.fileType = pThis->GetType(data.name, data.dir);

		return data.fileType;
	}
	else if (column == 3)
	{
		if (!data.hasTime)
			return _T("");

		return data.lastModified.Format(_T("%c"));
	}
	return _T("");
}

// See comment to OnGetItemText
int CLocalListView::OnGetItemImage(long item) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData &data = pThis->m_fileData[item];
	int &icon = pThis->m_fileData[item].icon;
	if (icon == -2)
	{
#ifdef __WXMSW__
		wxString path;
		if (data.name == _T(".."))
			path = _T("alkjhgfdfghjjhgfdghuztxvbhzt");
		else
		{
#ifdef __WXMSW__
			if (m_dir == _T("\\"))
				path = data.name + _T("\\");
			else
#endif
				path = m_dir + data.name;
		}
		icon = -1;

		SHFILEINFO shFinfo;
		memset(&shFinfo, 0, sizeof(SHFILEINFO));
		if (SHGetFileInfo( path,
			data.dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof( SHFILEINFO ),
			SHGFI_ICON | ((data.name == _T(".."))?SHGFI_USEFILEATTRIBUTES:0) ) )
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

#ifdef __WXMSW__
	class wxImageListMsw : public wxImageList
	{
	public:
		wxImageListMsw(WXHIMAGELIST hList) { m_hImageList = hList; };
		~wxImageListMsw() { m_hImageList = 0; };
	};
#endif

void CLocalListView::GetImageList()
{
	if (m_pImageList)
		return;

#ifdef __WXMSW__
	SHFILEINFO shFinfo;	
	int m_nStyle = 0;
	wxChar buffer[MAX_PATH + 10];
	if (!GetWindowsDirectory(buffer, MAX_PATH))
		_tcscpy(buffer, _T("C:\\"));

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
	m_pState->SetLocalDir(event.GetText());
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
		pDrive += _tcslen(pDrive) + 1;
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
	type = dir ? _("File") : _("Folder");
#endif
	return type;
}
