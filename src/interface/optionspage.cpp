#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"

bool COptionsPage::CreatePage(COptions* pOptions, wxWindow* parent, wxSize& maxSize)
{
	m_pOptions = pOptions;

	if (!wxXmlResource::Get()->LoadPanel(this, parent, GetResourceName()))
		return false;

	wxSize size = GetSize();
	if (size.GetWidth() > maxSize.GetWidth())
		maxSize.SetWidth(size.GetWidth());
	if (size.GetHeight() > maxSize.GetHeight())
		maxSize.SetHeight(size.GetHeight());

	Hide();

	return true;
}
