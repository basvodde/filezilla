#include "FileZilla.h"
#include <wx/aui/aui.h>
#include "aui_notebook_ex.h"

class wxAuiTabArtEx : public wxAuiDefaultTabArt
{
public:
	virtual wxAuiTabArt* Clone()
	{
		return new wxAuiTabArtEx();
	}

	virtual wxSize GetTabSize(wxDC& dc, wxWindow* wnd, const wxString& caption, const wxBitmap& bitmap, bool active, int close_button_state, int* x_extent)
	{
		wxSize size = wxAuiDefaultTabArt::GetTabSize(dc, wnd, caption, bitmap, active, close_button_state, x_extent);

		wxString text = caption;
		int pos;
		if ((pos = caption.Find(_T(" ("))) != -1)
			text = text.Left(pos);
		std::map<wxString, int>::iterator iter = m_maxSizes.find(text);
		if (iter == m_maxSizes.end())
			m_maxSizes[text] = size.x;
		else
		{
			if (iter->second > size.x)
			{
				size.x = iter->second;
				*x_extent = size.x;
			}
			else
				iter->second = size.x;
		}

		return size;
	};

protected:

	static std::map<wxString, int> m_maxSizes;
};

std::map<wxString, int> wxAuiTabArtEx::m_maxSizes;

wxAuiNotebookEx::wxAuiNotebookEx()
{
}

wxAuiNotebookEx::~wxAuiNotebookEx()
{
}

void wxAuiNotebookEx::RemoveExtraBorders()
{
	wxAuiPaneInfoArray& panes = m_mgr.GetAllPanes();
	for (size_t i = 0; i < panes.Count(); i++)
	{
		panes[i].PaneBorder(false);
	}
	m_mgr.Update();
}

void wxAuiNotebookEx::SetExArtProvider()
{
	SetArtProvider(new wxAuiTabArtEx);
}
