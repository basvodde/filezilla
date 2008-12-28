#include "FileZilla.h"
#include "bookmarks_dialog.h"
#include "sitemanager.h"

BEGIN_EVENT_TABLE(CNewBookmarkDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CNewBookmarkDialog::OnOK)
END_EVENT_TABLE()

CNewBookmarkDialog::CNewBookmarkDialog(wxWindow* parent, wxString& site_path, const CServer* server)
: m_parent(parent), m_site_path(site_path), m_server(server)
{
}

int CNewBookmarkDialog::ShowModal()
{
	if (!Load(m_parent, _T("ID_NEWBOOKMARK")))
		return wxID_CANCEL;

	if (!m_server)
		XRCCTRL(*this, "ID_TYPE_SITE", wxRadioButton)->Enable(false);

	return wxDialogEx::ShowModal();
}

void CNewBookmarkDialog::OnOK(wxCommandEvent& event)
{
	const bool global = XRCCTRL(*this, "ID_TYPE_GLOBAL", wxRadioButton)->GetValue();

	const wxString name = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	if (name.empty())
	{
		wxMessageBox(_("You need to enter a name for the bookmark."), _("New bookmark"), wxICON_EXCLAMATION, this);
		return;
	}

	wxString local_path = XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl)->GetValue();
	wxString remote_path_raw = XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->GetValue();

	CServerPath remote_path;
	if (!remote_path_raw.empty())
	{
		if (!global && m_server)
			remote_path.SetType(m_server->GetType());
		if (!remote_path.SetPath(remote_path_raw))
		{
			wxMessageBox(_("Could not parse remote path."), _("New bookmark"), wxICON_EXCLAMATION);
			return;
		}
	}

	if (local_path.empty() && remote_path_raw.empty())
	{
		wxMessageBox(_("You need to enter at least one path, empty bookmarks are not supported."), _("New bookmark"), wxICON_EXCLAMATION, this);
		return;
	}

	if (!global)
	{
		std::list<wxString> bookmarks;

		if (m_site_path.empty() || !CSiteManager::GetBookmarks(m_site_path, bookmarks))
		{
			if (wxMessageBox(_("Site-specific bookmarks require the server to be stored in the Site Manager.\nAdd current connection to the site manager?"), _("New bookmark"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
				return;

			m_site_path = CSiteManager::AddServer(*m_server);
			if (m_site_path == _T(""))
			{
				wxMessageBox(_("Could not add connection to Site Manager"), _("New bookmark"), wxICON_EXCLAMATION, this);
				EndModal(wxID_CANCEL);
				return;
			}
		}
		for (std::list<wxString>::const_iterator iter = bookmarks.begin(); iter != bookmarks.end(); iter++)
		{
			if (*iter == name)
			{
				wxMessageBox(_("A bookmark with the entered name already exists. Please enter an unused name."), _("New bookmark"), wxICON_EXCLAMATION, this);
				return;
			}
		}

		CSiteManager::AddBookmark(m_site_path, name, local_path, remote_path);

		EndModal(wxID_OK);
	}
}
