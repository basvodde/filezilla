#include "FileZilla.h"
#include "sitemanager.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"

BEGIN_EVENT_TABLE(CSiteManager, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), CSiteManager::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CSiteManager::OnCancel)
EVT_BUTTON(XRCID("ID_NEWFOLDER"), CSiteManager::OnNewFolder)
EVT_BUTTON(XRCID("ID_RENAME"), CSiteManager::OnRename)
EVT_BUTTON(XRCID("ID_DELETE"), CSiteManager::OnDelete)
EVT_TREE_BEGIN_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnBeginLabelEdit)
EVT_TREE_END_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnEndLabelEdit)
END_EVENT_TABLE()

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
		return;
		
	Save();
	
	EndModal(wxID_OK);
}

void CSiteManager::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
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
		
		m_pOptions->FreeXml();
		return res;
	}
	
	TiXmlElement* pChild = pElement->FirstChildElement();
	while (pChild)
	{
		if (!strcmp(pChild->Value(), "Folder"))
		{
			TiXmlNode* pNode = pChild->FirstChild();
			while (pNode && !pNode->ToText())
				pNode = pNode->NextSibling();
			
			if (pNode)
			{
				wxTreeItemId id = pTree->AppendItem(treeId, m_pOptions->ConvLocal(pNode->ToText()->Value()));
				Load(pChild, id);
			}
		}
		else if (!strcmp(pChild->Value(), "Server"))
		{
		}
		pChild = pChild->NextSiblingElement();
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
		
		TiXmlNode* pNode = pElement->InsertEndChild(TiXmlElement("Folder"));
		pNode->InsertEndChild(TiXmlText(utf8));
		delete [] utf8;
		
		Save(pNode->ToElement(), child);
		
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
	int index = 1;
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
		
	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	child = pTree->GetFirstChild(parent, cookie);
	bool found = false;
	while (child.IsOk())
	{
		wxString name = pTree->GetItemText(child);
		int cmp = newName.CmpNoCase(name);
		if (!cmp && child != item)
		{
			found = true;
			break;
		}
					
		child = pTree->GetNextChild(item, cookie);
	}
	if (found)
	{
		event.Veto();
		wxBell();
		return;
	}
	
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
