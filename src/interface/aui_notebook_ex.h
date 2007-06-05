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
};

#endif //__AUI_NOTEBOOK_EX_H__
