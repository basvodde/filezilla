#include "FileZilla.h"
#include "RemoteTreeView.h"
#include "filter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRemoteTreeView::CRemoteTreeView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER | wxTR_HIDE_ROOT),
	CSystemImageList(16),
	CStateEventHandler(pState, STATECHANGE_REMOTE_DIR | STATECHANGE_REMOTE_DIR_MODIFIED | STATECHANGE_APPLYFILTER)
{
	m_pQueue = pQueue;
	AddRoot(_T(""));

	CreateImageList();
}

CRemoteTreeView::~CRemoteTreeView()
{
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
	m_pDirectoryListing = pListing;

	if (!pListing)
	{
		DeleteAllItems();
		AddRoot(_T(""));
		return;
	}

	wxTreeItemId parent = MakeParent(pListing->path, !modified);
	if (!parent)
		return;

	CFilterDialog filter;

	DeleteChildren(parent);
	for (unsigned int i = 0; i < pListing->m_entryCount; i++)
	{
		if (!pListing->m_pEntries[i].dir)
			continue;

		if (filter.FilenameFiltered(pListing->m_pEntries[i].name, true, -1, false))
			continue;

		AppendItem(parent, pListing->m_pEntries[i].name, 0, 2);
	}
	Expand(parent);
	SortChildren(parent);
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
		wxTreeItemIdValue cookie;
		wxTreeItemId child;
		for (child = GetFirstChild(parent, cookie); child; child = GetNextSibling(child))
		{
			if (GetItemText(child) == *iter)
				break;
		}
		if (!child)
		{
			child = AppendItem(parent, *iter, 0, 2);
			SortChildren(parent);
		}
		parent = child;

		if (select)
			Expand(parent);
	}
	if (select)
	{
		SelectItem(parent);
	}
	
	return parent;
}

wxBitmap CRemoteTreeView::CreateIconWithOverlay(int index)
{
	// Create memory DC
	wxBitmap bmp(16, 16, 32);
	wxMemoryDC dc;
	dc.SelectObject(bmp);

	// Draw item from system image list
	GetSystemImageList()->Draw(index, dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);

	// Load overlay
	wxImage unknownIcon = wxArtProvider::GetBitmap(_T("ART_UNKNOWN")).ConvertToImage();
	
	// Convert mask into alpha channel
	if (!unknownIcon.HasAlpha())
	{
		wxASSERT(unknownIcon.HasMask());
		unknownIcon.InitAlpha();
	}

	// Draw overlay
	dc.DrawBitmap(unknownIcon, 0, 0, true);

    dc.SelectObject(wxNullBitmap);
	return bmp;
}

void CRemoteTreeView::CreateImageList()
{
	wxImageList* pImageList = new wxImageList(16, 16, true, 4);

	// Normal directory
	int index = GetIconIndex(dir, _T("{78013B9C-3532-4fe1-A418-5CD1955127CC}"), false);
	pImageList->Add(GetSystemImageList()->GetIcon(index));
	pImageList->Add(CreateIconWithOverlay(index));
	
	// Opened directory
	index = GetIconIndex(opened_dir, _T("{78013B9C-3532-4fe1-A418-5CD1955127CC}"), false);
	pImageList->Add(GetSystemImageList()->GetIcon(index));
	pImageList->Add(CreateIconWithOverlay(index));
	
	SetImageList(pImageList);
}
