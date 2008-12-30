#include "FileZilla.h"
#include "bookmarks_dialog.h"
#include "sitemanager.h"
#include "ipcmutex.h"
#include "xmlfunctions.h"

BEGIN_EVENT_TABLE(CNewBookmarkDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CNewBookmarkDialog::OnOK)
EVT_BUTTON(XRCID("ID_BROWSE"), CNewBookmarkDialog::OnBrowse)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CBookmarksDialog, wxDialogEx)
EVT_TREE_SEL_CHANGING(XRCID("ID_TREE"), CBookmarksDialog::OnSelChanging)
EVT_TREE_SEL_CHANGED(XRCID("ID_TREE"), CBookmarksDialog::OnSelChanged)
EVT_BUTTON(XRCID("wxID_OK"), CBookmarksDialog::OnOK)
EVT_BUTTON(XRCID("ID_BOOKMARK_BROWSE"), CBookmarksDialog::OnBrowse)
EVT_BUTTON(XRCID("ID_NEW"), CBookmarksDialog::OnNewBookmark)
EVT_BUTTON(XRCID("ID_RENAME"), CBookmarksDialog::OnRename)
EVT_BUTTON(XRCID("ID_DELETE"), CBookmarksDialog::OnDelete)
EVT_BUTTON(XRCID("ID_COPY"), CBookmarksDialog::OnCopy)
EVT_TREE_BEGIN_LABEL_EDIT(XRCID("ID_TREE"), CBookmarksDialog::OnBeginLabelEdit)
EVT_TREE_END_LABEL_EDIT(XRCID("ID_TREE"), CBookmarksDialog::OnEndLabelEdit)
END_EVENT_TABLE()

CNewBookmarkDialog::CNewBookmarkDialog(wxWindow* parent, wxString& site_path, const CServer* server)
: m_parent(parent), m_site_path(site_path), m_server(server)
{
}

int CNewBookmarkDialog::ShowModal(const wxString &local_path, const CServerPath &remote_path)
{
	if (!Load(m_parent, _T("ID_NEWBOOKMARK")))
		return wxID_CANCEL;

	XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl)->ChangeValue(local_path);
	if (!remote_path.IsEmpty())
		XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->ChangeValue(remote_path.GetPath());
	
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
	else
	{
		if (!CBookmarksDialog::AddBookmark(name, local_path, remote_path))
			return;

		EndModal(wxID_OK);
	}
}

void CNewBookmarkDialog::OnBrowse(wxCommandEvent& event)
{
	wxTextCtrl *pText = XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl);

	wxDirDialog dlg(this, _("Choose the local directory"), pText->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() == wxID_OK)
	{
		pText->ChangeValue(dlg.GetPath());
	}
}

class CBookmarkItemData : public wxTreeItemData
{
public:
	CBookmarkItemData(const wxString& local_dir = _T(""), const CServerPath& remote_dir = CServerPath())
		: m_local_dir(local_dir), m_remote_dir(remote_dir)
	{
	}

	wxString m_local_dir;
	CServerPath m_remote_dir;
};

CBookmarksDialog::CBookmarksDialog(wxWindow* parent, wxString& site_path, const CServer* server)
: m_parent(parent), m_site_path(site_path), m_server(server)
{
}

void CBookmarksDialog::LoadGlobalBookmarks()
{
	CInterProcessMutex mutex(MUTEX_GLOBALBOOKMARKS);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("bookmarks"));

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	for (TiXmlElement *pBookmark = pDocument->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		wxString name;
		wxString local_dir;
		wxString remote_dir_raw;
		CServerPath remote_dir;

		name = GetTextElement(pBookmark, "Name");
		if (name.empty())
			continue;

		local_dir = GetTextElement(pBookmark, "LocalDir");
		remote_dir_raw = GetTextElement(pBookmark, "RemoteDir");
		if (!remote_dir_raw.empty())
		{
			if (!remote_dir.SetSafePath(remote_dir_raw))
				continue;
		}
		if (local_dir.empty() && remote_dir.IsEmpty())
			continue;

		CBookmarkItemData *data = new CBookmarkItemData(local_dir, remote_dir);
		m_pTree->AppendItem(m_bookmarks_global, name, 1, 1, data);
	}

	m_pTree->SortChildren(m_bookmarks_global);
}

