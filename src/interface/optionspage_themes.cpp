#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_themes.h"
#include "themeprovider.h"

BEGIN_EVENT_TABLE(COptionsPageThemes, COptionsPage)
EVT_COMBOBOX(XRCID("ID_THEME"), COptionsPageThemes::OnThemeChange)
END_EVENT_TABLE();

COptionsPageThemes::~COptionsPageThemes()
{
	for (int i = 0; i < 3; i++)
	{
		for (std::list<wxBitmap*>::iterator iter = m_preview[i].begin(); iter != m_preview[i].end(); iter++)
			delete *iter;
	}
}
bool COptionsPageThemes::LoadPage()
{
	m_previewInitialized = false;

	bool failure = false;

	wxComboBox* pTheme = XRCCTRL(*this, "ID_THEME", wxComboBox);
	wxScrolledWindow* pPanelSmall = XRCCTRL(*this, "ID_SMALL", wxScrolledWindow);
	wxScrolledWindow* pPanelMedium = XRCCTRL(*this, "ID_MEDIUM", wxScrolledWindow);
	wxScrolledWindow* pPanelLarge = XRCCTRL(*this, "ID_LARGE", wxScrolledWindow);

	if (!pTheme || !XRCCTRL(*this, "ID_PREVIEW", wxNotebook) ||
		!pPanelSmall || !pPanelMedium ||
		!pPanelLarge || !XRCCTRL(*this, "ID_AUTHOR", wxStaticText) ||
		!XRCCTRL(*this, "ID_EMAIL", wxStaticText))
		return false;

	std::list<wxString> themes = CThemeProvider::GetThemes();
	if (themes.empty())
		return false;

	wxString theme = m_pOptions->GetOption(OPTION_THEME);
	for (std::list<wxString>::const_iterator iter = themes.begin(); iter != themes.end(); iter++)
	{
		int n = pTheme->Append(*iter);
		if (*iter == theme)
			pTheme->SetSelection(n);
	}
	if (pTheme->GetSelection() == wxNOT_FOUND)
		pTheme->SetSelection(pTheme->FindString(themes.front()));

	pPanelSmall->Connect(wxID_ANY, wxEVT_PAINT, (wxObjectEventFunction)(wxEventFunction)&COptionsPageThemes::OnPanelPaint, 0, this);
	pPanelMedium->Connect(wxID_ANY, wxEVT_PAINT, (wxObjectEventFunction)(wxEventFunction)&COptionsPageThemes::OnPanelPaint, 0, this);
	pPanelLarge->Connect(wxID_ANY, wxEVT_PAINT, (wxObjectEventFunction)(wxEventFunction)&COptionsPageThemes::OnPanelPaint, 0, this);

	wxString author, mail;
	CThemeProvider::GetThemeData(pTheme->GetString(pTheme->GetSelection()), author, mail);
	if (author == _T(""))
		author = _("N/a");
	if (mail == _T(""))
		mail = _("N/a");

	SetStaticText(XRCID("ID_AUTHOR"), author, failure);
	SetStaticText(XRCID("ID_EMAIL"), mail, failure);

	return !failure;
}

bool COptionsPageThemes::SavePage()
{
	wxComboBox* pTheme = XRCCTRL(*this, "ID_THEME", wxComboBox);
	
	m_pOptions->SetOption(OPTION_THEME, pTheme->GetString(pTheme->GetSelection()));
	return true;
}

bool COptionsPageThemes::Validate()
{
	return true;
}

