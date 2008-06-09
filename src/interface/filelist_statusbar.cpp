#include "FileZilla.h"
#include "filelist_statusbar.h"

BEGIN_EVENT_TABLE(CFilelistStatusBar, wxStatusBar)
EVT_TIMER(wxID_ANY, CFilelistStatusBar::OnTimer)
END_EVENT_TABLE()

CFilelistStatusBar::CFilelistStatusBar(wxWindow* pParent)
	: wxStatusBar(pParent, wxID_ANY, 0)
{
	m_count_files = 0;
	m_count_dirs = 0;
	m_total_size = 0;
	m_unknown_size = false;
	m_hidden = false;

	m_count_selected_files = 0;
	m_count_selected_dirs = 0;
	m_total_selected_size = 0;
	m_unknown_selected_size = 0;

	m_updateTimer.SetOwner(this);

	UpdateText();
}

// Defined in LocalListView.cpp
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix = false);

void CFilelistStatusBar::UpdateText()
{
	wxString text;
	if (m_count_selected_files || m_count_selected_dirs)
	{
		if (!m_count_selected_files)
			text = wxString::Format(wxPLURAL("Selected %d directory.", "Selected %d directories.", m_count_selected_dirs), m_count_selected_dirs);
		else if (!m_count_selected_dirs)
		{
			const wxString size = FormatSize(m_total_selected_size, true);
			if (m_unknown_selected_size)
				text = wxString::Format(wxPLURAL("Selected %d file. Total size: At least %s", "Selected %d files. Total size: At least %s", m_count_selected_files), m_count_selected_files, size.c_str());
			else
				text = wxString::Format(wxPLURAL("Selected %d file. Total size: %s", "Selected %d files. Total size: %s", m_count_selected_files), m_count_selected_files, size.c_str());
		}
		else
		{
			const wxString files = wxString::Format(wxPLURAL("%d file", "%d files", m_count_selected_files), m_count_selected_files);
			const wxString dirs = wxString::Format(wxPLURAL("%d directory", "%d directories", m_count_selected_dirs), m_count_selected_dirs);
			const wxString size = FormatSize(m_total_selected_size, true);
			if (m_unknown_selected_size)
				text = wxString::Format(_("Selected %s and %s. Total size: At least %s"), files.c_str(), dirs.c_str(), size.c_str());
			else
				text = wxString::Format(_("Selected %s and %s. Total size: %s"), files.c_str(), dirs.c_str(), size.c_str());
		}
	}
	else if (m_count_files || m_count_dirs)
	{
		if (!m_count_files)
			text = wxString::Format(wxPLURAL("%d directory", "%d directories", m_count_dirs), m_count_dirs);
		else if (!m_count_dirs)
		{
			const wxString size = FormatSize(m_total_size, true);
			if (m_unknown_size)
				text = wxString::Format(wxPLURAL("%d file. Total size: At least %s", "%d files. Total size: At least %s", m_count_files), m_count_files, size.c_str());
			else
				text = wxString::Format(wxPLURAL("%d file. Total size: %s", "%d files. Total size: %s", m_count_files), m_count_files, size.c_str());
		}
		else
		{
			const wxString files = wxString::Format(wxPLURAL("%d file", "%d files", m_count_files), m_count_files);
			const wxString dirs = wxString::Format(wxPLURAL("%d directory", "%d directories", m_count_dirs), m_count_dirs);
			const wxString size = FormatSize(m_total_size, true);
			if (m_unknown_size)
				text = wxString::Format(_("%s and %s. Total size: At least %s"), files.c_str(), dirs.c_str(), size.c_str());
			else
				text = wxString::Format(_("%s and %s. Total size: %s"), files.c_str(), dirs.c_str(), size.c_str());
		}
		if (m_hidden)
		{
			text += ' ';
			text += wxString::Format(wxPLURAL("(%d object filtered)", "(%d objects filtered)", m_hidden), m_hidden);
		}
	}
	else
		text = _("Empty directory.");

	SetStatusText(text);
}

void CFilelistStatusBar::SetDirectoryContents(int count_files, int count_dirs, const wxLongLong &total_size, int unknown_size, int hidden)
{
	m_count_files = count_files;
	m_count_dirs = count_dirs;
	m_total_size = total_size;
	m_unknown_size = unknown_size;
	m_hidden = hidden;

	m_count_selected_files = 0;
	m_count_selected_dirs = 0;
	m_total_selected_size = 0;
	m_unknown_selected_size = 0;

	TriggerUpdateText();
}

void CFilelistStatusBar::SelectAll()
{
	m_count_selected_files = m_count_files;
	m_count_selected_dirs = m_count_dirs;
	m_total_selected_size = m_total_size;
	m_unknown_selected_size = m_unknown_size;
	TriggerUpdateText();
}

void CFilelistStatusBar::UnselectAll()
{
	m_count_selected_files = 0;
	m_count_selected_dirs = 0;
	m_total_selected_size = 0;
	m_unknown_selected_size = false;
	TriggerUpdateText();
}

void CFilelistStatusBar::SelectFile(const wxLongLong &size)
{
	m_count_selected_files++;
	if (size == -1)
		m_unknown_selected_size++;
	else
		m_total_selected_size += size;
	TriggerUpdateText();
}

void CFilelistStatusBar::UnselectFile(const wxLongLong &size)
{
	m_count_selected_files--;
	if (size == -1)
		m_unknown_selected_size--;
	else
		m_total_selected_size -= size;
	TriggerUpdateText();
}

void CFilelistStatusBar::SelectDirectory()
{
	m_count_selected_dirs++;
	TriggerUpdateText();
}

void CFilelistStatusBar::UnselectDirectory()
{
	m_count_selected_dirs--;
	TriggerUpdateText();
}

void CFilelistStatusBar::OnTimer(wxTimerEvent& event)
{
	UpdateText();
}

void CFilelistStatusBar::TriggerUpdateText()
{
	if (m_updateTimer.IsRunning())
		return;

	m_updateTimer.Start(1, true);
}

void CFilelistStatusBar::AddFile(const wxLongLong& size)
{
	m_count_files++;
	if (size != -1)
		m_total_size += size;
	else
		m_unknown_size++;
	TriggerUpdateText();
}

void CFilelistStatusBar::RemoveFile(const wxLongLong& size)
{
	m_count_files++;
	if (size != -1)
		m_total_size -= size;
	else
		m_unknown_size--;
	TriggerUpdateText();
}

void CFilelistStatusBar::AddDirectory()
{
	m_count_dirs++;
	TriggerUpdateText();
}

void CFilelistStatusBar::RemoveDirectory()
{
	m_count_dirs--;
	TriggerUpdateText();
}

void CFilelistStatusBar::SetHidden(int hidden)
{
	m_hidden = hidden;
	TriggerUpdateText();
}