void CBookmarksDialog::LoadSiteSpecificBookmarks()
{
	if (m_site_path.empty())
		return;

	std::list<wxString> bookmarks;

	if (!CSiteManager::GetBookmarks(m_site_path, bookmarks))
		return;

	for (std::list<wxString>::const_iterator iter = bookmarks.begin(); iter != bookmarks.end(); iter++)
	{
		wxString path = *iter;
		path.Replace(_T("\\"), _T("\\\\"));
		path.Replace(_T("/"), _T("\\/"));
		path = m_site_path + _T("/") + path;

		CSiteManagerItemData_Site* data = CSiteManager::GetSiteByPath(path);

		CBookmarkItemData* new_data = new CBookmarkItemData(data->m_localDir, data->m_remoteDir);
		m_pTree->AppendItem(m_bookmarks_site, *iter, 1, 1, new_data);

		delete data;
	}

	m_pTree->SortChildren(m_bookmarks_site);
}

int CBookmarksDialog::ShowModal(const wxString &local_path, const CServerPath &remote_path)
{
	if (!Load(m_parent, _T("ID_BOOKMARKS")))
		return wxID_CANCEL;

	// Now create the imagelist for the site tree
	m_pTree = XRCCTRL(*this, "ID_TREE", wxTreeCtrl);
	if (!m_pTree)
		return false;
	wxImageList* pImageList = new wxImageList(16, 16);

	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_BOOKMARK"),  wxART_OTHER, wxSize(16, 16)));

	m_pTree->AssignImageList(pImageList);

	wxTreeItemId root = m_pTree->AddRoot(_T(""));
	m_bookmarks_global = m_pTree->AppendItem(root, _("Global bookmarks"), 0, 0);
	LoadGlobalBookmarks();
	m_pTree->Expand(m_bookmarks_global);
	if (m_server)
	{
		m_bookmarks_site = m_pTree->AppendItem(root, _("Site-specific bookmarks"), 0, 0);
		LoadSiteSpecificBookmarks();
		m_pTree->Expand(m_bookmarks_site);
	}

	wxNotebook *pBook = XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook);

	wxPanel* pPanel = new wxPanel;
	wxXmlResource::Get()->LoadPanel(pPanel, pBook, _T("ID_SITEMANAGER_BOOKMARK_PANEL"));
	pBook->AddPage(pPanel, _("Bookmark"));

	XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetContainingSizer()->GetItem((size_t)0)->SetMinSize(200, -1);

	GetSizer()->Fit(this);

	wxSize minSize = GetSizer()->GetMinSize();
	wxSize size = GetSize();
	wxSize clientSize = GetClientSize();
	SetMinSize(GetSizer()->GetMinSize() + size - clientSize);
	SetClientSize(minSize);

	return wxDialogEx::ShowModal();
}

void CBookmarksDialog::SaveGlobalBookmarks()
{
	CInterProcessMutex mutex(MUTEX_GLOBALBOOKMARKS);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("bookmarks"));

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	TiXmlElement *pBookmark = pDocument->FirstChildElement("Bookmark");
	while (pBookmark)
	{
		pDocument->RemoveChild(pBookmark);
		pBookmark = pDocument->FirstChildElement("Bookmark");
	}

	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = m_pTree->GetFirstChild(m_bookmarks_global, cookie); child.IsOk(); child = m_pTree->GetNextChild(m_bookmarks_global, cookie))
	{
		CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(child);
		wxASSERT(data);

		TiXmlElement *pBookmark = pDocument->InsertEndChild(TiXmlElement("Bookmark"))->ToElement();
		AddTextElement(pBookmark, "Name", m_pTree->GetItemText(child));
		if (!data->m_local_dir.empty())
			AddTextElement(pBookmark, "LocalDir", data->m_local_dir);
		if (!data->m_remote_dir.IsEmpty())
			AddTextElement(pBookmark, "RemoteDir", data->m_remote_dir.GetSafePath());
	}

	wxString error;
	if (!file.Save(&error))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}
}

void CBookmarksDialog::SaveSiteSpecificBookmarks()
{
	if (m_site_path.empty())
		return;

	if (!CSiteManager::ClearBookmarks(m_site_path))
			return;

	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = m_pTree->GetFirstChild(m_bookmarks_site, cookie); child.IsOk(); child = m_pTree->GetNextChild(m_bookmarks_site, cookie))
	{
		CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(child);
		wxASSERT(data);

		if (!CSiteManager::AddBookmark(m_site_path, m_pTree->GetItemText(child), data->m_local_dir, data->m_remote_dir))
			return;
	}
}

void CBookmarksDialog::OnOK(wxCommandEvent& event)
{
	if (!Verify())
		return;
	UpdateBookmark();

	SaveGlobalBookmarks();
	SaveSiteSpecificBookmarks();

	EndModal(wxID_OK);
}

void CBookmarksDialog::OnBrowse(wxCommandEvent& event)
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item)
		return;
	
	CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(item);
	if (!data)
		return;

	wxTextCtrl *pText = XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl);

	wxDirDialog dlg(this, _("Choose the local directory"), pText->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() == wxID_OK)
	{
		pText->ChangeValue(dlg.GetPath());
	}
}

