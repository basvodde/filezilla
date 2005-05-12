#ifndef __SYSTEMIMAGELIST_H__
#define __SYSTEMIMAGELIST_H__

#ifdef __WXMSW__
#include <shellapi.h>
#include <commctrl.h>
#endif

// Required wxImageList extension
class wxImageListEx : public wxImageList
{
public:
	wxImageListEx();
	wxImageListEx(int width, int height, const bool mask = true, int initialCount = 1);
	virtual ~wxImageListEx() {  }

#ifdef __WXMSW__
	wxImageListEx(WXHIMAGELIST hList) { m_hImageList = hList; }
	HIMAGELIST GetHandle() const { return (HIMAGELIST)m_hImageList; }
	HIMAGELIST Detach();
#endif
};

class CSystemImageList
{
public:
	CSystemImageList(int size);
	virtual ~CSystemImageList();

	wxImageList* GetSystemImageList() { return m_pImageList; }

	// Return the icon index. Under systems other than wxMSW and an index > 1,
	// increase the index by 1 if the item is selected.
	// Returns -1 on failure.
	int GetIconIndex(bool dir, const wxString& fileName = _T(""), bool physical = true);

protected:
	wxImageListEx *m_pImageList;
};

#endif __SYSTEMIMAGELIST_H__
