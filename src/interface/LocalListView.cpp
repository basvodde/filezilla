#include "filezilla.h"
#include "LocalListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLocalListView::CLocalListView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
}

CLocalListView::~CLocalListView()
{
}
