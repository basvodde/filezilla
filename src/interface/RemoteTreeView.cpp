#include "FileZilla.h"
#include "RemoteTreeView.h"
#include "commandqueue.h"
#include <wx/dnd.h>
#include "dndobjects.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CItemData : public wxTreeItemData
{
public:
	CItemData(CServerPath path) : m_path(path) {}
	CServerPath m_path;
};

class CRemoteTreeViewDropTarget : public wxDropTarget
{
public:
	CRemoteTreeViewDropTarget(CRemoteTreeView* pRemoteTreeView)
		: m_pRemoteTreeView(pRemoteTreeView), m_pFileDataObject(new wxFileDataObject()),
		m_pRemoteDataObject(new CRemoteDataObject())
	{
		m_pDataObject = new wxDataObjectComposite;
		m_pDataObject->Add(m_pRemoteDataObject, true);
		m_pDataObject->Add(m_pFileDataObject, false);
		SetDataObject(m_pDataObject);
	}

	void ClearDropHighlight()
	{
		const wxTreeItemId dropHighlight = m_pRemoteTreeView->m_dropHighlight;
		if (dropHighlight != wxTreeItemId())
		{
			m_pRemoteTreeView->SetItemDropHighlight(dropHighlight, false);
			m_pRemoteTreeView->m_dropHighlight = wxTreeItemId();
		}
	}

	wxTreeItemId GetHit(const wxPoint& point)
	{
		int flags = 0;
		wxTreeItemId hit = m_pRemoteTreeView->HitTest(point, flags);

		if (flags & (wxTREE_HITTEST_ABOVE | wxTREE_HITTEST_BELOW | wxTREE_HITTEST_NOWHERE | wxTREE_HITTEST_TOLEFT | wxTREE_HITTEST_TORIGHT))
			return wxTreeItemId();

		return hit;
	}

	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
			return def;

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return wxDragNone;

		CServerPath path = m_pRemoteTreeView->GetPathFromItem(hit);
		if (path.IsEmpty())
			return wxDragNone;

		if (!GetData())
			return wxDragError;

		if (m_pDataObject->GetReceivedFormat() == m_pFileDataObject->GetFormat())
			m_pRemoteTreeView->m_pState->UploadDroppedFiles(m_pFileDataObject, path, false);
		else
		{
			if (m_pRemoteDataObject->GetProcessId() != (int)wxGetProcessId())
			{
				wxMessageBox(_("Drag&drop between different instances of FileZilla has not been implemented yet."));
				return wxDragNone;
			}

			if (!m_pRemoteTreeView->m_pState->GetServer() || m_pRemoteDataObject->GetServer() != *m_pRemoteTreeView->m_pState->GetServer())
			{
				wxMessageBox(_("Drag&drop between different servers has not been implemented yet."));
				return wxDragNone;
			}

			// Make sure path path is valid
			if (path == m_pRemoteDataObject->GetServerPath())
			{
				wxMessageBox(_("Source and path of the drop operation are identical"));
				return wxDragNone;
			}

			const std::list<CRemoteDataObject::t_fileInfo>& files = m_pRemoteDataObject->GetFiles();
			for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				const CRemoteDataObject::t_fileInfo& info = *iter;
				if (info.dir)
				{
					CServerPath dir = m_pRemoteDataObject->GetServerPath();
					dir.AddSegment(info.name);
					if (dir == path || dir.IsParentOf(path, false))
					{
						wxMessageBox(_("A directory cannot be dragged into one if its subdirectories."));
						return wxDragNone;
					}
				}
			}

