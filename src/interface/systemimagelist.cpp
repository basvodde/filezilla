#include <filezilla.h>
#include "systemimagelist.h"
#ifdef __WXMSW__
#include "shlobj.h"

  // Once again lots of the shell stuff is missing in MinGW's headers
  #ifndef IDO_SHGIOI_LINK
    #define IDO_SHGIOI_LINK 0x0FFFFFFE
  #endif
  #ifndef SHGetIconOverlayIndex
    extern "C" int WINAPI SHGetIconOverlayIndexW(LPCWSTR pszIconPath, int iIconIndex);
    extern "C" int WINAPI SHGetIconOverlayIndexA(LPCSTR pszIconPath, int iIconIndex);
    #if wxUSE_UNICODE
      #define SHGetIconOverlayIndex SHGetIconOverlayIndexW
    #else
      #define SHGetIconOverlayIndex SHGetIconOverlayIndexA
    #endif
  #endif
#endif
#ifndef __WXMSW__
#include <wx/rawbmp.h>
#endif

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

#ifndef __WXMSW__
static inline void AlphaComposite_Over_Inplace(wxAlphaPixelData::Iterator &a, wxAlphaPixelData::Iterator &b)
{
	// Alpha compositing of a single pixel, b gets composited over a
	// (well-known over operator), result stored in a.
	// All RGB and A values have range from 0 to 255, RGB values aren't
	// premultiplied ba A.
	// Safe for multiple compositions.

	if (!b.Alpha())
	{
		// Nothing to do
		return;
	}

	int new_alpha = a.Alpha() + b.Alpha() - a.Alpha() * b.Alpha() / 255; // Could only get 0 if both alphas were 0, caught that already.
	a.Red() = ((int)a.Red() * (255 - b.Alpha()) * a.Alpha() / 255 + (int)b.Red() * b.Alpha()) / new_alpha;
	a.Green() = ((int)a.Green() * (255 - b.Alpha()) * a.Alpha() / 255 + (int)b.Green() * b.Alpha()) / new_alpha;
	a.Blue() = ((int)a.Blue() * (255 - b.Alpha()) * a.Alpha() / 255 + (int)b.Blue() * b.Alpha()) / new_alpha;
	a.Alpha() = new_alpha;
}

static void OverlaySymlink(wxBitmap& bmp)
{
	// This is ugly, but apparently needed so that the data is _really_ in the right internal format
	bmp = bmp.ConvertToImage();
	wxBitmap symlink = wxArtProvider::GetBitmap(_T("ART_SYMLINK"),  wxART_OTHER, wxSize(bmp.GetWidth(), bmp.GetHeight())).ConvertToImage();

	wxAlphaPixelData target(bmp);
	wxAlphaPixelData source(symlink);

	int sx = bmp.GetWidth();
	if (symlink.GetWidth() < sx)
		sx = symlink.GetWidth();		
	int sy = bmp.GetHeight();
	if (symlink.GetHeight() < sy)
		sy = symlink.GetHeight();		

	// Do some rudimentary alpha copying
	wxAlphaPixelData::Iterator t(target);
	wxAlphaPixelData::Iterator s(source);
	for (int y = 0; y < sy; y++)
	{
		s.MoveTo(source, 0, y);
		t.MoveTo(target, 0, y);
		for (int x = 0; x < sx; x++, s++, t++)
			AlphaComposite_Over_Inplace(t, s);
	}
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

	wxBitmap file = wxArtProvider::GetBitmap(_T("ART_FILE"),  wxART_OTHER, wxSize(size, size));
	wxBitmap folderclosed = wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(size, size));
	wxBitmap folder = wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(size, size));
	m_pImageList->Add(file);
	m_pImageList->Add(folderclosed);
	m_pImageList->Add(folder);
	OverlaySymlink(file);
	OverlaySymlink(folderclosed);
	OverlaySymlink(folder);
	m_pImageList->Add(file);
	m_pImageList->Add(folderclosed);
	m_pImageList->Add(folder);
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

int CSystemImageList::GetIconIndex(enum filetype type, const wxString& fileName /*=_T("")*/, bool physical /*=true*/, bool symlink /*=false*/)
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
		icon = symlink ? 3 : 0;
		break;
	case dir:
		return symlink ? 4 : 1;
	case opened_dir:
		return symlink ? 5 : 2;
	}

	wxFileName fn(fileName);
	wxString ext = fn.GetExt();
	if (ext == _T(""))
		return icon;

	if (symlink)
	{
		std::map<wxString, int>::iterator cacheIter = m_iconCache.find(ext);
		if (cacheIter != m_iconCache.end())
			return cacheIter->second;
	}
	else
	{
		std::map<wxString, int>::iterator cacheIter = m_iconSymlinkCache.find(ext);
		if (cacheIter != m_iconSymlinkCache.end())
			return cacheIter->second;
	}

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
			if (symlink)
				OverlaySymlink(bmp);
			int index = m_pImageList->Add(bmp);
			if (index > 0)
				icon = index;
		}
	}
	delete pType;

	if (symlink)
		m_iconCache[ext] = icon;
	else
		m_iconSymlinkCache[ext] = icon;
	return icon;
#endif
	return -1;
}

#ifdef __WXMSW__
int CSystemImageList::GetLinkOverlayIndex()
{
	static int overlay = -1;
	if (overlay == -1)
	{
		overlay = SHGetIconOverlayIndex(0, IDO_SHGIOI_LINK);
		if (overlay < 0)
			overlay = 0;
	}

	return overlay;
}
#endif
