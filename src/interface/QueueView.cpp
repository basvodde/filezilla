#include "filezilla.h"
#include "queueview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CQueueView::CQueueView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
}

CQueueView::~CQueueView()
{
}
