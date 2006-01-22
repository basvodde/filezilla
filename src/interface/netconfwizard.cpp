#include "FileZilla.h"
#include "netconfwizard.h"
#include "Options.h"
#include "dialogex.h"

BEGIN_EVENT_TABLE(CNetConfWizard, wxWizard)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CNetConfWizard::OnPageChanging)
EVT_WIZARD_PAGE_CHANGED(wxID_ANY, CNetConfWizard::OnPageChanged)
END_EVENT_TABLE()

CNetConfWizard::CNetConfWizard(wxWindow* parent, COptions* pOptions)
: m_parent(parent), m_pOptions(pOptions)
{
}

CNetConfWizard::~CNetConfWizard()
{
}

bool CNetConfWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Firewall and router configuration wizard")))
		return false;

	for (int i = 1; i <= 6; i++)
	{
		wxWizardPageSimple* page = new wxWizardPageSimple();
		bool res = wxXmlResource::Get()->LoadPanel(page, this, wxString::Format(_T("NETCONF_PANEL%d"), i));
		if (!res)
			return false;
		page->Show(false);

		m_pages.push_back(page);
	}
	for (unsigned int i = 0; i < (m_pages.size() - 1); i++)
		m_pages[i]->Chain(m_pages[i], m_pages[i + 1]);

	GetPageAreaSizer()->Add(m_pages[0]);

	std::vector<wxWindow*> windows;
	for (unsigned int i = 0; i < m_pages.size(); i++)
		windows.push_back(m_pages[i]);
	WrapRecursive(windows, 1.7);

	return true;
}

bool CNetConfWizard::Run()
{
	RunWizard(m_pages.front());
	return true;
}

void CNetConfWizard::OnPageChanging(wxWizardEvent& event)
{
	if (event.GetPage() == m_pages[3])
	{
		int mode = XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->GetValue() ? 0 : (XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->GetValue() ? 1 : 2);
		if (mode == 1)
		{
			wxString ip = XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl)->GetValue();
		}
		else if (mode == 2)
		{
			wxTextCtrl* pResolver = XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl);
			wxString address = pResolver->GetValue();
			if (address == _T(""))
			{
				wxMessageBox(_("Please enter an URL where to get your external address from"));
				pResolver->SetFocus();
				event.Veto();
				return;
			}
		}
	}
}

void CNetConfWizard::OnPageChanged(wxWizardEvent& event)
{
}
