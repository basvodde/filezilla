#include "FileZilla.h"
#include "LocalTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLocalTreeView::CLocalTreeView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
}

CLocalTreeView::~CLocalTreeView()
{
}