			for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				const CRemoteDataObject::t_fileInfo& info = *iter;
				m_pRemoteTreeView->m_pState->m_pCommandQueue->ProcessCommand(
					new CRenameCommand(m_pRemoteDataObject->GetServerPath(), info.name, path, info.name)
					);
			}

			return wxDragNone;
		}

		return def;
	}

	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		ClearDropHighlight();

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return false;

		const CServerPath& path = m_pRemoteTreeView->GetPathFromItem(hit);
		if (path.IsEmpty())
			return false;

		return true;
	}

	CServerPath DisplayDropHighlight(wxPoint point)
	{
		wxTreeItemId hit = GetHit(point);
		if (!hit)
		{
			ClearDropHighlight();
			return CServerPath();
		}

		const CServerPath& path = m_pRemoteTreeView->GetPathFromItem(hit);

		if (path.IsEmpty())
		{
			ClearDropHighlight();
			return CServerPath();
		}

		const wxTreeItemId dropHighlight = m_pRemoteTreeView->m_dropHighlight;
		if (dropHighlight != wxTreeItemId())
			m_pRemoteTreeView->SetItemDropHighlight(dropHighlight, false);

		m_pRemoteTreeView->SetItemDropHighlight(hit, true);
		m_pRemoteTreeView->m_dropHighlight = hit;

		return path;
	}

	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
		{
			ClearDropHighlight();
			return def;
		}

		const CServerPath& path = DisplayDropHighlight(wxPoint(x, y));
		if (path.IsEmpty())
			return wxDragNone;

		if (def == wxDragLink)
			def = wxDragCopy;

		return def;
	}

	virtual void OnLeave()
	{
		ClearDropHighlight();
	}

	virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
	{
		return OnDragOver(x, y, def);
	}

protected:
	CRemoteTreeView *m_pRemoteTreeView;
	wxFileDataObject* m_pFileDataObject;
	CRemoteDataObject* m_pRemoteDataObject;
	wxDataObjectComposite* m_pDataObject;
};

IMPLEMENT_CLASS(CRemoteTreeView, wxTreeCtrl)

BEGIN_EVENT_TABLE(CRemoteTreeView, wxTreeCtrl)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CRemoteTreeView::OnItemExpanding)
EVT_TREE_SEL_CHANGED(wxID_ANY, CRemoteTreeView::OnSelectionChanged)
EVT_TREE_ITEM_ACTIVATED(wxID_ANY, CRemoteTreeView::OnItemActivated)
EVT_TREE_BEGIN_DRAG(wxID_ANY, CRemoteTreeView::OnBeginDrag)
#ifndef __WXMSW__
EVT_KEY_DOWN(CRemoteTreeView::OnKeyDown)
#endif //__WXMSW__
END_EVENT_TABLE()

CRemoteTreeView::CRemoteTreeView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER | wxTR_HIDE_ROOT),
	CSystemImageList(16),
	CStateEventHandler(pState, STATECHANGE_REMOTE_DIR | STATECHANGE_REMOTE_DIR_MODIFIED | STATECHANGE_APPLYFILTER)
{
	m_busy = false;
	m_pQueue = pQueue;
	AddRoot(_T(""));
	m_ExpandAfterList = wxTreeItemId();

	CreateImageList();

	SetDropTarget(new CRemoteTreeViewDropTarget(this));
}

CRemoteTreeView::~CRemoteTreeView()
{
	delete m_pImageList;
}

void CRemoteTreeView::OnStateChange(unsigned int event)
{
	if (event == STATECHANGE_REMOTE_DIR)
		SetDirectoryListing(m_pState->GetRemoteDir(), false);
	else if (event == STATECHANGE_REMOTE_DIR_MODIFIED)
		SetDirectoryListing(m_pState->GetRemoteDir(), true);
}

