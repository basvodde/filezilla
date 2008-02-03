#ifndef __THEMEPROVIDER_H__
#define __THEMEPROVIDER_H__

class CThemeProvider : public wxArtProvider
{
public:
	CThemeProvider();
	virtual ~CThemeProvider() { }

	static std::list<wxString> GetThemes();
	static std::list<wxBitmap*> GetAllImages(const wxString& theme, const wxSize& size);
	static bool GetThemeData(const wxString& theme, wxString& author, wxString& email);
	static std::list<wxString> GetThemeSizes(const wxString& theme);
	static wxIconBundle GetIconBundle(const wxArtID& id, const wxArtClient& client = wxART_OTHER);
	static bool ThemeHasSize(const wxString& theme, const wxString& size);

protected:
	static wxString GetThemePath(const wxString& theme);

	wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size);

	wxString m_themePath;
};

#endif //__THEMEPROVIDER_H__