void CBookmarksDialog::OnSelChanging(wxTreeEvent& event)
{
	if (!Verify())
	{
		event.Veto();
		return;
	}

	UpdateBookmark();
}

void CBookmarksDialog::OnSelChanged(wxTreeEvent& event)
{
	DisplayBookmark();
}

bool CBookmarksDialog::Verify()
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item)
		return true;
	
	CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(item);
	if (!data)
		return true;

	const CServer *server;
	if (m_pTree->GetItemParent(item) == m_bookmarks_site)
		server = m_server;
	else
		server = 0;

	const wxString remotePathRaw = XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->GetValue();
	if (remotePathRaw != _T(""))
	{
		CServerPath remotePath;
		if (server)
			remotePath.SetType(server->GetType());
		if (!remotePath.SetPath(remotePathRaw))
		{
			XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->SetFocus();
			wxMessageBox(_("Default remote path cannot be parsed. Make sure it is valid and is supported by the servertype selected on the parent site."));
			return false;
		}
	}

	const wxString localPath = XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetValue();

	if (remotePathRaw.empty() && localPath.empty())
	{
		XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->SetFocus();
		wxMessageBox(_("You need to enter at least one path, empty bookmarks are not supported."));
		return false;
	}

	return true;
}

void CBookmarksDialog::UpdateBookmark()
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item)
		return;
	
	CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(item);
	if (!data)
		return;

	const CServer *server;
	if (m_pTree->GetItemParent(item) == m_bookmarks_site)
		server = m_server;
	else
		server = 0;

	const wxString remotePathRaw = XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->GetValue();
	if (remotePathRaw != _T(""))
	{
		if (server)
			data->m_remote_dir.SetType(server->GetType());
		data->m_remote_dir.SetPath(remotePathRaw);
	}

	data->m_local_dir = XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetValue();
}

void CBookmarksDialog::DisplayBookmark()
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item)
	{
		XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_DELETE", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_RENAME", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_COPY", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook)->Enable(false);
		return;
	}
	
	CBookmarkItemData *data = (CBookmarkItemData *)m_pTree->GetItemData(item);
	if (!data)
	{
		XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_DELETE", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_RENAME", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_COPY", wxButton)->Enable(false);
		XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook)->Enable(false);
		return;
	}

	XRCCTRL(*this, "ID_DELETE", wxButton)->Enable(true);
	XRCCTRL(*this, "ID_RENAME", wxButton)->Enable(true);
	XRCCTRL(*this, "ID_COPY", wxButton)->Enable(true);
	XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook)->Enable(true);

	XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->ChangeValue(data->m_remote_dir.GetPath());
	XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->ChangeValue(data->m_local_dir);
}

void CBookmarksDialog::OnNewBookmark(wxCommandEvent& event)
{
	if (!Verify())
		return;
	UpdateBookmark();

	wxTreeItemId item = m_pTree->GetSelection();
	if (!item)
		item = m_bookmarks_global;

	if (m_pTree->GetItemData(item))
		item = m_pTree->GetItemParent(item);

	if (item == m_bookmarks_site)
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
				return;
			}
		}
	}

	wxString newName = _("New bookmark");
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = m_pTree->GetFirstChild(item, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = m_pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}

			child = m_pTree->GetNextChild(item, cookie);
		}
		if (!found)
			break;

		newName = _("New bookmark") + wxString::Format(_T(" %d"), index++);
	}

	wxTreeItemId child = m_pTree->AppendItem(item, newName, 1, 1, new CBookmarkItemData);
	m_pTree->SortChildren(item);
	m_pTree->SelectItem(child);
	m_pTree->EditLabel(child);
}

void CBookmarksDialog::OnRename(wxCommandEvent& event)
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item || item == m_bookmarks_global || item == m_bookmarks_site)
		return;

	m_pTree->EditLabel(item);
}

void CBookmarksDialog::OnDelete(wxCommandEvent& event)
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item || item == m_bookmarks_global || item == m_bookmarks_site)
		return;

	wxTreeItemId parent = m_pTree->GetItemParent(item);

	m_pTree->Delete(item);
}

void CBookmarksDialog::OnCopy(wxCommandEvent& event)
{
	wxTreeItemId item = m_pTree->GetSelection();
	if (!item.IsOk())
		return;

	if (!Verify())
		return;

	CBookmarkItemData* data = reinterpret_cast<CBookmarkItemData *>(m_pTree->GetItemData(item));
	if (!data)
		return;

	UpdateBookmark();

	wxTreeItemId parent = m_pTree->GetItemParent(item);

	const wxString name = m_pTree->GetItemText(item);
	wxString newName = wxString::Format(_("Copy of %s"), name.c_str());
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = m_pTree->GetFirstChild(parent, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = m_pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}

			child = m_pTree->GetNextChild(parent, cookie);
		}
		if (!found)
			break;

		newName = wxString::Format(_("Copy (%d) of %s"), index++, name.c_str());
	}

	CBookmarkItemData* newData = new CBookmarkItemData(*data);
	wxTreeItemId newItem = m_pTree->AppendItem(parent, newName, 1, 1, newData);

	m_pTree->SortChildren(parent);
	m_pTree->SelectItem(newItem);
	m_pTree->EditLabel(newItem);
}

