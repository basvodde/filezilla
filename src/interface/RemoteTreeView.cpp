#include "FileZilla.h"
#include "RemoteTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_CLASS(CRemoteTreeView, wxTreeCtrl)

BEGIN_EVENT_TABLE(CRemoteTreeView, wxTreeCtrl)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CRemoteTreeView::OnItemExpanding)
END_EVENT_TABLE()

class CItemData : public wxTreeItemData
{
public:
	CItemData(CServerPath path) : m_path(path) {}
	CServerPath m_path;
};

CRemoteTreeView::CRemoteTreeView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER | wxTR_HIDE_ROOT),
	CSystemImageList(16),
	CStateEventHandler(pState, STATECHANGE_REMOTE_DIR | STATECHANGE_REMOTE_DIR_MODIFIED | STATECHANGE_APPLYFILTER)
{
	m_busy = false;
	m_pQueue = pQueue;
	AddRoot(_T(""));

	CreateImageList();
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

	if (!IsExpanded(parent))
	{
		DeleteChildren(parent);
		CFilterDialog filter;
		if (HasSubdirs(*pListing, filter))
			AppendItem(parent, _T(""), -1, -1);
	}
	else
		RefreshItem(parent, *pListing);

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
				child = 0;
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
	for (unsigned int i = 0; i < listing.m_entryCount; i++)
	{
		if (!listing.m_pEntries[i].dir)
			continue;

		if (filter.FilenameFiltered(listing.m_pEntries[i].name, true, -1, false))
			continue;

		return true;
	}

	return false;
}

void CRemoteTreeView::DisplayItem(wxTreeItemId parent, const CDirectoryListing& listing)
{
	DeleteChildren(parent);

	CFilterDialog filter;
	for (unsigned int i = 0; i < listing.m_entryCount; i++)
	{
		if (!listing.m_pEntries[i].dir)
			continue;

		if (filter.FilenameFiltered(listing.m_pEntries[i].name, true, -1, false))
			continue;

		const wxString& name = listing.m_pEntries[i].name;
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
	if (GetItemText(child) == _T(""))
	{
		DisplayItem(parent, listing);
		return;
	}

	CFilterDialog filter;

	std::list<wxString> dirs;
	for (unsigned int i = 0; i < listing.m_entryCount; i++)
	{
		if (!listing.m_pEntries[i].dir)
			continue;

		if (!filter.FilenameFiltered(listing.m_pEntries[i].name, true, -1, false))
			dirs.push_back(listing.m_pEntries[i].name);
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
