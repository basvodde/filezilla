#ifndef __FILELIST_STATUSBAR_H__
#define __FILELIST_STATUSBAR_H__

class CFilelistStatusBar : public wxStatusBar
{
public:
	CFilelistStatusBar(wxWindow* pParent);

	void SetDirectoryContents(int count_files, int count_dirs, const wxLongLong &total_size, int unknown_size);
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

protected:

	int m_count_files;
	int m_count_dirs;
	wxLongLong m_total_size;
	int m_unknown_size; // Set to true if there are files with unknown size

	int m_count_selected_files;
	int m_count_selected_dirs;
	wxLongLong m_total_selected_size;
	int m_unknown_selected_size; // Set to true if there are files with unknown size

	wxTimer m_updateTimer;

	DECLARE_EVENT_TABLE()
	void OnTimer(wxTimerEvent& event);
};

#endif //__FILELIST_STATUSBAR_H__
