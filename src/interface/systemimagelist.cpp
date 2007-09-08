#include "FileZilla.h"
#include "systemimagelist.h"

wxImageListEx::wxImageListEx()
	: wxImageList()
{
}

wxImageListEx::wxImageListEx(int width, int height, const bool mask /*=true*/, int initialCount /*=1*/)
	: wxImageList(width, height, mask, initialCount)
{
}

#ifdef __WXMSW__
HIMAGELIST wxImageListEx::Detach()
{
	 HIMAGELIST hImageList = (HIMAGELIST)m_hImageList;
	 m_hImageList = 0;
	 return hImageList;
}
#endif

CSystemImageList::CSystemImageList(int size)
{
#ifdef __WXMSW__
	SHFILEINFO shFinfo;
	wxChar buffer[MAX_PATH + 10];
	if (!GetWindowsDirectory(buffer, MAX_PATH))
#ifdef _tcscpy
		_tcscpy(buffer, _T("C:\\"));
#else
		strcpy(buffer, _T("C:\\"));
#endif

	m_pImageList = new wxImageListEx((WXHIMAGELIST)SHGetFileInfo(buffer,
							  0,
							  &shFinfo,
							  sizeof( shFinfo ),
							  SHGFI_SYSICONINDEX |
							  ((size != 16) ? SHGFI_ICON : SHGFI_SMALLICON) ));
#else
	m_pImageList = new wxImageListEx(size, size);

	m_pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FILE"),  wxART_OTHER, wxSize(size, size)));
	m_pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(size, size)));
	m_pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(size, size)));
#endif
}

CSystemImageList::~CSystemImageList()
{
	if (!m_pImageList)
		return;

#ifdef __WXMSW__
	m_pImageList->Detach();
#endif

	delete m_pImageList;

	m_pImageList = 0;
}

#ifndef __WXMSW__
// This function converts to the right size with the given background colour
wxBitmap PrepareIcon(wxIcon icon, wxSize size)
{
	if (icon.GetWidth() == size.GetWidth() && icon.GetHeight() == size.GetHeight())
		return icon;
	wxBitmap bmp;
	bmp.CopyFromIcon(icon);
	return bmp.ConvertToImage().Rescale(size.GetWidth(), size.GetHeight());
}
#endif

int CSystemImageList::GetIconIndex(enum filetype type, const wxString& fileName /*=_T("")*/, bool physical /*=true*/)
{
#ifdef __WXMSW__
	if (fileName == _T(""))
		physical = false;

	SHFILEINFO shFinfo;
	memset(&shFinfo, 0, sizeof(SHFILEINFO));
	if (SHGetFileInfo(fileName != _T("") ? fileName : _T("{B97D3074-1830-4b4a-9D8A-17A38B074052}"),
		(type != file) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
		&shFinfo,
		sizeof(SHFILEINFO),
		SHGFI_ICON | ((type == opened_dir) ? SHGFI_OPENICON : 0) | ((physical) ? 0 : SHGFI_USEFILEATTRIBUTES) ) )
	{
		int icon = shFinfo.iIcon;
		// we only need the index from the system image list
		DestroyIcon(shFinfo.hIcon);
		return icon;
	}
#else
	int icon;
	switch (type)
	{
	case file:
	default:
		icon = 0;
		break;
	case dir:
		return 1;
	case opened_dir:
		return 2;
	}

	wxFileName fn(fileName);
	wxString ext = fn.GetExt();
	if (ext == _T(""))
		return icon;

	std::map<wxString, int>::iterator cacheIter = m_iconCache.find(ext);
	if (cacheIter != m_iconCache.end())
		return cacheIter->second;

	wxFileType *pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
	if (!pType)
	{
		m_iconCache[ext] = icon;
		return icon;
	}
	
	wxIconLocation loc;
	if (pType->GetIcon(&loc) && loc.IsOk())
	{
		wxLogNull nul;
		wxIcon newIcon(loc);

		if (newIcon.Ok())
		{
			wxBitmap bmp = PrepareIcon(newIcon, wxSize(16, 16));
			int index = m_pImageList->Add(bmp);
			if (index > 0)
				icon = index;
		}
	}
	delete pType;

	m_iconCache[ext] = icon;
	return icon;
#endif
	return -1;
}
