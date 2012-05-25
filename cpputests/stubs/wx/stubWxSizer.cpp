

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/sizer.h"

wxClassInfo wxStaticBoxSizer::ms_classInfo(NULL, NULL, NULL, 0, NULL);

void wxwxSizerItemListNode::DeleteData()
{
	FAIL("wxwxSizerItemListNode::DeleteData");
}

wxSizerItem::wxSizerItem( wxSizer *sizer, int proportion, int flag, int border, wxObject* userData )
{
	FAIL("wxSizerItem::wxSizerItem");
}

wxSizerItem::wxSizerItem( int width, int height, int proportion, int flag, int border, wxObject* userData)
{
	FAIL("wxSizerItem::wxSizerItem");
}

wxSizerItem::~wxSizerItem()
{
	FAIL("wxSizerItem::~wxSizerItem");
}

wxSize wxSizerItem::CalcMin()
{
	FAIL("wxSizerItem::CalcMin");
	return wxSize();
}

wxSize wxSizerItem::GetSize() const
{
	FAIL("wxSizerItem::GetSize");
	return wxSize();
}

void wxSizerItem::SetDimension( const wxPoint& pos, const wxSize& size )
{
	FAIL("wxSizerItem::SetDimension");
}

void wxSizerItem::DeleteWindows()
{
	FAIL("wxSizerItem::DeleteWindows");
}



wxClassInfo *wxSizerItem::GetClassInfo() const
{
	FAIL("wxSizerItem::GetClassInfo");
	return NULL;
}

wxSizerItem::wxSizerItem( wxWindow *window, int proportion, int flag, int border, wxObject* userData )
{
	FAIL("wxSizerItem::wxSizerItem");
}

void wxSizerItem::SetWindow(wxWindow *window)
{
	FAIL("wxSizerItem::SetWindow");
}

bool wxSizerItem::IsShown() const
{
	FAIL("wxSizerItem::SetWindow");
	return true;
}


wxClassInfo *wxSizer::GetClassInfo() const
{
	FAIL("wxSizer::GetClassInfo");
	return NULL;
}

wxSizer::~wxSizer()
{
	FAIL("wxSizer::~wxSizer");
}

bool wxSizer::Remove( wxSizer *sizer )
{
	FAIL("wxSizer::Remove");
	return true;
}

bool wxSizer::Remove( int index )
{
	FAIL("wxSizer::Remove");
	return true;
}

bool wxSizer::Detach( wxWindow *window )
{
	FAIL("wxSizer::Detach");
	return true;
}

bool wxSizer::Detach( wxSizer *sizer )
{
	FAIL("wxSizer::Detach");
	return true;
}

bool wxSizer::Detach( int index )
{
	FAIL("wxSizer::Detach");
	return true;
}

bool wxSizer::Replace( wxWindow *oldwin, wxWindow *newwin, bool recursive )
{
	FAIL("wxSizer::Replace");
	return true;
}

bool wxSizer::Replace( wxSizer *oldsz, wxSizer *newsz, bool recursive  )
{
	FAIL("wxSizer::Replace");
	return true;
}

bool wxSizer::Replace( size_t index, wxSizerItem *newitem )
{
	FAIL("wxSizer::Replace");
	return true;
}

void wxSizer::Clear( bool delete_windows)
{
	FAIL("wxSizer::Clear");
}

void wxSizer::DeleteWindows()
{
	FAIL("wxSizer::DeleteWindows");
}

void wxSizer::SetSizeHints( wxWindow *window )
{
	FAIL("wxSizer::SetSizeHints");
}

wxSize wxSizer::Fit( wxWindow *window )
{
	FAIL("wxSizer::Fit");
	return wxSize();
}

wxSizerItem* wxSizer::GetItem( wxWindow *window, bool recursive )
{
	FAIL("wxSizer::GetItem");
	return NULL;
}

bool wxSizer::Show( wxWindow *window, bool show , bool recursive )
{
	FAIL("wxSizer::Show");
	return true;
}

void wxSizer::DoSetMinSize( int width, int height )
{
	FAIL("wxSizer::DoSetMinSize");
}

bool wxSizer::DoSetItemMinSize( wxWindow *window, int width, int height )
{
	FAIL("wxSizer::DoSetItemMinSize");
	return true;
}

bool wxSizer::DoSetItemMinSize( wxSizer *sizer, int width, int height )
{
	FAIL("wxSizer::DoSetItemMinSize");
	return true;
}

bool wxSizer::DoSetItemMinSize( size_t index, int width, int height )
{
	FAIL("wxSizer::DoSetItemMinSize");
	return true;
}

wxSizerItem* wxSizer::Insert( size_t index, wxSizerItem *item)
{
	FAIL("wxSizer::Insert");
	return NULL;
}

bool wxSizer::Remove( wxWindow *window )
{
	FAIL("wxSizer::Remove");
	return true;
}

void wxSizer::Layout()
{
	FAIL("wxSizer::Layout");
}

wxSize wxSizer::GetMinSize()
{
	FAIL("wxSizer::GetMinSize");
	return wxSize();
}

void wxSizer::ShowItems (bool show)
{
	FAIL("wxSizer::ShowItems");
}

wxSizerItem* wxSizer::GetItem( size_t index )
{
	FAIL("wxSizer::GetItem");
	return NULL;
}

wxGridSizer::wxGridSizer( int rows, int cols, int vgap, int hgap )
{
	FAIL("wxGridSizer::wxGridSizer");
}

wxGridSizer::wxGridSizer( int cols, int vgap, int hgap)
{
	FAIL("wxGridSizer::wxGridSizer");
}

void wxGridSizer::RecalcSizes()
{
	FAIL("wxGridSizer::RecalcSizes");
}

wxSize wxGridSizer::CalcMin()
{
	FAIL("wxGridSizer::CalcMin");
	return wxSize();
}

wxClassInfo *wxGridSizer::GetClassInfo() const
{
	FAIL("wxGridSizer::GetClassInfo");
	return NULL;
}


wxFlexGridSizer::wxFlexGridSizer( int cols, int vgap, int hgap)
	: wxGridSizer(cols, vgap, hgap)
{
	FAIL("wxFlexGridSizer::wxFlexGridSizer");
}

wxFlexGridSizer::~wxFlexGridSizer()
{
	FAIL("wxFlexGridSizer::~wxFlexGridSizer");
}

wxClassInfo *wxFlexGridSizer::GetClassInfo() const
{
	FAIL("wxFlexGridSizer::GetClassInfo");
	return NULL;
}

void wxFlexGridSizer::RecalcSizes()
{
	FAIL("wxFlexGridSizer::RecalcSizes");
}

wxSize wxFlexGridSizer::CalcMin()
{
	FAIL("wxFlexGridSizer::CalcMin");
	return wxSize();
}

wxBoxSizer::wxBoxSizer( int orient )
{
	FAIL("wxFlexGridSizer::wxBoxSizer");
}

wxClassInfo *wxBoxSizer::GetClassInfo() const
{
	FAIL("wxBoxSizer::GetClassInfo");
	return NULL;
}

void wxBoxSizer::RecalcSizes()
{
	FAIL("wxBoxSizer::RecalcSizes");
}

wxSize wxBoxSizer::CalcMin()
{
	FAIL("wxBoxSizer::CalcMin");
	return wxSize();
}