void CRemoteTreeView::SetDirectoryListing(const CDirectoryListing* pListing, bool modified)
{
	m_busy = true;

	m_pDirectoryListing = pListing;

	if (!pListing)
	{
        m_ExpandAfterList = wxTreeItemId();
		DeleteAllItems();
		AddRoot(_T(""));
		m_busy = false;
		return;
	}

	wxTreeItemId parent = MakeParent(pListing->path, !modified);
	if (!parent)
	{
		m_busy = false;
		return;
	}

	if (!IsExpanded(parent) && parent != m_ExpandAfterList)
	{
		DeleteChildren(parent);
		CFilterDialog filter;
		if (HasSubdirs(*pListing, filter))
			AppendItem(parent, _T(""), -1, -1);
	}
	else
	{
		RefreshItem(parent, *pListing);

		if (m_ExpandAfterList == parent)
			Expand(parent);
	}
	m_ExpandAfterList = wxTreeItemId();


	SetItemImages(parent, false);

	Refresh(false);

	m_busy = false;
}

wxTreeItemId CRemoteTreeView::MakeParent(CServerPath path, bool select)
{
	std::list<wxString> pieces;
	while (path.HasParent())
	{
		pieces.push_front(path.GetLastSegment());
		path = path.GetParent();
	}
	wxASSERT(path.GetPath() != _T(""));
	pieces.push_front(path.GetPath());

	wxTreeItemId parent = GetRootItem();
	for (std::list<wxString>::const_iterator iter = pieces.begin(); iter != pieces.end(); iter++)
	{
		if (iter != pieces.begin())
			path.AddSegment(*iter);

		wxTreeItemIdValue cookie;
		wxTreeItemId child;
		for (child = GetFirstChild(parent, cookie); child; child = GetNextSibling(child))
		{
			const wxString& text = GetItemText(child);
			if (text == *iter)
				break;
			if (text == _T(""))
			{
				Delete(child);
				child = wxTreeItemId();
				break;
			}
		}
		if (!child)
		{
			CDirectoryListing listing;

			if (m_pState->m_pEngine->CacheLookup(path, listing) == FZ_REPLY_OK)
			{
				child = AppendItem(parent, *iter, 0, 2, new CItemData(path));
				SetItemImages(child, false);
			}
			else
			{
				child = AppendItem(parent, *iter, 1, 3, new CItemData(path));
				SetItemImages(child, true);
			}
			SortChildren(parent);

			std::list<wxString>::const_iterator nextIter = iter;
			nextIter++;
			if (nextIter != pieces.end())
				DisplayItem(child, listing);
		}
		if (select && iter != pieces.begin())
			Expand(parent);

		parent = child;
	}
	if (select)
		SelectItem(parent);

	return parent;
}

wxBitmap CRemoteTreeView::CreateIcon(int index, const wxString& overlay /*=_T("")*/)
{
	// Create memory DC
#ifdef __WXMSW__
	wxBitmap bmp(16, 16, 32);
#else
	wxBitmap bmp(16, 16, 24);
#endif
	wxMemoryDC dc;
	dc.SelectObject(bmp);

	// Make sure the background is set correctly
	dc.SetBrush(wxBrush(GetBackgroundColour()));
	dc.SetPen(wxPen(GetBackgroundColour()));
	dc.DrawRectangle(0, 0, 16, 16);

	// Draw item from system image list
	GetSystemImageList()->Draw(index, dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);

	// Load overlay
	if (overlay != _T(""))
	{
		wxImage unknownIcon = wxArtProvider::GetBitmap(overlay, wxART_OTHER, wxSize(16,16)).ConvertToImage();

		// Convert mask into alpha channel
		if (!unknownIcon.HasAlpha())
		{
			wxASSERT(unknownIcon.HasMask());
			unknownIcon.InitAlpha();
		}

		// Draw overlay
		dc.DrawBitmap(unknownIcon, 0, 0, true);
	}

    dc.SelectObject(wxNullBitmap);
	return bmp;
}

