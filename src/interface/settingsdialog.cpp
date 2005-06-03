#include "FileZilla.h"
#include "settingsdialog.h"
#include "Options.h"
#include "optionspage.h"
#include "optionspage_passive.h"
#include "optionspage_filetype.h"
#include "optionspage_themes.h"
#include "optionspage_language.h"

enum pagenames
{
	page_none = -1,
	page_filetype = 0,
	page_passive = 1,
	page_themes = 2,
	page_language = 3
};

// Helper macro to add pages in the most simplistic way
#define ADD_PAGE(name, classname, parent) \
	wxASSERT(parent < (int)m_pages.size()); \
	page.page = new classname; \
	if (parent == page_none) \
		page.id = treeCtrl->AppendItem(root, name); \
	else \
		page.id = treeCtrl->AppendItem(m_pages[(unsigned int)parent].id, name); \
	m_pages.push_back(page);

BEGIN_EVENT_TABLE(CSettingsDialog, wxDialogEx)
EVT_TREE_SEL_CHANGING(XRCID("ID_TREE"), CSettingsDialog::OnPageChanging)
EVT_TREE_SEL_CHANGED(XRCID("ID_TREE"), CSettingsDialog::OnPageChanged)
EVT_BUTTON(XRCID("wxID_OK"), CSettingsDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CSettingsDialog::OnCancel)
END_EVENT_TABLE()

CSettingsDialog::CSettingsDialog(COptions* pOptions)
{
	m_pOptions = pOptions;
	m_activePanel = 0;
}

CSettingsDialog::~CSettingsDialog()
{
}

bool CSettingsDialog::Create(wxWindow* parent)
{
	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);
	if (!wxXmlResource::Get()->LoadDialog(this, GetParent(), _T("ID_SETTINGS")))
		return false;

	if (!LoadPages())
		return false;

	return true;
}

bool CSettingsDialog::LoadPages()
{
	// Get the tree control.
	
	wxTreeCtrl* treeCtrl = XRCCTRL(*this, "ID_TREE", wxTreeCtrl);
	wxASSERT(treeCtrl);
	if (!treeCtrl)
		return false;

	wxTreeItemId root = treeCtrl->AddRoot(_T(""));

	// Create the instances of the page classes and fill the tree.
	t_page page;
	ADD_PAGE(_("Transfer Mode"), COptionsPagePassive, page_none);
	ADD_PAGE(_("File Types"), COptionsPageFiletype, page_none);
	ADD_PAGE(_("Themes"), COptionsPageThemes, page_none);
	ADD_PAGE(_("Language"), COptionsPageLanguage, page_none);

	// Before we can initialize the pages, get the target panel in the settings
	// dialog.
	wxPanel* parentPanel = XRCCTRL(*this, "ID_PAGEPANEL", wxPanel);
	wxASSERT(parentPanel);
	if (!parentPanel)
		return false;
	
	// Keep track of maximum page size
	wxSize size;

	for (std::vector<t_page>::iterator iter = m_pages.begin(); iter != m_pages.end(); iter++)
	{
		if (!iter->page->CreatePage(m_pOptions, parentPanel, size))
			return false;
	}

	parentPanel->SetSizeHints(size);

	// Adjust pages sizes according to maximum size
	for (std::vector<t_page>::iterator iter = m_pages.begin(); iter != m_pages.end(); iter++)
	{
		iter->page->GetSizer()->SetMinSize(size);
		iter->page->GetSizer()->Fit(iter->page);
		iter->page->GetSizer()->SetSizeHints(iter->page);
	}

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

#ifdef __WXGTK__
	// Pre-show dialog under GTK, else panels won't get initialized properly
	Show();
#endif

	for (std::vector<t_page>::iterator iter = m_pages.begin(); iter != m_pages.end(); iter++)
	{
		if (!iter->page->LoadPage())
			return false;
		iter->page->Hide();
	}

	// Select first page
	treeCtrl->SelectItem(m_pages[0].id);
	if (!m_activePanel)
	{
		m_activePanel = m_pages[0].page;
		m_activePanel->Show();
	}

	return true;
}

void CSettingsDialog::OnPageChanged(wxTreeEvent& event)
{
	if (m_activePanel)
		m_activePanel->Hide();

	wxTreeItemId item = event.GetItem();

	unsigned int size = m_pages.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (m_pages[i].id == item)
		{
			m_activePanel = m_pages[i].page;
			m_activePanel->Show();
			break;
		}
	}
}

void CSettingsDialog::OnOK(wxCommandEvent& event)
{
	unsigned int size = m_pages.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (!m_pages[i].page->Validate())
			return;
	}

	for (unsigned int i = 0; i < size; i++)
		m_pages[i].page->SavePage();

	EndModal(wxID_OK);
}

void CSettingsDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CSettingsDialog::OnPageChanging(wxTreeEvent& event)
{
	if (!m_activePanel)
		return;

	if (!m_activePanel->Validate())
		event.Veto();
}
