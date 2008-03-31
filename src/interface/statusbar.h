#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

class wxStatusBarEx : public wxStatusBar
{
public:
	wxStatusBarEx(wxTopLevelWindow* parent);
	virtual ~wxStatusBarEx();

	// Adds a child window that gets repositioned on window resize
	// field >= 0: Actual field
	// -1 = last field, -2 = second-last field and so on
	//
	// cx is the horizontal offset inside the field.
	// Children are always centered vertically.
	void AddChild(int field, wxWindow* pChild, int cx);
	
	void RemoveChild(int field, wxWindow* pChild);

	// We override these for two reasons:
	// - wxWidgets does not provide a function to get the field widths back
	// - fixup for last field. Under MSW it has a gripper if window is not 
	//   maximized, under GTK2 it always has a gripper. These grippers overlap
	//   last field.
	virtual void SetFieldsCount(int number = 1, const int* widths = NULL);
	virtual void SetStatusWidths(int n, const int *widths);

	void SetFieldWidth(int field, int width);

protected:
	int GetFieldIndex(int field);

	struct t_statbar_child
	{
		int field;
		wxWindow* pChild;
		int cx;
	};

	std::list<struct t_statbar_child> m_children;

	wxTopLevelWindow* m_pParent;
#ifdef __WXMSW__
	bool m_parentWasMaximized;
#endif

	void FixupFieldWidth(int field);

	int* m_columnWidths;

	void PositionChild(const struct t_statbar_child& data);

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

class CStatusBar : public wxStatusBarEx
{
public:
	CStatusBar(wxTopLevelWindow* parent);
	virtual ~CStatusBar();

	void DisplayQueueSize(wxLongLong totalSize, bool hasUnknown);
	void UpdateSizeFormat();

	void DisplayDataType(const CServer* const pServer);
	void DisplayEncrypted(const CServer* const pServer);
	void SetCertificate(CCertificateNotification* pCertificate);
	void SetSftpEncryptionInfo(const CSftpEncryptionNotification* pEncryptionInfo);

	void OnHandleClick(wxWindow* wnd);

protected:
	void MeasureQueueSizeWidth();

	int m_sizeFormat;

	wxStaticBitmap* m_pDataTypeIndicator;
	wxStaticBitmap* m_pEncryptionIndicator;

	CCertificateNotification* m_pCertificate;
	CSftpEncryptionNotification* m_pSftpEncryptionInfo;
};

#endif //__STATUSBAR_H__
