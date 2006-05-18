#ifndef __REMOTETREEVIEW_H__
#define __REMOTETREEVIEW_H__

#include "systemimagelist.h"
#include "state.h"

class CQueueView;
class CRemoteTreeView : public wxTreeCtrl, CSystemImageList, CStateEventHandler
{
public:
	CRemoteTreeView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue);
	virtual ~CRemoteTreeView();

protected:
	wxTreeItemId MakeParent(CServerPath path, bool select);
	void SetDirectoryListing(const CDirectoryListing* pListing, bool modified);
	virtual void OnStateChange(unsigned int event);

	CQueueView* m_pQueue;
	const CDirectoryListing* m_pDirectoryListing;

	void CreateImageList();
};

#endif
