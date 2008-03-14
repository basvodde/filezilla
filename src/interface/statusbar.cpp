#include "FileZilla.h"
#include "statusbar.h"

static const int statbarWidths[6] = {
#ifdef __WXMSW__
	-2, 90, -1, 150, -1, 41
#else
	-2, 90, -1, 150, -1, 50
#endif
};

BEGIN_EVENT_TABLE(CStatusBar, wxStatusBar)
EVT_SIZE(CStatusBar::OnSize)
END_EVENT_TABLE()

CStatusBar::CStatusBar(wxTopLevelWindow* pParent)
{
	m_pParent = pParent;
	Create(pParent, wxID_ANY);

	const int count = 6;
	SetFieldsCount(count);
	int array[count];
	for (int i = 1; i < 5; i++)
		array[i] = wxSB_NORMAL;
	array[0] = wxSB_FLAT;
	array[count - 1] = wxSB_FLAT;
	SetStatusStyles(count, array);

	SetStatusWidths(count, statbarWidths);

#ifdef __WXMSW__
	m_parentWasMaximized = false;
#endif
}

void CStatusBar::AddChild(int field, wxWindow* pChild, int cx)
{
	t_statbar_child data;
	if (field >= 0)
	{
		wxASSERT(field <= GetFieldsCount());
		data.field = field;
	}
	else
	{
		data.field = GetFieldsCount() + field;
		wxASSERT(data.field >= 0);
	}
	data.pChild = pChild;
	data.cx = cx;

	m_children.push_back(data);
}

void CStatusBar::OnSize(wxSizeEvent& event)
{
#ifdef __WXMSW__
	// No sizegrip on maximized windows
	bool isMaximized = m_pParent->IsMaximized();
	if (isMaximized != m_parentWasMaximized)
	{
		m_parentWasMaximized = isMaximized;
				
		int count = GetFieldsCount();
		int* widths = new int[count];
		memcpy(widths, statbarWidths, count * sizeof(int));
		if (isMaximized)
			widths[count - 1] -= 6;
		SetStatusWidths(count, widths);
		Refresh();

		delete [] widths;
	}
#endif

	for (std::list<struct t_statbar_child>::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		t_statbar_child& data = *iter;

		const wxSize size = data.pChild->GetSize();

		wxRect rect;
		GetFieldRect(data.field, rect);

		data.pChild->SetSize(rect.x + data.cx, rect.GetTop() + (rect.GetHeight() - size.x) / 2 + 1, -1, -1);
	}
}
