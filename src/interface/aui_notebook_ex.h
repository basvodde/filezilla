#ifndef __AUI_NOTEBOOK_EX_H__
#define __AUI_NOTEBOOK_EX_H__

#include <wx/aui/aui.h>

class wxAuiTabArtEx;
class wxAuiNotebookEx : public wxAuiNotebook
{
public:
	wxAuiNotebookEx();
	virtual ~wxAuiNotebookEx();

	void RemoveExtraBorders();

	void SetExArtProvider();

	// Basically identical to the AUI one, but not calling Update
	bool SetPageText(size_t page_idx, const wxString& text);
};

#endif //__AUI_NOTEBOOK_EX_H__
