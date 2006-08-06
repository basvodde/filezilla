#include "FileZilla.h"
#include "aboutdialog.h"
#include "buildinfo.h"

BEGIN_EVENT_TABLE(CAboutDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CAboutDialog::OnOK)
END_EVENT_TABLE();

bool CAboutDialog::Create(wxWindow* parent)
{
	if (!Load(parent, _T("ID_ABOUT")))
		return false;

	if (!SetLabel(XRCID("ID_VERSION"), CBuildInfo::GetVersion()))
		return false;

	wxStaticText* pHost = XRCCTRL(*this, "ID_HOST", wxStaticText);
	if (!pHost)
		return false;

	wxStaticText* pHostDesc = XRCCTRL(*this, "ID_HOST_DESC", wxStaticText);
	if (!pHostDesc)
		return false;

	wxString host = CBuildInfo::GetHostname();
	if (host == _T(""))
	{
		pHost->Hide();
		pHostDesc->Hide();
	}
	else
		pHost->SetLabel(host);

	if (!SetLabel(XRCID("ID_BUILDDATE"), CBuildInfo::GetBuildDateString()))
		return false;

	if (!SetLabel(XRCID("ID_COMPILEDWITH"), CBuildInfo::GetCompiler(), 200))
		return false;

	wxStaticText* pCompilerFlags = XRCCTRL(*this, "ID_CFLAGS", wxStaticText);
	if (!pCompilerFlags)
		return false;

	wxStaticText* pCompilerFlagsDesc = XRCCTRL(*this, "ID_CFLAGS_DESC", wxStaticText);
	if (!pCompilerFlagsDesc)
		return false;

	wxString compilerFlags = CBuildInfo::GetCompilerFlags();
	if (compilerFlags == _T(""))
	{
		pCompilerFlags->Hide();
		pCompilerFlagsDesc->Hide();
	}
	else
		pCompilerFlags->SetLabel(WrapText(this, compilerFlags, 200));

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	return true;
}

void CAboutDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