void CBookmarksDialog::OnBeginLabelEdit(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (item != m_pTree->GetSelection())
	{
		if (!Verify())
		{
			event.Veto();
			return;
		}
	}

	if (!item || item == m_bookmarks_global || item == m_bookmarks_site)
	{
		event.Veto();
		return;
	}
}

void CBookmarksDialog::OnEndLabelEdit(wxTreeEvent& event)
{
	if (event.IsEditCancelled())
		return;

	wxTreeItemId item = event.GetItem();
	if (item != m_pTree->GetSelection())
	{
		if (!Verify())
		{
			event.Veto();
			return;
		}
	}

	if (!item || item == m_bookmarks_global || item == m_bookmarks_site)
	{
		event.Veto();
		return;
	}

	wxString name = event.GetLabel();

	wxTreeItemId parent = m_pTree->GetItemParent(item);

	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = m_pTree->GetFirstChild(parent, cookie); child.IsOk(); child = m_pTree->GetNextChild(parent, cookie))
	{
		if (child == item)
			continue;
		if (!name.CmpNoCase(m_pTree->GetItemText(child)))
		{
			wxMessageBox(_("Name already exists"), _("Cannot rename entry"), wxICON_EXCLAMATION, this);
			event.Veto();
			return;
		}
	}

	m_pTree->SortChildren(parent);
}

bool CBookmarksDialog::GetBookmarks(std::list<wxString> &bookmarks)
{
	CInterProcessMutex mutex(MUTEX_GLOBALBOOKMARKS);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("bookmarks"));

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	for (TiXmlElement *pBookmark = pDocument->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		wxString name;
		wxString local_dir;
		wxString remote_dir_raw;
		CServerPath remote_dir;

		name = GetTextElement(pBookmark, "Name");
		if (name.empty())
			continue;

		local_dir = GetTextElement(pBookmark, "LocalDir");
		remote_dir_raw = GetTextElement(pBookmark, "RemoteDir");
		if (!remote_dir_raw.empty())
		{
			if (!remote_dir.SetSafePath(remote_dir_raw))
				continue;
		}
		if (local_dir.empty() && remote_dir.IsEmpty())
			continue;

		bookmarks.push_back(name);
	}

	return true;
}

bool CBookmarksDialog::GetBookmark(const wxString &name, wxString &local_dir, CServerPath &remote_dir)
{
	CInterProcessMutex mutex(MUTEX_GLOBALBOOKMARKS);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("bookmarks"));

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	for (TiXmlElement *pBookmark = pDocument->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		wxString remote_dir_raw;
	
		if (name != GetTextElement(pBookmark, "Name"))
			continue;

		local_dir = GetTextElement(pBookmark, "LocalDir");
		remote_dir_raw = GetTextElement(pBookmark, "RemoteDir");
		if (!remote_dir_raw.empty())
		{
			if (!remote_dir.SetSafePath(remote_dir_raw))
				return false;
		}
		if (local_dir.empty() && remote_dir_raw.empty())
			return false;

		return true;
	}

	return false;
}


bool CBookmarksDialog::AddBookmark(const wxString &name, const wxString &local_dir, const CServerPath &remote_dir)
{
	if (local_dir.empty() && remote_dir.IsEmpty())
		return false;

	CInterProcessMutex mutex(MUTEX_GLOBALBOOKMARKS);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("bookmarks"));

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement *pBookmark;
	for (pBookmark = pDocument->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		wxString remote_dir_raw;
	
		if (!name.CmpNoCase(GetTextElement(pBookmark, "Name")))
		{
			wxMessageBox(_("Name of bookmark already exists."), _("New bookmark"), wxICON_EXCLAMATION);
			return false;
		}
	}

	pBookmark = pDocument->InsertEndChild(TiXmlElement("Bookmark"))->ToElement();
	AddTextElement(pBookmark, "Name", name);
	if (!local_dir.empty())
		AddTextElement(pBookmark, "LocalDir", local_dir);
	if (!remote_dir.IsEmpty())
		AddTextElement(pBookmark, "RemoteDir", remote_dir.GetSafePath());

	wxString error;
	if (!file.Save(&error))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
		return false;
	}

	return true;
}
