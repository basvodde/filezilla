#include "FileZilla.h"
#include "sitemanager.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"
#include "ipcmutex.h"

BEGIN_EVENT_TABLE(CSiteManager, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CSiteManager::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CSiteManager::OnCancel)
EVT_BUTTON(XRCID("ID_CONNECT"), CSiteManager::OnConnect)
EVT_BUTTON(XRCID("ID_NEWSITE"), CSiteManager::OnNewSite)
EVT_BUTTON(XRCID("ID_NEWFOLDER"), CSiteManager::OnNewFolder)
EVT_BUTTON(XRCID("ID_RENAME"), CSiteManager::OnRename)
EVT_BUTTON(XRCID("ID_DELETE"), CSiteManager::OnDelete)
EVT_TREE_BEGIN_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnBeginLabelEdit)
EVT_TREE_END_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnEndLabelEdit)
EVT_TREE_SEL_CHANGING(XRCID("ID_SITETREE"), CSiteManager::OnSelChanging)
EVT_TREE_SEL_CHANGED(XRCID("ID_SITETREE"), CSiteManager::OnSelChanged)
EVT_COMBOBOX(XRCID("ID_LOGONTYPE"), CSiteManager::OnLogontypeSelChanged)
EVT_BUTTON(XRCID("ID_BROWSE"), CSiteManager::OnRemoteDirBrowse)
EVT_TREE_ITEM_ACTIVATED(XRCID("ID_SITETREE"), CSiteManager::OnItemActivated)
EVT_CHECKBOX(XRCID("ID_LIMITMULTIPLE"), CSiteManager::OnLimitMultipleConnectionsChanged)
END_EVENT_TABLE()

CSiteManager::CSiteManager(COptions* pOptions)
	: m_pOptions(pOptions)
{
	m_pSiteManagerMutex = 0;
}

CSiteManager::~CSiteManager()
{
	delete m_pSiteManagerMutex;
}

bool CSiteManager::Create(wxWindow* parent)
{
	m_pSiteManagerMutex = new CInterProcessMutex(MUTEX_SITEMANAGERGLOBAL, false);
	if (!m_pSiteManagerMutex->TryLock())
	{
		int answer = wxMessageBox(_("The Site Manager is opened in another instance of FileZilla 3.\nDo you want to continue? Any changes made in the Site Manager won't be saved then."),
								  _("Site Manager already open"), wxYES_NO | wxICON_QUESTION);
		if (answer != wxYES)					
			return false;

		delete m_pSiteManagerMutex;
		m_pSiteManagerMutex = 0;
	}
	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);
	CreateControls();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	
	Load();

	XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->Update();
	XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->Update();
	XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->Update();

	// Now create the imagelist for the site tree
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;
	wxImageList* pImageList = new wxImageList(16, 16);

	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_SERVER"),  wxART_OTHER, wxSize(16, 16)));
	
	pTree->AssignImageList(pImageList);
	
	SetCtrlState();
		
	return true;
}

void CSiteManager::CreateControls()
{	
	wxXmlResource::Get()->LoadDialog(this, GetParent(), _T("ID_SITEMANAGER"));
}

void CSiteManager::OnOK(wxCommandEvent& event)
{
	if (!Verify())
	{
		wxBell();
		return;
	}

	UpdateServer();
		
	Save();
	
	EndModal(wxID_OK);
}

void CSiteManager::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CSiteManager::OnConnect(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
	{
		wxBell();
		return;
	}
	
	if (!Verify())
	{
		wxBell();
		return;
	}

	UpdateServer();
		
	Save();
	
	EndModal(wxID_YES);
}