void CRemoteTreeView::CreateImageList()
{
	m_pImageList = new wxImageList(16, 16, true, 4);

	// Normal directory
	int index = GetIconIndex(dir, _T("{78013B9C-3532-4fe1-A418-5CD1955127CC}"), false);
	m_pImageList->Add(CreateIcon(index));
	m_pImageList->Add(CreateIcon(index, _T("ART_UNKNOWN")));

	// Opened directory
	index = GetIconIndex(opened_dir, _T("{78013B9C-3532-4fe1-A418-5CD1955127CC}"), false);
	m_pImageList->Add(CreateIcon(index));
	m_pImageList->Add(CreateIcon(index, _T("ART_UNKNOWN")));

	SetImageList(m_pImageList);
}

bool CRemoteTreeView::HasSubdirs(const CDirectoryListing& listing, const CFilterDialog& filter)
{
	if (!listing.m_hasDirs)
		return false;

	for (unsigned int i = 0; i < listing.GetCount(); i++)
	{
		if (!listing[i].dir)
			continue;

		if (filter.FilenameFiltered(listing[i].name, true, -1, false))
			continue;

		return true;
	}

	return false;
}

void CRemoteTreeView::DisplayItem(wxTreeItemId parent, const CDirectoryListing& listing)
{
	DeleteChildren(parent);

	CFilterDialog filter;
	for (unsigned int i = 0; i < listing.GetCount(); i++)
	{
		if (!listing[i].dir)
			continue;

		if (filter.FilenameFiltered(listing[i].name, true, -1, false))
			continue;

		const wxString& name = listing[i].name;
		CServerPath subdir = listing.path;
		subdir.AddSegment(name);

		CDirectoryListing subListing;

		if (m_pState->m_pEngine->CacheLookup(subdir, subListing) == FZ_REPLY_OK)
		{
			wxTreeItemId child = AppendItem(parent, name, 0, 2, new CItemData(subdir));
			SetItemImages(child, false);

			if (HasSubdirs(subListing, filter))
				AppendItem(child, _T(""), -1, -1);
		}
		else
		{
			wxTreeItemId child = AppendItem(parent, name, 1, 3, new CItemData(subdir));
			SetItemImages(child, true);
		}
	}
	SortChildren(parent);
}

static bool sortfunc(const wxString& a, const wxString& b)
{
	int cmp = a.CmpNoCase(b);

	if (!cmp)
		cmp = a.Cmp(b);

	return cmp < 0;
}


