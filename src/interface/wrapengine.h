#ifndef __WRAPENGINE_H__
#define __WRAPENGINE_H__

class CWrapEngine
{
public:
	CWrapEngine();
	virtual ~CWrapEngine();

	bool LoadCache();

	// Find all wxStaticText controls in the given window(s) and wrap them, so
	// that the window has the right aspect ratio...
	bool WrapRecursive(wxWindow* wnd, double ratio, const char* name = "");
	bool WrapRecursive(std::vector<wxWindow*>& windows, double ratio, const char* name = "");

	// .. or does not exceed the given maximum length.
	bool WrapRecursive(wxWindow* wnd, wxSizer* sizer, int max);

	// Find all wxStaticText controls in the window and unwrap them
	bool UnwrapRecursive(wxWindow* wnd, wxSizer* sizer);

	// Wrap the given text so its length in pixels does not exceed maxLength
	wxString WrapText(wxWindow* parent, const wxString &text, unsigned long maxLength);
	bool WrapText(wxWindow* parent, int id, unsigned long maxLength);

	int GetWidthFromCache(const char* name);

protected:
	void SetWidthToCache(const char* name, int width);

	std::map<wxChar, unsigned int> m_charWidths;
};

#endif //__WRAPENGINE_H__
