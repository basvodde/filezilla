#ifndef __THEMEPROVIDER_H__
#define __THEMEPROVIDER_H__

class CThemeProvider : public wxArtProvider
{
public:
	CThemeProvider();
	virtual ~CThemeProvider() { }

	static std::list<wxString> GetThemes();
	static std::list<wxBitmap*> GetAllImages(const wxString& theme, wxSize& size);
	static bool GetThemeData(const wxString& theme, wxString& author, wxString& email);

protected:
	static wxString GetThemePath(const wxString& theme);

	wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size);
};

#endif //__THEMEPROVIDER_H__