bool CSiteManager::Load(TiXmlElement *pElement /*=0*/, wxTreeItemId treeId /*=wxTreeItemId()*/)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;
	
	if (!pElement || !treeId)
	{
		pTree->DeleteAllItems();
		treeId = pTree->AddRoot(_("My Sites"), 0, 0);
		pTree->SelectItem(treeId);
		pTree->SetItemImage(treeId, 1, wxTreeItemIcon_Expanded);
		pTree->SetItemImage(treeId, 1, wxTreeItemIcon_SelectedExpanded);

		// We have to synchronize access to sitemanager.xml so that multiple processed don't write 
		// to the same file or one is reading while the other one writes.
		CInterProcessMutex mutex(MUTEX_SITEMANAGER);

		wxFileName file(wxGetApp().GetSettingsDir(), _T("sitemanager.xml"));
		TiXmlElement* pDocument = GetXmlFile(file);
		if (!pDocument)
		{
			wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFullPath().c_str());
			wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

			return false;
		}
		pElement = pDocument->FirstChildElement("Servers");
		if (!pElement)
		{
			delete pDocument->GetDocument();
			return true;
		}

		bool res = Load(pElement, treeId);

		delete pDocument->GetDocument();

		pTree->SortChildren(treeId);
		pTree->Expand(treeId);
		pTree->SelectItem(treeId);
		
		return res;
	}
	
	for (TiXmlElement* pChild = pElement->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
	{
		TiXmlNode* pNode = pChild->FirstChild();
		while (pNode && !pNode->ToText())
			pNode = pNode->NextSibling();
			
		if (!pNode)
			continue;
	
		wxString name = ConvLocal(pNode->ToText()->Value());
		
		if (!strcmp(pChild->Value(), "Folder"))
		{
			wxTreeItemId id = pTree->AppendItem(treeId, name, 0, 0);
			pTree->SetItemImage(id, 1, wxTreeItemIcon_Expanded);
			pTree->SetItemImage(id, 1, wxTreeItemIcon_SelectedExpanded);
			Load(pChild, id);
		}
		else if (!strcmp(pChild->Value(), "Server"))
		{
			CServer server;
			if (!::GetServer(pChild, server))
				continue;

			CSiteManagerItemData* data = new CSiteManagerItemData(server);
			
			TiXmlHandle handle(pChild);

			TiXmlText* comments = handle.FirstChildElement("Comments").FirstChild().Text();
			if (comments)
				data->m_comments = ConvLocal(comments->Value());
			
			TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
			if (localDir)
				data->m_localDir = ConvLocal(localDir->Value());
			
			TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
			if (remoteDir)
				data->m_remoteDir.SetSafePath(ConvLocal(remoteDir->Value()));
			
			pTree->AppendItem(treeId, name, 2, 2, data);
		}
	}
	pTree->SortChildren(treeId);
	pTree->Expand(treeId);
		
	return true;
}

