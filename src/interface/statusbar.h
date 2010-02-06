#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include "option_change_event_handler.h"
#include "sizeformatting.h"
#include "state.h"

enum widgets
{
	widget_led_send,
	widget_led_recv,
	widget_speedlimit,
	widget_datatype,
	widget_encryption,

	WIDGET_COUNT
};

class wxStatusBarEx : public wxStatusBar
{
public:
	wxStatusBarEx(wxTopLevelWindow* parent);
	virtual ~wxStatusBarEx();

	// We override these for two reasons:
	// - wxWidgets does not provide a function to get the field widths back
	// - fixup for last field. Under MSW it has a gripper if window is not 
	//   maximized, under GTK2 it always has a gripper. These grippers overlap
	//   last field.
	virtual void SetFieldsCount(int number = 1, const int* widths = NULL);
	virtual void SetStatusWidths(int n, const int *widths);

	void SetFieldWidth(int field, int width);

	int GetGripperWidth();

#ifdef __WXGTK__
	// Basically identical to the wx one, but not calling Update
	virtual void SetStatusText(const wxString& text, int number = 0);
#endif

protected:
	int GetFieldIndex(int field);

	wxTopLevelWindow* m_pParent;
#ifdef __WXMSW__
	bool m_parentWasMaximized;
#endif

	void FixupFieldWidth(int field);

	int* m_columnWidths;

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

class CWidgetsStatusBar : public wxStatusBarEx
{
public:
	CWidgetsStatusBar(wxTopLevelWindow* parent);
	virtual ~CWidgetsStatusBar();

	// Adds a child window that gets repositioned on window resize
	// Positioned in the field given in the constructor,
	// right aligned and in reverse order.
	void AddChild(int field, int idx, wxWindow* pChild);
	
	void RemoveChild(int idx);

protected:

	struct t_statbar_child
	{
		int field;
		wxWindow* pChild;
	};

	std::map<int, struct t_statbar_child> m_children;

	void PositionChildren(int field);

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

class CStatusBar : public CWidgetsStatusBar, protected COptionChangeEventHandler, protected CStateEventHandler
{
public:
	CStatusBar(wxTopLevelWindow* parent);
	virtual ~CStatusBar();

	void DisplayQueueSize(wxLongLong totalSize, bool hasUnknown);

	void OnHandleLeftClick(wxWindow* wnd);
	void OnHandleRightClick(wxWindow* wnd);

protected:
	void UpdateSizeFormat();
	void DisplayDataType();
	void DisplayEncrypted();
	void UpdateSpeedLimitsIcon();

	void MeasureQueueSizeWidth();

	virtual void OnOptionChanged(int option);
	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2);

	CSizeFormat::_format m_sizeFormat;
	bool m_sizeFormatThousandsSep;
	int m_sizeFormatDecimalPlaces;
	wxLongLong m_size;
	bool m_hasUnknownFiles;

	wxStaticBitmap* m_pDataTypeIndicator;
	wxStaticBitmap* m_pEncryptionIndicator;
	wxStaticBitmap* m_pSpeedLimitsIndicator;
};

#endif //__STATUSBAR_H__
