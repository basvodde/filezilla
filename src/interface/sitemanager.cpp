#include "FileZilla.h"
#include "sitemanager.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"

BEGIN_EVENT_TABLE(CSiteManager, wxDialog)
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
END_EVENT_TABLE()

class CSiteManagerItemData : public wxTreeItemData
{
public:
	CSiteManagerItemData(CServer server = CServer())
	{
		m_server = server;
	}

	virtual ~CSiteManagerItemData()
	{
	}

	CServer m_server;
	wxString m_comments;
};

CSiteManager::CSiteManager(COptions* pOptions)
	: m_pOptions(pOptions)
{
}

bool CSiteManager::Create(wxWindow* parent)
{
	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);
	CreateControls();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	
	Load();
		
	return TRUE;
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
		return;
	
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
		treeId = pTree->AddRoot(_("My Sites"));
		
		pElement = m_pOptions->GetXml();
		pElement = pElement->FirstChildElement("Servers");

		if (!pElement)
		{
			m_pOptions->FreeXml();
			return true;
		}

		bool res = Load(pElement, treeId);
	
		pTree->SortChildren(treeId);
		pTree->Expand(treeId);
		pTree->SelectItem(treeId);
		
		m_pOptions->FreeXml();
		return res;
	}
	
	for (TiXmlElement* pChild = pElement->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
	{
		TiXmlNode* pNode = pChild->FirstChild();
		while (pNode && !pNode->ToText())
			pNode = pNode->NextSibling();
			
		if (!pNode)
			continue;
	
		wxString name = m_pOptions->ConvLocal(pNode->ToText()->Value());
		
		if (!strcmp(pChild->Value(), "Folder"))
		{
			wxTreeItemId id = pTree->AppendItem(treeId, name);
			Load(pChild, id);
		}
		else if (!strcmp(pChild->Value(), "Server"))
		{
			CServer server;
			if (!m_pOptions->GetServer(pChild, server))
				continue;

			CSiteManagerItemData* data = new CSiteManagerItemData(server);
			
			TiXmlHandle handle(pChild);

			TiXmlText* comments = handle.FirstChildElement("Comments").FirstChild().Text();
			if (comments)
				data->m_comments = m_pOptions->ConvLocal(comments->Value());
			
			pTree->AppendItem(treeId, name, -1, -1, data);
		}
	}
	pTree->SortChildren(treeId);
	pTree->Expand(treeId);
		
	return true;
}

bool CSiteManager::Save(TiXmlElement *pElement /*=0*/, wxTreeItemId treeId /*=wxTreeItemId()*/)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;
		
	if (!pElement || !treeId)
	{
		pElement = m_pOptions->GetXml();
		TiXmlElement *pServers = pElement->FirstChildElement("Servers");
		while (pServers)
		{
			pElement->RemoveChild(pServers);
			pServers = pElement->FirstChildElement("Servers");
		}
		pElement = pElement->InsertEndChild(TiXmlElement("Servers"))->ToElement();

		if (!pElement)
		{
			m_pOptions->FreeXml();
			return true;
		}

		bool res = Save(pElement, pTree->GetRootItem());
		m_pOptions->FreeXml();
		return res;
	}
	
	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	child = pTree->GetFirstChild(treeId, cookie);
	while (child.IsOk())
	{
		wxString name = pTree->GetItemText(child);
		char* utf8 = m_pOptions->ConvUTF8(name);
		
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
			m_pOptions->SetServer(pNode->ToElement(), data->m_server);

			TiXmlNode* sub = pNode->InsertEndChild(TiXmlElement("Comments"));
			char* comments = m_pOptions->ConvUTF8(data->m_comments);
			sub->InsertEndChild(TiXmlText(comments));
			delete [] comments;

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

	wxTreeItemId newItem = pTree->AppendItem(item, newName);
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
	pTree->SetFocus();
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
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = event.GetItem();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
	{
		// Set the control stats according if it's possible to use the control
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
	}
	else
	{
		// Set the control stats according if it's possible to use the control
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
			XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->SetValue(_("Ask"));
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
	}
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

	wxTreeItemId newItem = pTree->AppendItem(item, newName, -1, -1, new CSiteManagerItemData());
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

	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->GetValue() != _("Anonymous"));
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->GetValue() == _("Normal"));
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
	else
		data->m_server.SetProtocol(FTP);

	wxString logonType = XRCCTRL(*this, "ID_LOGONTYPE", wxComboBox)->GetValue();
	if (logonType == _("Normal"))
		data->m_server.SetLogonType(NORMAL);
	else if (logonType == _("Ask"))
		data->m_server.SetLogonType(ASK);
	else if (logonType == _("Interactive"))
		data->m_server.SetLogonType(INTERACTIVE);
	else
		data->m_server.SetLogonType(ANONYMOUS);

	data->m_server.SetUser(XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue(), XRCCTRL(*this, "ID_PASS", wxTextCtrl)->GetValue());
	
	data->m_comments = XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->GetValue();

	return true;
}

bool CSiteManager::GetServer(CServer &server)
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

	server = data->m_server;

	return true;
}