void COptionsPageThemes::GetPreview()
{
	if (m_previewInitialized)
		return;

	m_previewInitialized = true;

	wxComboBox* pTheme = XRCCTRL(*this, "ID_THEME", wxComboBox);

	wxString theme = pTheme->GetString(pTheme->GetSelection());
	wxScrolledWindow* pPanels[3];
	pPanels[0] = XRCCTRL(*this, "ID_SMALL", wxScrolledWindow);
	pPanels[1] = XRCCTRL(*this, "ID_MEDIUM", wxScrolledWindow);
	pPanels[2] = XRCCTRL(*this, "ID_LARGE", wxScrolledWindow);

	int step[3] = {16,32,48};

	for (int i = 0; i < 3; i++)
	{
		for (std::list<wxBitmap*>::iterator iter = m_preview[i].begin(); iter != m_preview[i].end(); iter++)
			delete *iter;

		wxSize size = pPanels[i]->GetClientSize();
		
		wxSize imageSize(step[i], step[i]);
		m_preview[i] = CThemeProvider::GetAllImages(theme, imageSize);
		if (!m_preview[i].empty())
		{
			int lines = (m_preview[i].size() - 1) / ((size.GetWidth() - 5) / (step[i] + 5)) + 1;
			int vheight = lines * (step[i] + 5) + 5;
			if (vheight > size.GetHeight())
			{
				size.SetHeight(vheight);
				pPanels[i]->SetVirtualSize(size);
				pPanels[i]->SetVirtualSizeHints(size, size);
				pPanels[i]->SetScrollRate(0, step[i] + 5);

				wxSize size2 = pPanels[i]->GetClientSize();
				size.SetWidth(size2.GetWidth());
			
				lines = (m_preview[i].size() - 1) / ((size.GetWidth() - 5) / (step[i] + 5)) + 1;
				vheight = lines * (step[i] + 5) + 5;
				if (vheight > size.GetHeight())
					size.SetHeight(vheight);
			}
		}
		pPanels[i]->SetVirtualSize(size);
		pPanels[i]->SetVirtualSizeHints(size, size);
		pPanels[i]->SetScrollRate(0, step[i] + 5);
	}
}

void COptionsPageThemes::OnPanelPaint(wxPaintEvent& event)
{
	GetPreview();

	wxScrolledWindow* pPanels[3];
	pPanels[0] = XRCCTRL(*this, "ID_SMALL", wxScrolledWindow);
	pPanels[1] = XRCCTRL(*this, "ID_MEDIUM", wxScrolledWindow);
	pPanels[2] = XRCCTRL(*this, "ID_LARGE", wxScrolledWindow);

	int step[3] = {16,32,48};

	wxObject* object = event.GetEventObject();
	int page;
	if (object == pPanels[0])
		page = 0;
	else if (object == pPanels[1])
		page = 1;
	else if (object == pPanels[2])
		page = 2;
	else
	{
		event.Skip();
		return;
	}

	wxPaintDC dc(pPanels[page]);
	pPanels[page]->DoPrepareDC(dc);

	wxSize size = pPanels[page]->GetClientSize();

	if (m_preview[page].empty())
	{
		dc.SetFont(pPanels[page]->GetFont());
		wxString text = _("No images available");
		wxCoord w, h;
		dc.GetTextExtent(text, &w, &h);
		dc.DrawText(text, (size.GetWidth() - w) / 2, (size.GetHeight() - h) / 2);
		return;
	}

	int x = 5;
	int y = 5;

	for (std::list<wxBitmap*>::iterator iter = m_preview[page].begin(); iter != m_preview[page].end(); iter++)
	{
		wxBitmap* bitmap = *iter;
		dc.DrawBitmap(*bitmap, x, y, true);
		x += step[page] + 5;
		if ((x + step[page] + 5 + 5) >= size.GetWidth())
		{
			x = 5;
			y += step[page] + 5;
		}
	}
}

void COptionsPageThemes::OnThemeChange(wxCommandEvent& event)
{
	m_previewInitialized = false;
	XRCCTRL(*this, "ID_SMALL", wxScrolledWindow)->Refresh();
	XRCCTRL(*this, "ID_MEDIUM", wxScrolledWindow)->Refresh();
	XRCCTRL(*this, "ID_LARGE", wxScrolledWindow)->Refresh();
	
	wxComboBox* pTheme = XRCCTRL(*this, "ID_THEME", wxComboBox);

	wxString author, mail;
	CThemeProvider::GetThemeData(pTheme->GetString(pTheme->GetSelection()), author, mail);
	if (author == _T(""))
		author = _("N/a");
	if (mail == _T(""))
		mail = _("N/a");

	bool failure = false;
	SetStaticText(XRCID("ID_AUTHOR"), author, failure);
	SetStaticText(XRCID("ID_EMAIL"), mail, failure);
}
