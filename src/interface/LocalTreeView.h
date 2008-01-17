#ifndef __LOCALTREEVIEW_H__
#define __LOCALTREEVIEW_H__

#include "systemimagelist.h"
#include "state.h"

class CQueueView;

#ifdef __WXMSW__
class CVolumeDescriptionEnumeratorThread;
#endif

class CLocalTreeView : public wxTreeCtrl, CSystemImageList, CStateEventHandler
{
	DECLARE_CLASS(CLocalTreeView)

	friend class CLocalTreeViewDropTarget;

public:
	CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView);
	virtual ~CLocalTreeView();

protected:
	virtual void OnStateChange(unsigned int event, const wxString& data);

	void SetDir(wxString localDir);
	void Refresh();

#ifdef __WXMSW__
	bool CreateRoot();
	bool DisplayDrives(wxTreeItemId parent);
	wxString GetSpecialFolder(int folder, int &iconIndex, int &openIconIndex);

	wxTreeItemId m_desktop;
	wxTreeItemId m_drives;
	wxTreeItemId m_documents;
#endif

	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);

	wxTreeItemId GetNearestParent(wxString& localDir);
	wxTreeItemId GetSubdir(wxTreeItemId parent, const wxString& subDir);
	void DisplayDir(wxTreeItemId parent, const wxString& dirname, const wxString& knownSubdir = _T(""));
	bool HasSubdir(const wxString& dirname);
	wxTreeItemId MakeSubdirs(wxTreeItemId parent, wxString dirname, wxString subDir);
	wxString m_currentDir;

	DECLARE_EVENT_TABLE()
	void OnItemExpanding(wxTreeEvent& event);
	void OnSelectionChanged(wxTreeEvent& event);
	void OnBeginDrag(wxTreeEvent& event);
#ifndef __WXMSW__
	void OnKeyDown(wxKeyEvent& event);
#else
	void OnVolumesEnumerated(wxCommandEvent& event);
	CVolumeDescriptionEnumeratorThread* m_pVolumeEnumeratorThread;
#endif
	void OnContextMenu(wxTreeEvent& event);
	void OnMenuUpload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnBeginLabelEdit(wxTreeEvent& event);
	void OnEndLabelEdit(wxTreeEvent& event);
	
	wxString GetDirFromItem(wxTreeItemId item);

	CQueueView* m_pQueueView;

	bool m_setSelection;

	wxTreeItemId m_contextMenuItem;
	wxTreeItemId m_dropHighlight;
};

#ifdef __WXMSW__

DECLARE_EVENT_TYPE(fzEVT_VOLUMESENUMERATED, -1)

// Windows has this very strange concept of drive letters (nowadays called
// volumes), even if the drive isn't mounted (in the sense of no media
// inserted).
// This can result in a long seek time if trying to enumerate the volume
// labels, especially with legacy floppy drives (why are people still using
// them?)
// Since the local directory including the drives is populated at startup,
// use a background thread to obtain the labels.
class CVolumeDescriptionEnumeratorThread : protected wxThreadEx
{
public:
	CVolumeDescriptionEnumeratorThread(wxEvtHandler* pEvtHandler);
	virtual ~CVolumeDescriptionEnumeratorThread();

	bool Failed() const { return m_failure; }

	struct t_VolumeInfo
	{
		wxString volume;
		wxString volumeName;
	};

	std::list<t_VolumeInfo> GetVolumes();

protected:
	bool GetDrives();
	virtual ExitCode Entry();

	wxEvtHandler* m_pEvtHandler;

	bool m_failure;
	bool m_stop;
	bool m_running;

	struct t_VolumeInfoInternal
	{
		wxChar* pVolume;
		wxChar* pVolumeName;
	};

	std::list<t_VolumeInfoInternal> m_volumeInfo;
};

#endif

#endif
