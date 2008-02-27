#include "FileZilla.h"
#include "themeprovider.h"
#include "filezillaapp.h"
#include "Options.h"
#include "xmlfunctions.h"

CThemeProvider::CThemeProvider()
{
	wxArtProvider::Push(this);

	m_themePath = GetThemePath();
}

wxBitmap CThemeProvider::CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size)
{
	if (id.Left(4) != _T("ART_"))
		return wxNullBitmap;

	wxString resourceDir = wxGetApp().GetResourceDir();

	std::list<wxString> dirs;
	wxString strSize = wxString::Format(_T("%dx%d/"), size.GetWidth(), size.GetHeight());
	if (wxDir::Exists(m_themePath + strSize))
		dirs.push_back(m_themePath + strSize);
	if (wxDir::Exists(resourceDir + strSize))
		dirs.push_back(resourceDir + strSize);

	if (size.GetWidth() >= 40)
	{
		dirs.push_back(m_themePath + _T("48x48/"));
		dirs.push_back(resourceDir + _T("48x48/"));
	}
	if (size.GetWidth() >= 24)
	{
		dirs.push_back(m_themePath + _T("32x32/"));
		dirs.push_back(resourceDir + _T("32x32/"));
	}
	dirs.push_back(m_themePath + _T("16x16/"));
	dirs.push_back(resourceDir + _T("16x16/"));

	wxString name = id.Mid(4);
	name.MakeLower();

	wxLogNull logNull;

	for (std::list<wxString>::const_iterator iter = dirs.begin(); iter != dirs.end(); iter++)
	{
		wxString fileName = *iter + name + _T(".png");
		wxBitmap bmp(fileName, wxBITMAP_TYPE_PNG);
		if (bmp.Ok())
		return bmp;
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
		if (size.GetWidth() >= 40)
			path += _T("48x48/");
		else if (size.GetWidth() >= 24)
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
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
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
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
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

	const wxString& name = id.Mid(4).Lower();

	const wxChar* dirs[3] = { _T("16x16/"), _T("32x32/"), _T("48x48/") };

	const wxString& resourcePath = wxGetApp().GetResourceDir();

	for (int i = 0; i < 3; i++)
	{
		wxString file = resourcePath + dirs[i] + name + _T(".png");
		if (!wxFileName::FileExists(file))
			continue;

		iconBundle.AddIcon(wxIcon(file));
	}

	return iconBundle;
}

bool CThemeProvider::ThemeHasSize(const wxString& themePath, const wxString& size)
{
	wxFileName fn(wxGetApp().GetResourceDir() + themePath, _T("theme.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
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
	if (!wxFile::Exists(themePath + _T("theme.xml")))
	{
	    themePath = resourceDir;
		wxASSERT(wxFile::Exists(themePath + _T("theme.xml")));
	}
	return themePath;
}
