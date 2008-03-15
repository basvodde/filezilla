#include "FileZilla.h"
#include "statusbar.h"

static const int statbarWidths[6] = {
	-2, 90, -1, 150, -1, 35
};

BEGIN_EVENT_TABLE(wxStatusBarEx, wxStatusBar)
EVT_SIZE(wxStatusBarEx::OnSize)
END_EVENT_TABLE()

wxStatusBarEx::wxStatusBarEx(wxTopLevelWindow* pParent)
{
	m_pParent = pParent;
	m_columnWidths = 0;
	Create(pParent, wxID_ANY);

#ifdef __WXMSW__
	m_parentWasMaximized = false;
#endif
}

wxStatusBarEx::~wxStatusBarEx()
{
	delete [] m_columnWidths;
}

void wxStatusBarEx::SetFieldsCount(int number /*=1*/, const int* widths /*=NULL*/)
{
	wxASSERT(number > 0);

	int oldCount = GetFieldsCount();
	int* oldWidths = m_columnWidths;
	
	m_columnWidths = new int[number];
	if (!widths)
	{
		if (oldWidths)
		{
			const int min = wxMin(oldCount, number);
			for (int i = 0; i < min; i++)
				m_columnWidths[i] = oldWidths[i];
			for (int i = min; i < number; i++)
				m_columnWidths[i] = -1;
			delete [] oldWidths;
		}
		else
		{
			for (int i = 0; i < number; i++)
				m_columnWidths[i] = -1;
		}
	}
	else
	{
		for (int i = 0; i < number; i++)
			m_columnWidths[i] = widths[i];

#ifdef __WXMSW__
		// Gripper overlaps last field
		if (!m_parentWasMaximized && m_columnWidths[number - 1] > 0)
			m_columnWidths[number - 1] += 6;
#elif __WXGTK20__
		// Gripper overlaps last all the time
		if (m_columnWidths[number - 1] > 0)
			m_columnWidths[number - 1] += 15;
#endif
	}

	wxStatusBar::SetFieldsCount(number, m_columnWidths);
}

void wxStatusBarEx::SetStatusWidths(int n, const int *widths)
{
	wxASSERT(n == GetFieldsCount());
	wxASSERT(widths);
	for (int i = 0; i < n; i++)
		m_columnWidths[i] = widths[i];

#ifdef __WXMSW__
	// Gripper overlaps last field if not maximized
	if (!m_parentWasMaximized && m_columnWidths[n - 1] > 0)
		m_columnWidths[n - 1] += 6;
#elif __WXGTK20__
	// Gripper overlaps last all the time
	if (m_columnWidths[number - 1] > 0)
		m_columnWidths[number - 1] += 15;
#endif

	wxStatusBar::SetStatusWidths(n, m_columnWidths);
}

void wxStatusBarEx::AddChild(int field, wxWindow* pChild, int cx)
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

void wxStatusBarEx::OnSize(wxSizeEvent& event)
{
#ifdef __WXMSW__
	const int count = GetFieldsCount();
	if (count && m_columnWidths && m_columnWidths[count - 1] > 0)
	{
		// No sizegrip on maximized windows
		bool isMaximized = m_pParent->IsMaximized();
		if (isMaximized != m_parentWasMaximized)
		{
			m_parentWasMaximized = isMaximized;

			if (isMaximized)
				m_columnWidths[count - 1] -= 6;
			else
				m_columnWidths[count - 1] += 6;

			wxStatusBar::SetStatusWidths(count, m_columnWidths);
			Refresh();
		}
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

CStatusBar::CStatusBar(wxTopLevelWindow* pParent)
	: wxStatusBarEx(pParent)
{
	const int count = 6;
	SetFieldsCount(count);
	int array[count];
	for (int i = 1; i < 5; i++)
		array[i] = wxSB_NORMAL;
	array[0] = wxSB_FLAT;
	array[count - 1] = wxSB_FLAT;
	SetStatusStyles(count, array);

	SetStatusWidths(count, statbarWidths);
}
