#include "FileZilla.h"
#include "import.h"
#include "xmlfunctions.h"

CImportDialog::CImportDialog(wxWindow* parent)
	: m_parent(parent)
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
	if (!xmlDocument.LoadFile(dlg.GetPath().mb_str()))
	{
		wxMessageBox(_("Cannot load file, not a valid XML file."), _("Error importing"), wxICON_ERROR, m_parent);
		return;
	}

	TiXmlElement* fz3Root = xmlDocument.FirstChildElement("FileZilla3");
	TiXmlElement* fz2Root = xmlDocument.FirstChildElement("FileZilla");

	bool settings = false;
	bool queue = false;
	bool sites = false;
	bool sites_fz2 = false;

	if (fz3Root)
	{
		settings = fz3Root->FirstChildElement("Settings") != 0;
		queue = fz3Root->FirstChildElement("Queue") != 0;
		sites = fz3Root->FirstChildElement("Servers") != 0;
	}
	else if (fz2Root)
	{
		sites_fz2 = fz2Root->FirstChildElement("Sites") != 0;
	}

	if (!settings && !queue && !sites && !sites_fz2)
	{
		wxMessageBox(_("File does not contain any importable data."), _("Error importing"), wxICON_ERROR, m_parent);
		return;
	}
}
