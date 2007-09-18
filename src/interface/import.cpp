#include "FileZilla.h"
#include "import.h"
#include "xmlfunctions.h"
#include "ipcmutex.h"
#include "Options.h"
#include "queue.h"

CImportDialog::CImportDialog(wxWindow* parent, CQueueView* pQueueView)
	: m_parent(parent), m_pQueueView(pQueueView)
{
}

void CImportDialog::Show()
{
	wxFileDialog dlg(m_parent, _("Select file to import settings from"), _T(""), 
					_T("FileZilla.xml"), _T("XML files (*.xml)|*.xml"), 
					wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dlg.ShowModal() != wxID_OK)
		return;

	TiXmlDocument xmlDocument;
	xmlDocument.SetCondenseWhiteSpace(false);
	if (!xmlDocument.LoadFile(dlg.GetPath().mb_str()))
	{
		wxMessageBox(_("Cannot load file, not a valid XML file."), _("Error importing"), wxICON_ERROR, m_parent);
		return;
	}

	TiXmlElement* fz3Root = xmlDocument.FirstChildElement("FileZilla3");
	TiXmlElement* fz2Root = xmlDocument.FirstChildElement("FileZilla");

	if (fz3Root)
	{
		bool settings = fz3Root->FirstChildElement("Settings") != 0;
		bool queue = fz3Root->FirstChildElement("Queue") != 0;
		bool sites = fz3Root->FirstChildElement("Servers") != 0;

		if (settings || queue || sites)
		{
			Load(m_parent, _T("ID_IMPORT"));
			if (!queue)
				XRCCTRL(*this, "ID_QUEUE", wxCheckBox)->Hide();
			if (!sites)
				XRCCTRL(*this, "ID_SITEMANAGER", wxCheckBox)->Hide();
			if (!settings)
				XRCCTRL(*this, "ID_SETTINGS", wxCheckBox)->Hide();
			Fit();

			if (ShowModal() != wxID_OK)
				return;

			if (queue && XRCCTRL(*this, "ID_QUEUE", wxCheckBox)->IsChecked())
			{
				m_pQueueView->ImportQueue(fz3Root->FirstChildElement("Queue"), true);
			}

			if (sites && XRCCTRL(*this, "ID_SITEMANAGER", wxCheckBox)->IsChecked())
			{
				ImportSites(fz3Root->FirstChildElement("Servers"));
			}

			if (settings && XRCCTRL(*this, "ID_SETTINGS", wxCheckBox)->IsChecked())
			{
				COptions::Get()->Import(fz3Root->FirstChildElement("Settings"));
				wxMessageBox(_("The settings have been imported. You have to restart FileZilla for all settings to have effect."), _("Import successful"), wxOK, this);
			}
			
			wxMessageBox(_("The selected categories have been imported."), _("Import successful"), wxOK, this);

			return;
		}
	}
	else if (fz2Root)
	{
		bool sites_fz2 = fz2Root->FirstChildElement("Sites") != 0;
		if (sites_fz2)
		{
			int res = wxMessageBox(_("The file you have selected contains site manager data from a previous version of FileZilla.\nDue to differences in the storage format, only host, port, username and password will be imported.\nContinue with the import?"),
				_("Import data from older version"), wxICON_QUESTION | wxYES_NO);
			
			if (res == wxYES)
				ImportLegacySites(fz2Root->FirstChildElement("Sites"));

			return;
		}
	}

	wxMessageBox(_("File does not contain any importable data."), _("Error importing"), wxICON_ERROR, m_parent);
}

bool CImportDialog::ImportLegacySites(TiXmlElement* pSites)
{
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file(_T("sitemanager"));
	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pCurrentSites = pDocument->FirstChildElement("Servers");
	if (!pCurrentSites)
		pCurrentSites = pDocument->InsertEndChild(TiXmlElement("Servers"))->ToElement();

	if (!ImportLegacySites(pSites, pCurrentSites))
		return false;

	return file.Save();
}

wxString CImportDialog::DecodeLegacyPassword(wxString pass)
{
	wxString output;
	const char* key = "FILEZILLA1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int pos = (pass.Length() / 3) % strlen(key);
	for (unsigned int i = 0; i < pass.Length(); i += 3)
	{
		if (pass[i] < '0' || pass[i] > '9' ||
			pass[i + 1] < '0' || pass[i + 1] > '9' ||
			pass[i + 2] < '0' || pass[i + 2] > '9')
			return _T("");
		int number = (pass[i] - '0') * 100 +
						(pass[i + 1] - '0') * 10 +
						pass[i + 2] - '0';
		wxChar c = number ^ key[(i / 3 + pos) % strlen(key)];
		output += c;
	}

	return output;
}

