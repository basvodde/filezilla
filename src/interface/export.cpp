#include "filezilla.h"
#include "export.h"
#include "xmlfunctions.h"
#include "ipcmutex.h"
#include "queue.h"

CExportDialog::CExportDialog(wxWindow* parent, CQueueView* pQueueView)
	: m_parent(parent), m_pQueueView(pQueueView)
{
}

void CExportDialog::Show()
{
	if (!Load(m_parent, _T("ID_EXPORT")))
		return;

	if (ShowModal() != wxID_OK)
		return;

	bool sitemanager = XRCCTRL(*this, "ID_SITEMANAGER", wxCheckBox)->GetValue();
	bool settings = XRCCTRL(*this, "ID_SETTINGS", wxCheckBox)->GetValue();
	bool queue = XRCCTRL(*this, "ID_QUEUE", wxCheckBox)->GetValue();

	if (!sitemanager && !settings && !queue)
	{
		wxMessageBox(_("No category to export selected"), _("Error exporting settings"), wxICON_ERROR, m_parent);
		return;
	}

	wxString str;
	if (sitemanager && !queue && !settings)
		str = _("Select file for exported sites");
	else if (!sitemanager && queue && !settings)
		str = _("Select file for exported queue");
	else if (!sitemanager && !queue && settings)
		str = _("Select file for exported settings");
	else
		str = _("Select file for exported data");

	wxFileDialog dlg(m_parent, str, _T(""), 
					_T("FileZilla.xml"), _T("XML files (*.xml)|*.xml"), 
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxFileName fn(dlg.GetPath());
	CXmlFile xml(fn);

	TiXmlElement* exportRoot = xml.CreateEmpty();

	if (sitemanager)
	{
		CInterProcessMutex mutex(MUTEX_SITEMANAGER);

		CXmlFile file(_T("sitemanager"));
		TiXmlElement* pDocument = file.Load();
		if (pDocument)
		{
			TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
			if (pElement)
				exportRoot->InsertEndChild(*pElement);
		}
	}
	if (settings)
	{
		CInterProcessMutex mutex(MUTEX_OPTIONS);
		CXmlFile file(_T("filezilla"));
		TiXmlElement* pDocument = file.Load();
		if (pDocument)
		{
			TiXmlElement* pElement = pDocument->FirstChildElement("Settings");
			if (pElement)
				exportRoot->InsertEndChild(*pElement);
		}
	}

	if (queue)
	{
		m_pQueueView->WriteToFile(exportRoot);
	}

	xml.Save();
}
