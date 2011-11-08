#ifndef __THEMEPROVIDER_H__
#define __THEMEPROVIDER_H__

#include <option_change_event_handler.h>

enum iconSize
{
	iconSizeSmall,
	iconSizeNormal,
	iconSizeLarge
};

class CThemeProvider : public wxArtProvider, protected COptionChangeEventHandler
{
public:
	CThemeProvider();
	virtual ~CThemeProvider() { }

	static std::list<wxString> GetThemes();
	static std::list<wxBitmap*> GetAllImages(const wxString& theme, const wxSize& size);
	static bool GetThemeData(const wxString& themePath, wxString& name, wxString& author, wxString& email);
	static std::list<wxString> GetThemeSizes(const wxString& themePath);
	static wxIconBundle GetIconBundle(const wxArtID& id, const wxArtClient& client = wxART_OTHER);
	static bool ThemeHasSize(const wxString& themePath, const wxString& size);

	static wxSize GetIconSize(enum iconSize size);

protected:
	static wxString GetThemePath();
	
	wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& /*client*/, const wxSize& size);

	std::list<wxString> GetSearchDirs(const wxSize& size);

	virtual void OnOptionChanged(int option);

	wxString m_themePath;
};

#endif //__THEMEPROVIDER_H__
