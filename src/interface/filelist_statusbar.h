#ifndef __FILELIST_STATUSBAR_H__
#define __FILELIST_STATUSBAR_H__

#include <option_change_event_handler.h>

class CFilelistStatusBar : public wxStatusBar, protected COptionChangeEventHandler
{
public:
	CFilelistStatusBar(wxWindow* pParent);

	void SetDirectoryContents(int count_files, int count_dirs, const wxLongLong &total_size, int unknown_size, int hidden);
	void Clear();
	void SetHidden(int hidden);
	void TriggerUpdateText();
	void UpdateText();

	void AddFile(const wxLongLong& size);
	void RemoveFile(const wxLongLong& size);
	void AddDirectory();
	void RemoveDirectory();

	void SelectAll();
	void UnselectAll();
	void SelectFile(const wxLongLong &size);
	void UnselectFile(const wxLongLong &size);
	void SelectDirectory();
	void UnselectDirectory();

	void SetEmptyString(const wxString& empty);

	void SetConnected(bool connected);
protected:

	virtual void OnOptionChanged(int option);

	bool m_connected;
	int m_count_files;
	int m_count_dirs;
	wxLongLong m_total_size;
	int m_unknown_size; // Set to true if there are files with unknown size
	int m_hidden;

	int m_count_selected_files;
	int m_count_selected_dirs;
	wxLongLong m_total_selected_size;
	int m_unknown_selected_size; // Set to true if there are files with unknown size

	wxTimer m_updateTimer;

	wxString m_empty_string;
	wxString m_offline_string;

	DECLARE_EVENT_TABLE()
	void OnTimer(wxTimerEvent& event);
};

#endif //__FILELIST_STATUSBAR_H__
