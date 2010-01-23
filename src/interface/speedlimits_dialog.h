#ifndef __SPEEDLIMITS_DIALOG_H__
#define __SPEEDLIMITS_DIALOG_H__

#include "dialogex.h"

class CSpeedLimitsDialog : public wxDialogEx
{
public:
	CSpeedLimitsDialog() {}
	virtual ~CSpeedLimitsDialog() {}

	void Run(wxWindow* parent);

protected:
	DECLARE_EVENT_TABLE();
	void OnToggleEnable(wxCommandEvent& event);
};

#endif //__SPEEDLIMITS_DIALOG_H__
