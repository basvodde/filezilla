#include "FileZilla.h"
#include "QueueView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CQueueView::CQueueView(wxWindow* parent, wxWindowID id)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxSUNKEN_BORDER)
{
	InsertColumn(0, _("Local file"));
	InsertColumn(1, _("Direction"));
	InsertColumn(2, _("Remote file"));
}

CQueueView::~CQueueView()
{
}
