#include <filezilla.h>
#include "sitemanager.h"

#include "filezillaapp.h"
#include "ipcmutex.h"
#include "Options.h"
#include "xmlfunctions.h"

std::map<int, struct CSiteManager::_menu_data> CSiteManager::m_idMap;

bool CSiteManager::Load(TiXmlElement *pElement, CSiteManagerXmlHandler* pHandler)
{
	wxASSERT(pElement);
	wxASSERT(pHandler);

	for (TiXmlElement* pChild = pElement->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
	{
		if (!strcmp(pChild->Value(), "Folder"))
		{
			wxString name = GetTextElement_Trimmed(pChild);
			if (name.empty())
				continue;

			const bool expand = GetTextAttribute(pChild, "expanded") != _T("0");
			if (!pHandler->AddFolder(name, expand))
				return false;
			Load(pChild, pHandler);
			if (!pHandler->LevelUp())
				return false;
		}
		else if (!strcmp(pChild->Value(), "Server"))
		{
			CSiteManagerItemData_Site* data = ReadServerElement(pChild);

			if (data)
			{
				pHandler->AddSite(data);

				// Bookmarks
				for (TiXmlElement* pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
				{
					TiXmlHandle handle(pBookmark);

					wxString name = GetTextElement_Trimmed(pBookmark, "Name");
					if (name.empty())
						continue;

					CSiteManagerItemData* data = new CSiteManagerItemData(CSiteManagerItemData::BOOKMARK);

					TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
					if (localDir)
						data->m_localDir = GetTextElement(pBookmark, "LocalDir");

					TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
					if (remoteDir)
						data->m_remoteDir.SetSafePath(ConvLocal(remoteDir->Value()));

					if (data->m_localDir.empty() && data->m_remoteDir.IsEmpty())
					{
						delete data;
						continue;
					}

					if (!data->m_localDir.empty() && !data->m_remoteDir.IsEmpty())
						data->m_sync = GetTextElementBool(pBookmark, "SyncBrowsing", false);

					pHandler->AddBookmark(name, data);
				}

				if (!pHandler->LevelUp())
					return false;
			}
		}
	}

	return true;
}

CSiteManagerItemData_Site* CSiteManager::ReadServerElement(TiXmlElement *pElement)
{
	CServer server;
	if (!::GetServer(pElement, server))
		return 0;
	if (server.GetName().empty())
		return 0;

	CSiteManagerItemData_Site* data = new CSiteManagerItemData_Site(server);

	TiXmlHandle handle(pElement);

	TiXmlText* comments = handle.FirstChildElement("Comments").FirstChild().Text();
	if (comments)
		data->m_comments = ConvLocal(comments->Value());

	TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
	if (localDir)
		data->m_localDir = ConvLocal(localDir->Value());

	TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
	if (remoteDir)
		data->m_remoteDir.SetSafePath(ConvLocal(remoteDir->Value()));

	if (!data->m_localDir.empty() && !data->m_remoteDir.IsEmpty())
		data->m_sync = GetTextElementBool(pElement, "SyncBrowsing", false);

	return data;
}

class CSiteManagerXmlHandler_Menu : public CSiteManagerXmlHandler
{
public:
	CSiteManagerXmlHandler_Menu(wxMenu* pMenu, std::map<int, struct CSiteManager::_menu_data> *idMap, bool predefined)
		: m_pMenu(pMenu), m_idMap(idMap)
	{
		m_added_site = false;

		if (predefined)
			path = _T("1");
		else
			path = _T("0");
	}

	unsigned int GetInsertIndex(wxMenu* pMenu, const wxString& name)
	{
		unsigned int i;
		for (i = 0; i < pMenu->GetMenuItemCount(); ++i)
		{
			const wxMenuItem* const pItem = pMenu->FindItemByPosition(i);
			if (!pItem)
				continue;

			// Use same sorting as site tree in site manager
#ifdef __WXMSW__
			if (pItem->GetLabel().CmpNoCase(name) > 0)
				break;
#else
			if (pItem->GetLabel() > name)
				break;
#endif
		}

		return i;
	}

	virtual bool AddFolder(const wxString& name, bool)
	{
		m_parents.push_back(m_pMenu);
		m_childNames.push_back(name);
		m_paths.push_back(path);

		wxString str(name);
		str.Replace(_T("\\"), _T("\\\\"));
		str.Replace(_T("/"), _T("\\/"));
		path += _T("/") + str;

		m_pMenu = new wxMenu;

		return true;
	}

	virtual bool AddSite(CSiteManagerItemData_Site* data)
	{
		wxString newName(data->m_server.GetName());
		int i = GetInsertIndex(m_pMenu, newName);
		newName.Replace(_T("&"), _T("&&"));
		wxMenuItem* pItem = m_pMenu->Insert(i, wxID_ANY, newName);

		struct CSiteManager::_menu_data menu_data;
		menu_data.data = data;
		menu_data.path = data->m_server.GetName();
		menu_data.path.Replace(_T("\\"), _T("\\\\"));
		menu_data.path.Replace(_T("/"), _T("\\/"));
		menu_data.path = path + _T("/") + menu_data.path;

		(*m_idMap)[pItem->GetId()] = menu_data;

		m_added_site = true;

		return true;
	}

	virtual bool AddBookmark(const wxString& name, CSiteManagerItemData* data)
	{
		delete data;
		return true;
	}

	// Go up a level
	virtual bool LevelUp()
	{
		if (m_added_site)
		{
			m_added_site = false;
			return true;
		}

		if (m_parents.empty() || m_childNames.empty())
			return false;

		wxMenu* pChild = m_pMenu;
		m_pMenu = m_parents.back();
		if (pChild->GetMenuItemCount())
		{
			wxString name = m_childNames.back();
			int i = GetInsertIndex(m_pMenu, name);
			name.Replace(_T("&"), _T("&&"));

			wxMenuItem* pItem = new wxMenuItem(m_pMenu, wxID_ANY, name, _T(""), wxITEM_NORMAL, pChild);
			m_pMenu->Insert(i, pItem);
		}
		else
			delete pChild;
		m_childNames.pop_back();
		m_parents.pop_back();

		path = m_paths.back();
		m_paths.pop_back();

		return true;
	}

protected:
	wxMenu* m_pMenu;

	std::map<int, struct CSiteManager::_menu_data> *m_idMap;

	std::list<wxMenu*> m_parents;
	std::list<wxString> m_childNames;

	bool m_added_site;

	wxString path;
	std::list<wxString> m_paths;
};

wxMenu* CSiteManager::GetSitesMenu()
{
	ClearIdMap();

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	wxMenu* predefinedSites = GetSitesMenu_Predefined(m_idMap);

	CXmlFile file(_T("sitemanager"));
	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		if (!predefinedSites)
			return predefinedSites;

		wxMenu *pMenu = new wxMenu;
		wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
		pItem->Enable(false);
		return pMenu;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
	{
		if (!predefinedSites)
			return predefinedSites;

		wxMenu *pMenu = new wxMenu;
		wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
		pItem->Enable(false);
		return pMenu;
	}

	wxMenu* pMenu = new wxMenu;
	CSiteManagerXmlHandler_Menu handler(pMenu, &m_idMap, false);

	bool res = Load(pElement, &handler);
	if (!res || !pMenu->GetMenuItemCount())
	{
		delete pMenu;
		pMenu = 0;
	}

	if (pMenu)
	{
		if (!predefinedSites)
			return pMenu;

		wxMenu* pRootMenu = new wxMenu;
		pRootMenu->AppendSubMenu(predefinedSites, _("Predefined Sites"));
		pRootMenu->AppendSubMenu(pMenu, _("My Sites"));

		return pRootMenu;
	}

	if (predefinedSites)
		return predefinedSites;

	pMenu = new wxMenu;
	wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
	pItem->Enable(false);
	return pMenu;
}

void CSiteManager::ClearIdMap()
{
	for (std::map<int, struct _menu_data>::iterator iter = m_idMap.begin(); iter != m_idMap.end(); ++iter)
		delete iter->second.data;

	m_idMap.clear();
}

wxMenu* CSiteManager::GetSitesMenu_Predefined(std::map<int, struct _menu_data> &idMap)
{
	const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
	if (defaultsDir == _T(""))
		return 0;

	wxFileName name(defaultsDir, _T("fzdefaults.xml"));
	CXmlFile file(name);

	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
		return 0;

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return 0;

	wxMenu* pMenu = new wxMenu;
	CSiteManagerXmlHandler_Menu handler(pMenu, &idMap, true);

	if (!Load(pElement, &handler))
	{
		delete pMenu;
		return 0;
	}

	if (!pMenu->GetMenuItemCount())
	{
		delete pMenu;
		return 0;
	}

	return pMenu;
}

CSiteManagerItemData_Site* CSiteManager::GetSiteById(int id, wxString& path)
{
	std::map<int, struct _menu_data>::iterator iter = m_idMap.find(id);

	CSiteManagerItemData_Site *pData;
	if (iter != m_idMap.end())
	{
		pData = iter->second.data;
		iter->second.data = 0;
		path = iter->second.path;
	}
	else
		pData = 0;

	ClearIdMap();

	return pData;
}

bool CSiteManager::UnescapeSitePath(wxString path, std::list<wxString>& result)
{
	result.clear();

	wxString name;
	const wxChar *p = path;

	// Undo escapement
	bool lastBackslash = false;
	while (*p)
	{
		const wxChar& c = *p;
		if (c == '\\')
		{
			if (lastBackslash)
			{
				name += _T("\\");
				lastBackslash = false;
			}
			else
				lastBackslash = true;
		}
		else if (c == '/')
		{
			if (lastBackslash)
			{
				name += _T("/");
				lastBackslash = 0;
			}
			else
			{
				if (!name.IsEmpty())
					result.push_back(name);
				name.clear();
			}
		}
		else
			name += *p;
		++p;
	}
	if (lastBackslash)
		return false;
	if (name != _T(""))
		result.push_back(name);

	return !result.empty();
}

CSiteManagerItemData_Site* CSiteManager::GetSiteByPath(wxString sitePath)
{
	wxChar c = sitePath[0];
	if (c != '0' && c != '1')
	{
		wxMessageBox(_("Site path has to begin with 0 or 1."), _("Invalid site path"));
		return 0;
	}

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	if (c == '0')
		pDocument = file.Load(_T("sitemanager"));
	else
	{
		const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
		if (defaultsDir == _T(""))
		{
			wxMessageBox(_("Site does not exist."), _("Invalid site path"));
			return 0;
		}
		wxFileName name(defaultsDir, _T("fzdefaults.xml"));
		pDocument = file.Load(name);
	}

	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		return 0;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	std::list<wxString> segments;
	if (!UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild)
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pBookmark;
	if (!strcmp(pChild->Value(), "Bookmark"))
	{
		pBookmark = pChild;
		pChild = pChild->Parent()->ToElement();
	}
	else
		pBookmark = 0;

	CSiteManagerItemData_Site* data = ReadServerElement(pChild);

	if (!data)
	{
		wxMessageBox(_("Could not read server item."), _("Invalid site path"));
		return 0;
	}

	if (pBookmark)
	{
		TiXmlHandle handle(pBookmark);

		wxString localPath;
		CServerPath remotePath;
		TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
		if (localDir)
			localPath = ConvLocal(localDir->Value());
		TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
		if (remoteDir)
			remotePath.SetSafePath(ConvLocal(remoteDir->Value()));
		if (!localPath.empty() && !remotePath.IsEmpty())
		{
			data->m_sync = GetTextElementBool(pBookmark, "SyncBrowsing", false);
		}
		else
			data->m_sync = false;

		data->m_localDir = localPath;
		data->m_remoteDir = remotePath;
	}

	return data;
}

bool CSiteManager::GetBookmarks(wxString sitePath, std::list<wxString> &bookmarks)
{
	wxChar c = sitePath[0];
	if (c != '0' && c != '1')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	if (c == '0')
		pDocument = file.Load(_T("sitemanager"));
	else
	{
		const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
		if (defaultsDir == _T(""))
			return false;
		pDocument = file.Load(wxFileName(defaultsDir, _T("fzdefaults.xml")));
	}

	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
		return 0;

	// Bookmarks
	for (TiXmlElement* pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		TiXmlHandle handle(pBookmark);

		wxString name = GetTextElement_Trimmed(pBookmark, "Name");
		if (name.empty())
			continue;

		wxString localPath;
		CServerPath remotePath;
		TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
		if (localDir)
			localPath = ConvLocal(localDir->Value());
		TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
		if (remoteDir)
			remotePath.SetSafePath(ConvLocal(remoteDir->Value()));

		if (localPath.empty() && remotePath.IsEmpty())
			continue;

		bookmarks.push_back(name);		
	}

	return true;
}

wxString CSiteManager::AddServer(CServer server)
{
	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The server could not be added.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return _T("");
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return _T("");

	std::list<wxString> names;
	for (TiXmlElement* pChild = pElement->FirstChildElement("Server"); pChild; pChild = pChild->NextSiblingElement("Server"))
	{
		wxString name = GetTextElement(pChild, "Name");
		if (name.empty())
			continue;

		names.push_back(name);
	}

	wxString name = _("New site");
	int i = 1;

	while (true)
	{
		std::list<wxString>::const_iterator iter;
		for (iter = names.begin(); iter != names.end(); ++iter)
		{
			if (*iter == name)
				break;
		}
		if (iter == names.end())
			break;

		name = _("New site") + wxString::Format(_T(" %d"), ++i);
	}

	server.SetName(name);

	TiXmlElement* pServer = pElement->LinkEndChild(new TiXmlElement("Server"))->ToElement();
	SetServer(pServer, server);

	char* utf8 = ConvUTF8(name);
	if (utf8)
	{
		pServer->LinkEndChild(new TiXmlText(utf8));
		delete [] utf8;
	}

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return _T("");

		wxString msg = wxString::Format(_("Could not write \"%s\", any changes to the Site Manager could not be saved: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
		return _T("");
	}
	
	name.Replace(_T("\\"), _T("\\\\"));
	name.Replace(_T("/"), _T("\\/"));

	return _T("0/") + name;
}

TiXmlElement* CSiteManager::GetElementByPath(TiXmlElement* pNode, std::list<wxString> &segments)
{
	while (!segments.empty())
	{
		const wxString segment = segments.front();
		segments.pop_front();

		TiXmlElement* pChild;
		for (pChild = pNode->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
		{
			const char* value = pChild->Value();
			if (strcmp(value, "Server") && strcmp(value, "Folder") && strcmp(value, "Bookmark"))
				continue;

			wxString name = GetTextElement_Trimmed(pChild, "Name");
			if (name.empty())
				name = GetTextElement_Trimmed(pChild);
			if (name.empty())
				continue;

			if (name == segment)
				break;
		}
		if (!pChild)
			return 0;

		pNode = pChild;
		continue;
	}

	return pNode;
}

bool CSiteManager::AddBookmark(wxString sitePath, const wxString& name, const wxString &local_dir, const CServerPath &remote_dir, bool sync)
{
	if (local_dir.empty() && remote_dir.IsEmpty())
		return false;

	if (sitePath[0] != '0')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The bookmark could not be added.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	// Bookmarks
	TiXmlElement *pInsertBefore = 0;
	TiXmlElement* pBookmark;
	for (pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		TiXmlHandle handle(pBookmark);

		wxString old_name = GetTextElement_Trimmed(pBookmark, "Name");
		if (old_name.empty())
			continue;

		if (name == old_name)
		{
			wxMessageBox(_("Name of bookmark already exists."), _("New bookmark"), wxICON_EXCLAMATION);
			return false;
		}
		if (name < old_name && !pInsertBefore)
			pInsertBefore = pBookmark;
	}

	if (pInsertBefore)
		pBookmark = pChild->InsertBeforeChild(pInsertBefore, TiXmlElement("Bookmark"))->ToElement();
	else
		pBookmark = pChild->LinkEndChild(new TiXmlElement("Bookmark"))->ToElement();
	AddTextElement(pBookmark, "Name", name);
	if (!local_dir.empty())
		AddTextElement(pBookmark, "LocalDir", local_dir);
	if (!remote_dir.IsEmpty())
		AddTextElement(pBookmark, "RemoteDir", remote_dir.GetSafePath());
	if (sync)
		AddTextElementRaw(pBookmark, "SyncBrowsing", "1");

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return true;

		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	return true;
}

bool CSiteManager::ClearBookmarks(wxString sitePath)
{
	if (sitePath[0] != '0')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The bookmarks could not be cleared.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement *pBookmark = pChild->FirstChildElement("Bookmark");
	while (pBookmark)
	{
		pChild->RemoveChild(pBookmark);
		pBookmark = pChild->FirstChildElement("Bookmark");
	}

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return true;

		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	return true;
}
