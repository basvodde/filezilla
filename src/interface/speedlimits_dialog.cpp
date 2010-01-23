#include "FileZilla.h"
#include "speedlimits_dialog.h"

BEGIN_EVENT_TABLE(CSpeedLimitsDialog, wxDialogEx)
END_EVENT_TABLE()

void CSpeedLimitsDialog::Run(wxWindow* parent)
{
	if (!Load(parent, _T("ID_SPEEDLIMITS")))
		return;

	ShowModal();
}
