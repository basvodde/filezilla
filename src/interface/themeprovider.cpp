#include <filezilla.h>
#include "themeprovider.h"
#include "filezillaapp.h"
#include "Options.h"
#include "xmlfunctions.h"

CThemeProvider::CThemeProvider()
{
	wxArtProvider::Push(this);

	m_themePath = GetThemePath();

	RegisterOption(OPTION_THEME);
}

static wxString SubdirFromSize(const int size)
{
	return wxString::Format(_T("%dx%d/"), size, size);
}

std::list<wxString> CThemeProvider::GetSearchDirs(const wxSize& size)
{
	const int s(size.GetWidth());
	// Sort order:
	// - Current theme before general resource dir
	// - Try current size first
	// - Then try scale down next larger icon
	// - Then try scaling up next smaller icon


	int sizes[] = { 48,32,24,20,16 };

	std::list<wxString> sizeStrings;

	for (int i = 0; i < sizeof(sizes) / sizeof(int) && sizes[i] > s; ++i)
		sizeStrings.push_front(SubdirFromSize(sizes[i]));
	for (int i = 0; i < sizeof(sizes) / sizeof(int); ++i)
		if (sizes[i] < s)
			sizeStrings.push_back(SubdirFromSize(sizes[i]));

	sizeStrings.push_front(SubdirFromSize(s));

	std::list<wxString> dirs;

	for (std::list<wxString>::const_iterator it = sizeStrings.begin(); it != sizeStrings.end(); ++it)
		dirs.push_back(m_themePath + *it);

	const wxString resourceDir(wxGetApp().GetResourceDir());
	for (std::list<wxString>::const_iterator it = sizeStrings.begin(); it != sizeStrings.end(); ++it)
		dirs.push_back(resourceDir + *it);

	return dirs;
}

wxBitmap CThemeProvider::CreateBitmap(const wxArtID& id, const wxArtClient& /*client*/, const wxSize& size)
{
	if (id.Left(4) != _T("ART_"))
		return wxNullBitmap;
	wxASSERT(size.GetWidth() == size.GetHeight());

	std::list<wxString> dirs = GetSearchDirs(size);

	wxString name = id.Mid(4);

	// The ART_* IDs are always given in uppercase ASCII,
	// all filenames used by FileZilla for the resources
	// are lowercase ASCII. Locale-independent transformation
	// needed e.g. if using Turkish locale.
	MakeLowerAscii(name);

	wxLogNull logNull;

	for (std::list<wxString>::const_iterator iter = dirs.begin(); iter != dirs.end(); iter++)
	{
		wxString fileName = *iter + name + _T(".png");
//#ifdef __WXMSW__
		// MSW toolbar only greys out disabled buttons in a visually
		// pleasing way if the bitmap has an alpha channel. 
		wxImage img(fileName, wxBITMAP_TYPE_PNG);
		if (!img.Ok())
			continue;

		if (img.HasMask() && !img.HasAlpha())
			img.InitAlpha();
		if (size.IsFullySpecified())
			img.Rescale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
		return wxBitmap(img);
/*#else
		wxBitmap bmp(fileName, wxBITMAP_TYPE_PNG);
		if (bmp.Ok())
			return bmp;
#endif*/
	}

	return wxNullBitmap;
}

std::list<wxString> CThemeProvider::GetThemes()
{
	std::list<wxString> themes;

	const wxString& resourceDir = wxGetApp().GetResourceDir();
	if (wxFileName::FileExists(resourceDir + _T("theme.xml")))
		themes.push_back(_T(""));

	wxDir dir(resourceDir);
	bool found;
	wxString subdir;
	for (found = dir.GetFirst(&subdir, _T("*"), wxDIR_DIRS); found; found = dir.GetNext(&subdir))
	{
		if (wxFileName::FileExists(resourceDir + subdir + _T("/") + _T("theme.xml")))
			themes.push_back(subdir + _T("/"));
	}

	return themes;
}

std::list<wxBitmap*> CThemeProvider::GetAllImages(const wxString& theme, const wxSize& size)
{
	wxString path = wxGetApp().GetResourceDir() + theme;

	wxLogNull log;

	wxString strSize = wxString::Format(_T("%dx%d/"), size.GetWidth(), size.GetHeight());
	if (!wxDir::Exists(strSize))
	{
		if (size.GetWidth() > 32)
			path += _T("48x48/");
		else if (size.GetWidth() > 16)
			path += _T("32x32/");
		else
			path += _T("16x16/");
	}

	std::list<wxBitmap*> bitmaps;

	if (!wxDir::Exists(path))
		return bitmaps;

	wxDir dir(path);
	if (!dir.IsOpened())
		return bitmaps;

	wxBitmap* bmp = new wxBitmap;

	wxString file;
	for (bool found = dir.GetFirst(&file, _T("*.png")); found; found = dir.GetNext(&file))
	{
		if (file.Right(13) == _T("_disabled.png"))
			continue;

		wxFileName fn(path, file);
		if (bmp->LoadFile(fn.GetFullPath(), wxBITMAP_TYPE_PNG))
		{
			bitmaps.push_back(bmp);
			bmp = new wxBitmap;
		}
	}
	delete bmp;

	return bitmaps;
}