bool CSiteManager::Save(TiXmlElement *pElement /*=0*/, wxTreeItemId treeId /*=wxTreeItemId()*/)
{
	if (!m_pSiteManagerMutex)
		return false;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;
		
	if (!pElement || !treeId)
	{
		// We have to synchronize access to sitemanager.xml so that multiple processed don't write 
		// to the same file or one is reading while the other one writes.
		CInterProcessMutex mutex(MUTEX_SITEMANAGER);

		wxFileName file(wxGetApp().GetSettingsDir(), _T("sitemanager.xml"));
		TiXmlElement* pDocument = GetXmlFile(file);
		if (!pDocument)
		{
			wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager could not be saved."), file.GetFullPath().c_str());
			wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

			return false;
		}

		TiXmlElement *pServers = pDocument->FirstChildElement("Servers");
		while (pServers)
		{
			pDocument->RemoveChild(pServers);
			pServers = pDocument->FirstChildElement("Servers");
		}
		pElement = pDocument->InsertEndChild(TiXmlElement("Servers"))->ToElement();

		if (!pElement)
		{
			delete pDocument->GetDocument();

			return true;
		}

		bool res = Save(pElement, pTree->GetRootItem());
		
		if (!pDocument->GetDocument()->SaveFile(file.GetFullPath().mb_str()))
		{
			wxString msg = wxString::Format(_("Could not write \"%s\", any changes to the Site Manager could not be saved."), file.GetFullPath().c_str());
			wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
		}

		delete pDocument->GetDocument();
		return res;
	}
	
	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	child = pTree->GetFirstChild(treeId, cookie);
	while (child.IsOk())
	{
		wxString name = pTree->GetItemText(child);
		char* utf8 = ConvUTF8(name);
		
		CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(child));
		if (!data)
		{
			TiXmlNode* pNode = pElement->InsertEndChild(TiXmlElement("Folder"));
			pNode->InsertEndChild(TiXmlText(utf8));
			delete [] utf8;
		
			Save(pNode->ToElement(), child);
		}
		else
		{
			TiXmlNode* pNode = pElement->InsertEndChild(TiXmlElement("Server"));
			SetServer(pNode->ToElement(), data->m_server);

			TiXmlNode* sub;

			// Save comments
			sub = pNode->InsertEndChild(TiXmlElement("Comments"));
			char* comments = ConvUTF8(data->m_comments);
			sub->InsertEndChild(TiXmlText(comments));
			delete [] comments;

			// Save local dir
			sub = pNode->InsertEndChild(TiXmlElement("LocalDir"));
			char* localDir = ConvUTF8(data->m_localDir);
			sub->InsertEndChild(TiXmlText(localDir));
			delete [] localDir;

			// Save remote dir
			sub = pNode->InsertEndChild(TiXmlElement("RemoteDir"));
			char* remoteDir = ConvUTF8(data->m_remoteDir.GetSafePath());
			sub->InsertEndChild(TiXmlText(remoteDir));
			delete [] remoteDir;

			pNode->InsertEndChild(TiXmlText(utf8));
			delete [] utf8;
		}
		
		child = pTree->GetNextChild(treeId, cookie);
	}
	
	return false;
}

void CSiteManager::OnNewFolder(wxCommandEvent&event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;
		
	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;
	
	if (pTree->GetItemData(item))
		return;
		
	if (!Verify())
		return;
	
	wxString newName = _("New folder");
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = pTree->GetFirstChild(item, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}
							
			child = pTree->GetNextChild(item, cookie);
		}
		if (!found)
			break;
		
		newName = _("New folder") + wxString::Format(_T(" %d"), index++);
	}

	wxTreeItemId newItem = pTree->AppendItem(item, newName, 0, 0);
	pTree->SetItemImage(newItem, 1, wxTreeItemIcon_Expanded);
	pTree->SetItemImage(newItem, 1, wxTreeItemIcon_SelectedExpanded);
	pTree->SortChildren(item);
	pTree->SelectItem(newItem);
	pTree->Expand(item);
	pTree->EditLabel(newItem);
}


bool CSiteManager::Verify()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return true;
		
	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return true;

	wxTreeItemData* data = pTree->GetItemData(item);
	if (!data)
		return true;

	return true;
}

void CSiteManager::OnBeginLabelEdit(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
	{
		event.Veto();
		return;
	}
	
	if (!Verify())
	{
		event.Veto();
		return;
	}
		
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || item == pTree->GetRootItem())
	{
		event.Veto();
		return;
	}
}

void CSiteManager::OnEndLabelEdit(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
	{
		event.Veto();
		return;
	}
	
	if (!Verify())
	{
		event.Veto();
		return;
	}
		
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || item == pTree->GetRootItem())
	{
		event.Veto();
		return;
	}
	
	wxString newName = event.GetLabel();
	wxTreeItemId parent = pTree->GetItemParent(item);
		
	pTree->SortChildren(parent);
}

void CSiteManager::OnRename(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;
	
	if (!Verify())
		return;
		
	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || item == pTree->GetRootItem())
		return;
	
	pTree->EditLabel(item);
}

void CSiteManager::OnDelete(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;
		
	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || item == pTree->GetRootItem())
		return;
		
	pTree->Delete(item);
}

void CSiteManager::OnSelChanging(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	if (!Verify())
		event.Veto();

	UpdateServer();
}