void CRemoteTreeView::RefreshItem(wxTreeItemId parent, const CDirectoryListing& listing)
{
	SetItemImages(parent, false);

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild(parent, cookie);
	if (!child || GetItemText(child) == _T(""))
	{
		DisplayItem(parent, listing);
		return;
	}

	CFilterDialog filter;

	std::list<wxString> dirs;
	for (unsigned int i = 0; i < listing.GetCount(); i++)
	{
		if (!listing[i].dir)
			continue;

		if (!filter.FilenameFiltered(listing[i].name, true, -1, false))
			dirs.push_back(listing[i].name);
	}

	dirs.sort(sortfunc);

	bool inserted = false;
	child = GetLastChild(parent);
	std::list<wxString>::reverse_iterator iter = dirs.rbegin();
	while (child && iter != dirs.rend())
	{
		int cmp = GetItemText(child).CmpNoCase(*iter);
		if (!cmp)
			cmp = GetItemText(child).Cmp(*iter);

		if (!cmp)
		{
			CServerPath path = listing.path;
			path.AddSegment(*iter);

			CDirectoryListing subListing;
			if (m_pState->m_pEngine->CacheLookup(path, subListing) == FZ_REPLY_OK)
			{
				if (!GetLastChild(child) && HasSubdirs(subListing, filter))
					AppendItem(child, _T(""), -1, -1);
				SetItemImages(child, false);
			}
			else
				SetItemImages(child, true);

			child = GetPrevSibling(child);
			iter++;
		}
		else if (cmp > 0)
		{
			wxTreeItemId sel = GetSelection();
			if (sel)
			{
				do
				{
					sel = GetItemParent(sel);
					if (sel == parent)
						break;
				} while (sel);
			}
			if (!sel)
			{
				wxTreeItemId prev = GetPrevSibling(child);
				Delete(child);
				child = prev;
			}
		}
		else if (cmp < 0)
		{
			CServerPath path = listing.path;
			path.AddSegment(*iter);

			CDirectoryListing subListing;
			if (m_pState->m_pEngine->CacheLookup(path, subListing) == FZ_REPLY_OK)
			{
				wxTreeItemId child = AppendItem(parent, *iter, 0, 2, new CItemData(path));
				SetItemImages(child, false);

				if (HasSubdirs(subListing, filter))
					AppendItem(child, _T(""), -1, -1);
			}
			else
			{
				wxTreeItemId child = AppendItem(parent, *iter, 1, 3, new CItemData(path));
                if (child)
				    SetItemImages(child, true);
			}

			iter++;
			inserted = true;
		}
	}
	while (child)
	{
		wxTreeItemId sel = GetSelection();
		if (sel)
		{
			do
			{
				sel = GetItemParent(sel);
				if (sel == parent)
					break;
			} while (sel);
		}
		if (!sel)
		{
			wxTreeItemId prev = GetPrevSibling(child);
			Delete(child);
			child = prev;
		}
	}
	while (iter != dirs.rend())
	{
		CServerPath path = listing.path;
		path.AddSegment(*iter);

		CDirectoryListing subListing;
		if (m_pState->m_pEngine->CacheLookup(path, subListing) == FZ_REPLY_OK)
		{
			wxTreeItemId child = AppendItem(parent, *iter, 0, 2, new CItemData(path));
			SetItemImages(child, false);

			if (HasSubdirs(subListing, filter))
				AppendItem(child, _T(""), -1, -1);
		}
		else
		{
			wxTreeItemId child = AppendItem(parent, *iter, 1, 3, new CItemData(path));
			SetItemImages(child, true);
		}

		iter++;
		inserted = true;
	}

	if (inserted)
		SortChildren(parent);
}

int CRemoteTreeView::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
	wxString label1 = GetItemText(item1);
	wxString label2 = GetItemText(item2);

	int cmp = label1.CmpNoCase(label2);
	if (cmp)
		return cmp;
	return label1.Cmp(label2);
}

void CRemoteTreeView::OnItemExpanding(wxTreeEvent& event)
{
	if (m_busy)
		return;

	wxTreeItemId item = event.GetItem();
	if (!item)
		return;

	const CItemData* data = (CItemData*)GetItemData(item);
	wxASSERT(data);
	if (!data)
		return;

	CDirectoryListing listing;
	if (m_pState->m_pEngine->CacheLookup(data->m_path, listing) == FZ_REPLY_OK)
		RefreshItem(item, listing);
	else
	{
		SetItemImages(item, true);

		wxTreeItemId child = GetLastChild(item);
		if (!child)
		{
			event.Veto();
			return;
		}
		if (GetItemText(child) == _T(""))
		{
			DeleteChildren(item);
			event.Veto();
		}
	}
	Refresh(false);
}

void CRemoteTreeView::SetItemImages(wxTreeItemId item, bool unknown)
{
	if (!unknown)
	{
		SetItemImage(item, 0, wxTreeItemIcon_Normal);
		SetItemImage(item, 2, wxTreeItemIcon_Selected);
		SetItemImage(item, 0, wxTreeItemIcon_Expanded);
		SetItemImage(item, 2, wxTreeItemIcon_SelectedExpanded);
	}
	else
	{
		SetItemImage(item, 1, wxTreeItemIcon_Normal);
		SetItemImage(item, 3, wxTreeItemIcon_Selected);
		SetItemImage(item, 1, wxTreeItemIcon_Expanded);
		SetItemImage(item, 3, wxTreeItemIcon_SelectedExpanded);
	}
}