bool CThemeProvider::GetThemeData(const wxString& themePath, wxString& name, wxString& author, wxString& email)
{
	wxFileName fn(wxGetApp().GetResourceDir() + themePath, _T("theme.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath(), false);
	if (!pDocument)
		return false;

	TiXmlElement* pTheme = pDocument->FirstChildElement("Theme");
	if (pTheme)
	{
		name = GetTextElement(pTheme, "Name");
		author = GetTextElement(pTheme, "Author");
		email = GetTextElement(pTheme, "Mail");
	}
	delete pDocument->GetDocument();

	return pTheme != 0;
}

std::list<wxString> CThemeProvider::GetThemeSizes(const wxString& themePath)
{
	std::list<wxString> sizes;

	wxFileName fn(wxGetApp().GetResourceDir() + themePath, _T("theme.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath(), false);
	if (!pDocument)
		return sizes;

	TiXmlElement* pTheme = pDocument->FirstChildElement("Theme");
	if (pTheme)
	{
		for (TiXmlElement* pSize = pTheme->FirstChildElement("size"); pSize; pSize = pSize->NextSiblingElement("size"))
		{
			const char* txt = pSize->GetText();
			if (!txt)
				continue;

			wxString size = ConvLocal(txt);
			if (size == _T(""))
				continue;

			sizes.push_back(size);
		}
	}

	delete pDocument->GetDocument();

	return sizes;
}

wxIconBundle CThemeProvider::GetIconBundle(const wxArtID& id, const wxArtClient& client /*=wxART_OTHER*/)
{
	wxIconBundle iconBundle;

	if (id.Left(4) != _T("ART_"))
		return iconBundle;

	wxString name = id.Mid(4);
	MakeLowerAscii(name);

	const wxChar* dirs[3] = { _T("16x16/"), _T("32x32/"), _T("48x48/") };

	const wxString& resourcePath = wxGetApp().GetResourceDir();

	for (int i = 0; i < 3; i++)
	{
		wxString file = resourcePath + dirs[i] + name + _T(".png");
		if (!wxFileName::FileExists(file))
			continue;

		iconBundle.AddIcon(wxIcon(file, wxBITMAP_TYPE_PNG));
	}

	return iconBundle;
}

bool CThemeProvider::ThemeHasSize(const wxString& themePath, const wxString& size)
{
	wxFileName fn(wxGetApp().GetResourceDir() + themePath, _T("theme.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath(), false);
	if (!pDocument)
		return false;

	TiXmlElement* pTheme = pDocument->FirstChildElement("Theme");

	if (!pTheme)
	{
		delete pDocument->GetDocument();
		return false;
	}

	for (TiXmlElement* pSize = pTheme->FirstChildElement("size"); pSize; pSize = pSize->NextSiblingElement("size"))
	{
		const char* txt = pSize->GetText();
		if (!txt)
			continue;

		if (size == ConvLocal(txt))
		{
			delete pDocument->GetDocument();
			return true;
		}
	}

	delete pDocument->GetDocument();

	return false;
}

wxString CThemeProvider::GetThemePath()
{
	const wxString& resourceDir = wxGetApp().GetResourceDir();
	wxString themePath = resourceDir + COptions::Get()->GetOption(OPTION_THEME);
	if (wxFile::Exists(themePath + _T("theme.xml")))
		return themePath;

    themePath = resourceDir + _T("opencrystal/");
	if (wxFile::Exists(themePath + _T("theme.xml")))
		return themePath;

	wxASSERT(wxFile::Exists(resourceDir + _T("theme.xml")));
	return resourceDir;
}

void CThemeProvider::OnOptionChanged(int option)
{
	wxASSERT(option == OPTION_THEME);

	m_themePath = GetThemePath();

	wxArtProvider::Remove(this);
	wxArtProvider::Push(this);
}

wxSize CThemeProvider::GetIconSize(enum iconSize size)
{
	int s;
	if (size == iconSizeSmall)
	{
		s = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
		if (s <= 0)
			s = 16;
	}
	else if (size == iconSizeLarge)
	{
		s = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
		if (s <= 0)
			s = 48;
		else
			s *= 3;
	}
	else
	{
		s = wxSystemSettings::GetMetric(wxSYS_ICON_X);
		if (s <= 0)
			s = 32;
	}

	return wxSize(s, s);
}
