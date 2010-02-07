#include <filezilla.h>

#include "aboutdialog.h"
#include "buildinfo.h"

#include <misc.h>

#include <wx/hyperlink.h>
#include <wx/clipbrd.h>

BEGIN_EVENT_TABLE(CAboutDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CAboutDialog::OnOK)
EVT_BUTTON(XRCID("ID_COPY"), CAboutDialog::OnCopy)
END_EVENT_TABLE();

bool CAboutDialog::Create(wxWindow* parent)
{
	if (!Load(parent, _T("ID_ABOUT")))
		return false;

	XRCCTRL(*this, "ID_URL", wxHyperlinkCtrl)->SetLabel(_T("http://filezilla-project.org"));

	XRCCTRL(*this, "ID_COPYRIGHT", wxStaticText)->SetLabel(_T("Copyright (C) 2004-2010  Tim Kosse"));

	wxString version = CBuildInfo::GetVersion();
	if (CBuildInfo::GetBuildType() == _T("nightly"))
		version += _T("-nightly");
	if (!SetLabel(XRCID("ID_VERSION"), version))
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

	wxStaticText* pBuild = XRCCTRL(*this, "ID_BUILD", wxStaticText);
	if (!pBuild)
		return false;

	wxStaticText* pBuildDesc = XRCCTRL(*this, "ID_BUILD_DESC", wxStaticText);
	if (!pBuildDesc)
		return false;

	wxString build = CBuildInfo::GetBuildSystem();
	if (build == _T(""))
	{
		pBuild->Hide();
		pBuildDesc->Hide();
	}
	else
		pBuild->SetLabel(build);

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
	{
		WrapText(this, compilerFlags, 200);
		pCompilerFlags->SetLabel(compilerFlags);
	}

	wxStaticText* pVer_wx = XRCCTRL(*this, "ID_VER_WX", wxStaticText);
	if (pVer_wx)
		pVer_wx->SetLabel(wxVERSION_NUM_DOT_STRING_T);

	wxStaticText* pVer_gnutls = XRCCTRL(*this, "ID_VER_GNUTLS", wxStaticText);
	if (pVer_gnutls)
		pVer_gnutls->SetLabel(GetDependencyVersion(dependency_gnutls));

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	return true;
}

void CAboutDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

void CAboutDialog::OnCopy(wxCommandEvent& event)
{
	wxString text = _T("FileZilla Client\n");
	text += _T("----------------\n\n");

	text += _T("Version:          ") + CBuildInfo::GetVersion();
	if (CBuildInfo::GetBuildType() == _T("nightly"))
		text += _T("-nightly");
	text += '\n';

	text += _T("\nBuild information:\n");
	
	wxString host = CBuildInfo::GetHostname();
	if (!host.empty())
		text += _T("  Compiled for:   ") + host + _T("\n");

	wxString build = CBuildInfo::GetBuildSystem();
	if (!build.empty())
		text += _T("  Compiled on:    ") + build + _T("\n");

	text += _T("  Build date:     ") + CBuildInfo::GetBuildDateString() + _T("\n");

	text += _T("  Compiled with:  ") + CBuildInfo::GetCompiler() + _T("\n");

	wxString compilerFlags = CBuildInfo::GetCompilerFlags();
	if (!compilerFlags.empty())
		text += _T("  Compiler flags: ") + compilerFlags + _T("\n");

	text += _T("\nLinked against:\n  wxWidgets:      ") + wxString(wxVERSION_NUM_DOT_STRING_T) + _T("\n");
	text += _T("  GnuTLS:         ") + GetDependencyVersion(dependency_gnutls) + _T("\n");

#ifdef __WXMSW__
	text.Replace(_T("\n"), _T("\r\n"));
#endif

	if (!wxTheClipboard->Open())
	{
		wxMessageBox(_("Could not open clipboard"), _("Could not copy data"), wxICON_EXCLAMATION);
		return;
	}

	wxTheClipboard->SetData(new wxTextDataObject(text));
	wxTheClipboard->Flush();
	wxTheClipboard->Close();
}
