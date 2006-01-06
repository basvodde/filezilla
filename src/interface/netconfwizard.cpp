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

bool CNetConfWizard::WrapRecursive(wxWindow* wnd, wxSizer* sizer, int max)
{
	// This function auto-wraps static texts.

	if (max < 0)
		return false;

	wxSizer* root = wnd->GetSizer();

	wxPoint pos = sizer->GetPosition();

	for (unsigned int i = 0; i < sizer->GetChildren().GetCount(); i++)
	{
		wxSizerItem* item = sizer->GetItem(i);
		if (!item)
			continue;

		wxRect rect = item->GetRect();

		wxWindow* window;
		wxSizer* subSizer;
		if ((window = item->GetWindow()))
		{
			wxStaticText* text = wxDynamicCast(window, wxStaticText);
			if (!text)
				continue;
			
			wxString str = text->GetLabel();
			str = wxDialogEx::WrapText(str, max - rect.GetLeft() + pos.x, text);
			text->SetLabel(str);
		}
		else if ((subSizer = item->GetSizer()))
		{
			WrapRecursive(wnd, subSizer, wxMin(rect.GetWidth(), max - rect.GetLeft()));
		}
	}

	return true;
}

bool CNetConfWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Firewall and router configuration wizard")))
		return false;

	for (int i = 1; i <= 6; i++)
	{
		wxWizardPageSimple* page = new wxWizardPageSimple();
		bool res = wxXmlResource::Get()->LoadPanel(page, this, wxString::Format(_T("PANEL%d"), i));
		if (!res)
			return false;
		page->Show(false);

		m_pages.push_back(page);
	}
	for (unsigned int i = 0; i < (m_pages.size() - 1); i++)
		m_pages[i]->Chain(m_pages[i], m_pages[i + 1]);

	GetPageAreaSizer()->Add(m_pages[0]);

	for (unsigned int i = 0; i < m_pages.size(); i++)
	{
		wxWizardPage* page = m_pages[i];
		wxSizer* sizer = page->GetSizer();
		
		WrapRecursive(m_pages[i], m_pages[i]->GetSizer(), 350);
	}

	return true;
}

bool CNetConfWizard::Run()
{
	RunWizard(m_pages.front());
	return true;
}

void CNetConfWizard::OnPageChanging(wxWizardEvent& event)
{
	if (event.GetPage() == m_pages[1])
	{
	}
}

void CNetConfWizard::OnPageChanged(wxWizardEvent& event)
{
}
