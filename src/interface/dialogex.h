#ifndef __DIALOGEX_H__
#define __DIALOGEX_H__

class wxDialogEx : public wxDialog
{
	DECLARE_EVENT_TABLE();
	virtual void OnChar(wxKeyEvent& event);
};

#endif //__DIALOGEX_H__
