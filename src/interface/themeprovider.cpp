#include "FileZilla.h"
#include "themeprovider.h"
#include "filezillaapp.h"
#include "Options.h"
#include "xmlfunctions.h"

CThemeProvider::CThemeProvider(COptions* pOptions)
{
	wxArtProvider::PushProvider(this);
	m_pOptions = pOptions;
}

wxBitmap CThemeProvider::CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size)
{
	wxString path = GetThemePath(m_pOptions->GetOption(OPTION_THEME));

	wxString strSize;
	if (size.GetWidth() >= 40)
		strSize = _T("48x48/");
	if (size.GetWidth() >= 24)
		strSize = _T("32x32/");
	else
		strSize = _T("16x16/");

	if (id.Left(4) != _T("ART_"))
		return wxNullBitmap;
	wxString name = id.Mid(4);
	name.MakeLower();

	wxLogNull logNull;

	wxString fileName;

	// First try provided theme
	fileName = path + strSize + name + _T(".png");
	wxBitmap bmp(fileName, wxBITMAP_TYPE_PNG);
	if (bmp.Ok())
		return bmp;

	// Try classic theme
	wxString resourceDir = wxGetApp().GetResourceDir();
	fileName = resourceDir + strSize + name + _T(".png");
	bmp = wxBitmap(fileName, wxBITMAP_TYPE_PNG);
	if (bmp.Ok())
		return bmp;

	return wxNullBitmap;
}

std::list<wxString> CThemeProvider::GetThemes()
{
	std::list<wxString> themes;

	wxFileName fn(wxGetApp().GetResourceDir(), _T("themes.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
	if (!pDocument)
	{
		wxMessageBox(_("themes.xml missing, can not get theme data"), _("Theme error"), wxICON_EXCLAMATION);
		return themes;
	}

	TiXmlElement* pThemes = pDocument->FirstChildElement("Themes");
	if (pThemes)
	{
		TiXmlElement* pTheme = pThemes->FirstChildElement("Theme");
		while (pTheme)
		{
			wxString name = GetTextElement(pTheme, "Name");
			if (name != _T(""))
				themes.push_back(name);

			pTheme = pTheme->NextSiblingElement("Theme");
		}
	}

	if (themes.empty())
		wxMessageBox(_("No themes in themes.xml"), _("Theme error"), wxICON_EXCLAMATION);

	delete pDocument->GetDocument();

	return themes;
}

std::list<wxBitmap*> CThemeProvider::GetAllImages(const wxString& theme, wxSize& size)
{
	wxString path = GetThemePath(theme);

	wxLogNull log;

	wxString strSize;
	if (size.GetWidth() >= 40)
		path += _T("48x48/");
	if (size.GetWidth() >= 24)
		path += _T("32x32/");
	else
		path += _T("16x16/");

	std::list<wxBitmap*> bitmaps;
	wxBitmap* bmp = new wxBitmap;

	wxDir dir(path);
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

wxString CThemeProvider::GetThemePath(const wxString& theme)
{
	wxFileName fn(wxGetApp().GetResourceDir(), _T("themes.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
	if (!pDocument)
		return wxGetApp().GetResourceDir();

	TiXmlElement* pThemes = pDocument->FirstChildElement("Themes");
	if (pThemes)
	{
		TiXmlElement* pTheme = pThemes->FirstChildElement("Theme");
		while (pTheme)
		{
			wxString name = GetTextElement(pTheme, "Name");
			if (name != theme)
			{
				pTheme = pTheme->NextSiblingElement("Theme");
				continue;
			}

			wxString subdir = GetTextElement(pTheme, "Subdir");
			delete pDocument->GetDocument();
			if (subdir == _T(""))
				return wxGetApp().GetResourceDir();
			else 
				return wxGetApp().GetResourceDir() + subdir + _T("/");
		}
	}

	delete pDocument->GetDocument();

	return wxGetApp().GetResourceDir();
}

bool CThemeProvider::GetThemeData(const wxString& theme, wxString& author, wxString& email)
{
	wxFileName fn(wxGetApp().GetResourceDir(), _T("themes.xml"));
	TiXmlElement* pDocument = GetXmlFile(fn.GetFullPath());
	if (!pDocument)
		return false;

	TiXmlElement* pThemes = pDocument->FirstChildElement("Themes");
	if (pThemes)
	{
		TiXmlElement* pTheme = pThemes->FirstChildElement("Theme");
		while (pTheme)
		{
			wxString name = GetTextElement(pTheme, "Name");
			if (name != theme)
			{
				pTheme = pTheme->NextSiblingElement("Theme");
				continue;
			}

			author = GetTextElement(pTheme, "Author");
			email = GetTextElement(pTheme, "Mail");
			delete pDocument->GetDocument();
			return true;
		}
	}

	delete pDocument->GetDocument();

	return false;
}