void CSiteManager::OnSelChanged(wxTreeEvent& event)
{
	SetCtrlState();
}

void CSiteManager::OnNewSite(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;
		
	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;
	
	if (pTree->GetItemData(item))
		return;
		
	if (!Verify())
		return;
	
	wxString newName = _("New site");
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = pTree->GetFirstChild(item, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}
							
			child = pTree->GetNextChild(item, cookie);
		}
		if (!found)
			break;
		
		newName = _("New site") + wxString::Format(_T(" %d"), index++);
	}

	wxTreeItemId newItem = pTree->AppendItem(item, newName, 2, 2, new CSiteManagerItemData());
	pTree->SortChildren(item);
	pTree->SelectItem(newItem);
	pTree->Expand(item);
	pTree->EditLabel(newItem);
}

void CSiteManager::OnLogontypeSelChanged(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(event.GetString() != _("Anonymous"));
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(event.GetString() == _("Normal"));
}

bool CSiteManager::UpdateServer()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return false;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return false;

	unsigned long port;
	XRCCTRL(*this, "ID_PORT", wxTextCtrl)->GetValue().ToULong(&port);
	data->m_server.SetHost(XRCCTRL(*this, "ID_HOST", wxTextCtrl)->GetValue(), port);

	wxString protocol = XRCCTRL(*this, "ID_PROTOCOL", wxComboBox)->GetValue();
	if (protocol == _("FTP"))
		data->m_server.SetProtocol(FTP);
	else if (protocol == _("SFTP"))
		data->m_server.SetProtocol(SFTP);
	else
		data->m_server.SetProtocol(FTP);

	wxString logonType = XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->GetValue();
	if (logonType == _("Normal"))
		data->m_server.SetLogonType(NORMAL);
	else if (logonType == _("Ask for password"))
		data->m_server.SetLogonType(ASK);
	else if (logonType == _("Interactive"))
		data->m_server.SetLogonType(INTERACTIVE);
	else
		data->m_server.SetLogonType(ANONYMOUS);

	data->m_server.SetUser(XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue(), XRCCTRL(*this, "ID_PASS", wxTextCtrl)->GetValue());
	
	data->m_comments = XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->GetValue();

	wxString serverType = XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->GetValue();
	if (serverType == _("Unix"))
		data->m_server.SetType(UNIX);
	else if (serverType == _("Dos"))
		data->m_server.SetType(DOS);
	else if (serverType == _("VMS"))
		data->m_server.SetType(VMS);
	else if (serverType == _("MVS"))
		data->m_server.SetType(MVS);
	else
		data->m_server.SetType(DEFAULT);
	
	data->m_localDir = XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->GetValue();
	data->m_remoteDir = CServerPath();
	data->m_remoteDir.SetType(data->m_server.GetType());
	data->m_remoteDir.SetPath(XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->GetValue());
	data->m_server.SetTimezoneOffset(XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->GetValue() * 60 + 
									 XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->GetValue());

	if (XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->GetValue())
		data->m_server.SetPasvMode(MODE_ACTIVE);
	else if (XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->GetValue())
		data->m_server.SetPasvMode(MODE_PASSIVE);
	else
		data->m_server.SetPasvMode(MODE_DEFAULT);

	if (XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->GetValue())
	{
		data->m_server.MaximumMultipleConnections(XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->GetValue());
	}
	else
		data->m_server.MaximumMultipleConnections(0);

	return true;
}

bool CSiteManager::GetServer(CSiteManagerItemData& data)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return false;

	CSiteManagerItemData* pData = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!pData)
		return false;

	data = *pData;
	data.m_name = pTree->GetItemText(item);

	return true;
}

void CSiteManager::OnRemoteDirBrowse(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	wxDirDialog dlg(this, _("Choose the default local directory"), XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() == wxID_OK)
	{
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(dlg.GetPath());
	}
}