void CRemoteTreeView::OnSelectionChanged(wxTreeEvent& event)
{
	if (event.GetItem() != m_ExpandAfterList)
        m_ExpandAfterList = wxTreeItemId();
	if (m_busy)
		return;

	if (!m_pState->m_pCommandQueue->Idle())
	{
		wxBell();
		return;
	}

	wxTreeItemId item = event.GetItem();
	if (!item)
		return;

	const CItemData* data = (CItemData*)GetItemData(item);
	wxASSERT(data);
	if (!data)
		return;

	m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(data->m_path));
}

void CRemoteTreeView::OnItemActivated(wxTreeEvent& event)
{
	m_ExpandAfterList = GetSelection();
	event.Skip();
}

CServerPath CRemoteTreeView::GetPathFromItem(const wxTreeItemId& item) const
{
	const CItemData* const pData = (const CItemData*)GetItemData(item);

	if (!pData)
		return CServerPath();

	return pData->m_path;
}

void CRemoteTreeView::OnBeginDrag(wxTreeEvent& event)
{
	// Drag could result in recursive operation, don't allow at this point
	if (!m_pState->m_pCommandQueue->Idle())
	{
		wxBell();
		return;
	}

	const wxTreeItemId& item = event.GetItem();
	if (!item)
		return;

	const CServerPath& path = GetPathFromItem(item);
	if (path.IsEmpty() || !path.HasParent())
		return;

	const CServerPath& parent = path.GetParent();
	const wxString& lastSegment = path.GetLastSegment();
	if (lastSegment == _T(""))
		return;

	wxDataObjectComposite object;

	const CServer* const pServer = m_pState->GetServer();
	if (!pServer)
		return;

	CRemoteDataObject *pRemoteDataObject = new CRemoteDataObject(*pServer, parent);
	pRemoteDataObject->AddFile(lastSegment, true, -1);

	pRemoteDataObject->Finalize();

	object.Add(pRemoteDataObject, true);

#if FZ3_USESHELLEXT
	CShellExtensionInterface* ext = CShellExtensionInterface::CreateInitialized();
	if (ext)
	{
		const wxString& file = ext->GetDragDirectory();

		wxASSERT(file != _T(""));

		wxFileDataObject *pFileDataObject = new wxFileDataObject;
		pFileDataObject->AddFile(file);

		object.Add(pFileDataObject);
	}
#endif

	wxDropSource source(this);
	source.SetData(object);
	if (source.DoDragDrop() != wxDragCopy)
	{
#if FZ3_USESHELLEXT
		delete ext;
		ext = 0;
#endif
		return;
	}

	const wxDataFormat fmt = object.GetReceivedFormat();

#if FZ3_USESHELLEXT
	if (ext)
	{
		if (!pRemoteDataObject->DidSendData())
		{
			const CServer* const pServer = m_pState->GetServer();
			if (!pServer || !m_pState->m_pCommandQueue->Idle())
			{
				wxBell();
				delete ext;
				ext = 0;
				return;
			}

			wxString target = ext->GetTarget();
			if (target == _T(""))
			{
				delete ext;
				ext = 0;
				wxMessageBox(_("Could not determine the target of the Drag&Drop operation.\nEither the shell extension is not installed properly or you didn't drop the files into an Explorer window."));
				return;
			}

			m_pState->DownloadDroppedFiles(pRemoteDataObject, target);

			delete ext;
			ext = 0;
			return;
		}
		delete ext;
		ext = 0;
	}
#endif
}

void CRemoteTreeView::OnKeyDown(wxKeyEvent& event)
{
	if (event.GetKeyCode() != WXK_TAB)
	{
		event.Skip();
		return;
	}

	wxNavigationKeyEvent navEvent;
	navEvent.SetEventObject(this);
	navEvent.SetDirection(!event.ShiftDown());
	navEvent.SetFromTab(true);
	navEvent.ResumePropagation(1);
	ProcessEvent(navEvent);
}
