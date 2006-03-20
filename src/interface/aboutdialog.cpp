#include "FileZilla.h"
#include "aboutdialog.h"

BEGIN_EVENT_TABLE(CAboutDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CAboutDialog::OnOK)
END_EVENT_TABLE();

bool CAboutDialog::Create(wxWindow* parent)
{
	if (!Load(parent, _T("ID_ABOUT")))
		return false;

	if (!SetLabel(XRCID("ID_VERSION"), wxString(PACKAGE_STRING, wxConvLocal)))
		return false;

	if (!SetLabel(XRCID("ID_BUILDDATE"), GetBuildDate()))
		return false;

	if (!SetLabel(XRCID("ID_COMPILEDWITH"), GetCompiler(), 200))
		return false;

	wxStaticText* pCompilerFlags = XRCCTRL(*this, "ID_CFLAGS", wxStaticText);
	if (!pCompilerFlags)
		return false;

	wxStaticText* pCompilerFlagsDesc = XRCCTRL(*this, "ID_CFLAGS_DESC", wxStaticText);
	if (!pCompilerFlagsDesc)
		return false;

#ifdef USED_CXXFLAGS
	wxString cflags(USED_CXXFLAGS, wxConvLocal);
	if (cflags == _T(""))
	{
		pCompilerFlags->Hide();
		pCompilerFlagsDesc->Hide();
	}
	else
		pCompilerFlags->SetLabel(WrapText(this, cflags, 200));
#else
	pCompilerFlags->Hide();
	pCompilerFlagsDesc->Hide();
#endif

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	return true;
}

void CAboutDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

wxString CAboutDialog::GetBuildDate()
{
	// Get build date. Unfortunately it is in the ugly Mmm dd yyyy format.
	// Make a good yyyy-mm-dd out of it
	wxString date(__DATE__, wxConvLocal);
	while (date.Replace(_T("  "), _T(" ")));

	const wxChar months[][4] = { _T("Jan"), _T("Feb"), _T("Mar"),
								_T("Apr"), _T("May"), _T("Jun"), 
								_T("Jul"), _T("Aug"), _T("Sep"),
								_T("Oct"), _T("Nov"), _T("Dev") };

	int pos = date.Find(_T(" "));
	if (pos == -1)
		return date;

	wxString month = date.Left(pos);
	int i = 0;
	for (i = 0; i < 12; i++)
	{
		if (months[i] == month)
			break;
	}
	if (i == 12)
		return date;

	wxString tmp = date.Mid(pos + 1);
	pos = tmp.Find(_T(" "));
	if (pos == -1)
		return date;

	long day;
	if (!tmp.Left(pos).ToLong(&day))
		return date;

	long year;
	if (!tmp.Mid(pos + 1).ToLong(&year))
		return date;

	return wxString::Format(_T("%04d-%02d-%02d"), year, i + 1, day);
}

wxString CAboutDialog::GetCompiler()
{
#ifdef USED_COMPILER
	return wxString(USED_COMPILER, wxConvLocal);
#elif defined __VISUALC__
	int version = __VISUALC__;
	return wxString::Format(_T("Visual C++ %d"), version);
#else
	return _T("Unknown compiler");
#endif
}