void CSiteManager::OnItemActivated(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	wxCommandEvent cmdEvent;
	OnConnect(cmdEvent);
}

void CSiteManager::OnLimitMultipleConnectionsChanged(wxCommandEvent& event)
{
	XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(event.IsChecked());
}

void CSiteManager::SetCtrlState()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	
	CSiteManagerItemData* data = 0;
	if (item.IsOk())
		data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
	{
		// Set the control states according if it's possible to use the control
		XRCCTRL(*this, "ID_RENAME", wxWindow)->Enable(item != pTree->GetRootItem());
		XRCCTRL(*this, "ID_DELETE", wxWindow)->Enable(item != pTree->GetRootItem());
		XRCCTRL(*this, "ID_COPY", wxWindow)->Enable(item != pTree->GetRootItem());
		XRCCTRL(*this, "ID_NOTEBOOK", wxWindow)->Enable(false);
		XRCCTRL(*this, "ID_NEWFOLDER", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_NEWSITE", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_CONNECT", wxWindow)->Enable(false);

		// Empty all site information
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(_T("21"));
		XRCCTRL(*this, "ID_PROTOCOL", wxComboBox)->SetValue(_("FTP"));
		XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Anonymous"));
		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->SetValue(_T(""));
		
		XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("Default"));
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->SetValue(0);
		XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->SetValue(0);

		XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->SetValue(true);
		XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->SetValue(false);
		XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(1);
	}
	else
	{
		// Set the control states according if it's possible to use the control
		XRCCTRL(*this, "ID_RENAME", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_DELETE", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_COPY", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_NOTEBOOK", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_NEWFOLDER", wxWindow)->Enable(false);
		XRCCTRL(*this, "ID_NEWSITE", wxWindow)->Enable(false);
		XRCCTRL(*this, "ID_CONNECT", wxWindow)->Enable(true);

		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(data->m_server.GetHost());
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), data->m_server.GetPort()));
		switch (data->m_server.GetProtocol())
		{
		case SFTP:
			XRCCTRL(*this, "ID_PROTOCOL", wxComboBox)->SetValue(_("SFTP"));
			break;
		case FTP:
		default:
			XRCCTRL(*this, "ID_PROTOCOL", wxComboBox)->SetValue(_("FTP"));
			break;
		}

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(data->m_server.GetLogonType() != ANONYMOUS);
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(data->m_server.GetLogonType() == NORMAL);

		switch (data->m_server.GetLogonType())
		{
		case NORMAL:
			XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Normal"));
			break;
		case ASK:
			XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Ask for password"));
			break;
		case INTERACTIVE:
			XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Interactive"));
			break;
		default:
			XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Anonymous"));
			break;
		}

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(data->m_server.GetUser());
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(data->m_server.GetPass());
		XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->SetValue(data->m_comments);

		switch (data->m_server.GetType())
		{
		case UNIX:
			XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("Unix"));
			break;
		case DOS:
			XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("Dos"));
			break;
		case MVS:
			XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("MVS"));
			break;
		case VMS:
			XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("VMS"));
			break;
		default:
			XRCCTRL(*this, "ID_SERVERTYPE", wxComboBox)->SetValue(_("Default"));
			break;
		}
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(data->m_localDir);
		XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->SetValue(data->m_remoteDir.GetPath());
		XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->SetValue(data->m_server.GetTimezoneOffset() / 60);
		XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->SetValue(data->m_server.GetTimezoneOffset() % 60);

		enum PasvMode pasvMode = data->m_server.GetPasvMode();
		if (pasvMode == MODE_ACTIVE)
			XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->SetValue(true);
		else if (pasvMode == MODE_PASSIVE)
			XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->SetValue(true);
		else
			XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->SetValue(true);

		int maxMultiple = data->m_server.MaximumMultipleConnections();
		XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->SetValue(maxMultiple != 0);
		if (maxMultiple != 0)
		{
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(true);
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(maxMultiple);
		}
		else
		{
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(false);
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(1);
		}
	}
}