bool CImportDialog::ImportLegacySites(TiXmlElement* pSitesToImport, TiXmlElement* pExistingSites)
{
	for (TiXmlElement* pImportFolder = pSitesToImport->FirstChildElement("Folder"); pImportFolder; pImportFolder = pImportFolder->NextSiblingElement("Folder"))
	{
		wxString name = GetTextAttribute(pImportFolder, "Name");
		if (name == _T(""))
			continue;

		wxString newName = name;
		int i = 2;
		TiXmlElement* pFolder;
		while (!(pFolder = GetFolderWithName(pExistingSites, newName)))
		{
			newName = wxString::Format(_T("%s %d"), name.c_str(), i++);
		}
		
		ImportLegacySites(pImportFolder, pFolder);
	}

	for (TiXmlElement* pImportSite = pSitesToImport->FirstChildElement("Site"); pImportSite; pImportSite = pImportSite->NextSiblingElement("Site"))
	{
		wxString name = GetTextAttribute(pImportSite, "Name");
		if (name == _T(""))
			continue;

		wxString host = GetTextAttribute(pImportSite, "Host");
		if (host == _T(""))
			continue;

		int port = GetAttributeInt(pImportSite, "Port");
		if (port < 1 || port > 65535)
			continue;

		int serverType = GetAttributeInt(pImportSite, "ServerType");
		if (serverType < 0 || serverType > 4)
			continue;

		int protocol;
		switch (serverType)
		{
		default:
		case 0:
			protocol = 0;
			break;
		case 1:
			protocol = 3;
			break;
		case 2:
		case 4:
			protocol = 4;
			break;
		case 3:
			protocol = 1;
			break;
		}

		bool dontSavePass = GetAttributeInt(pImportSite, "DontSavePass") == 1;

		int logontype = GetAttributeInt(pImportSite, "Logontype");
		if (logontype < 0 || logontype > 2)
			continue;
		if (logontype == 2)
			logontype = 4;
		if (logontype == 1 && dontSavePass)
			logontype = 2;

		wxString user = GetTextAttribute(pImportSite, "User");
		wxString pass = DecodeLegacyPassword(GetTextAttribute(pImportSite, "Pass"));
		wxString account = GetTextAttribute(pImportSite, "Account");
		if (logontype && user == _T(""))
			continue;

		// Find free name
		wxString newName = name;
		int i = 2;
		while (HasEntryWithName(pExistingSites, newName))
		{
			newName = wxString::Format(_T("%s %d"), name.c_str(), i++);
		}

		TiXmlElement* pServer = pExistingSites->InsertEndChild(TiXmlElement("Server"))->ToElement();
		AddTextElement(pServer, newName);

		AddTextElement(pServer, "Host", host);
		AddTextElement(pServer, "Port", port);
		AddTextElement(pServer, "Protocol", protocol);
		AddTextElement(pServer, "Logontype", logontype);
		AddTextElement(pServer, "User", user);
		AddTextElement(pServer, "Pass", pass);
		AddTextElement(pServer, "Account", account);
	}

	return true;
}

bool CImportDialog::HasEntryWithName(TiXmlElement* pElement, const wxString& name)
{
	TiXmlElement* pChild;
	for (pChild = pElement->FirstChildElement("Server"); pChild; pChild = pChild->NextSiblingElement("Server"))
	{
		wxString childName = GetTextElement(pChild);
		if (!name.CmpNoCase(childName))
			return true;
	}
	for (pChild = pElement->FirstChildElement("Folder"); pChild; pChild = pChild->NextSiblingElement("Folder"))
	{
		wxString childName = GetTextElement(pChild);
		if (!name.CmpNoCase(childName))
			return true;
	}

	return false;
}

TiXmlElement* CImportDialog::GetFolderWithName(TiXmlElement* pElement, const wxString& name)
{
	TiXmlElement* pChild;
	for (pChild = pElement->FirstChildElement("Server"); pChild; pChild = pChild->NextSiblingElement("Server"))
	{
		wxString childName = GetTextElement(pChild);
		if (!name.CmpNoCase(childName))
			return 0;
	}

	for (pChild = pElement->FirstChildElement("Folder"); pChild; pChild = pChild->NextSiblingElement("Folder"))
	{
		wxString childName = GetTextElement(pChild);
		if (!name.CmpNoCase(childName))
			return pChild;
	}

	pChild = pElement->InsertEndChild(TiXmlElement("Folder"))->ToElement();
	AddTextElement(pChild, name);

	return pChild;
}

bool CImportDialog::ImportSites(TiXmlElement* pSites)
{
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file(_T("sitemanager"));
	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager will not be saved."), file.GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pCurrentSites = pDocument->FirstChildElement("Servers");
	if (!pCurrentSites)
		pCurrentSites = pDocument->InsertEndChild(TiXmlElement("Servers"))->ToElement();

	if (!ImportSites(pSites, pCurrentSites))
		return false;

	return file.Save();
}

bool CImportDialog::ImportSites(TiXmlElement* pSitesToImport, TiXmlElement* pExistingSites)
{
	for (TiXmlElement* pImportFolder = pSitesToImport->FirstChildElement("Folder"); pImportFolder; pImportFolder = pImportFolder->NextSiblingElement("Folder"))
	{
		wxString name = GetTextElement(pImportFolder);
		if (name == _T(""))
			continue;

		wxString newName = name;
		int i = 2;
		TiXmlElement* pFolder;
		while (!(pFolder = GetFolderWithName(pExistingSites, newName)))
		{
			newName = wxString::Format(_T("%s %d"), name.c_str(), i++);
		}
		
		ImportSites(pImportFolder, pFolder);
	}

	for (TiXmlElement* pImportSite = pSitesToImport->FirstChildElement("Server"); pImportSite; pImportSite = pImportSite->NextSiblingElement("Server"))
	{
		wxString name = GetTextElement(pImportSite);
		if (name == _T(""))
			continue;

		// Find free name
		wxString newName = name;
		int i = 2;
		while (HasEntryWithName(pExistingSites, newName))
		{
			newName = wxString::Format(_T("%s %d"), name.c_str(), i++);
		}

		TiXmlElement* pServer = pExistingSites->InsertEndChild(*pImportSite)->ToElement();
		AddTextElement(pServer, newName);
	}

	return true;
}
