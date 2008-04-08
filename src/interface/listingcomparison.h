#ifndef __LISTINGCOMPARISON_H__
#define __LISTINGCOMPARISON_H__

class CComparisonManager;
class CComparableListing
{
	friend class CComparisonManager;
public:
	CComparableListing(wxWindow* pParent);
	virtual ~CComparableListing() {}

	enum t_fileEntryFlags
	{
		normal,
		fill,
		different,
		newer,
		lonely
	};

	virtual bool CanStartComparison(wxString* pError) = 0;
	virtual void StartComparison() = 0;
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size, wxDateTime& date, bool &hasTime) = 0;
	virtual void CompareAddFile(t_fileEntryFlags flags) = 0;
	virtual void FinishComparison() = 0;
	virtual void ScrollTopItem(int item) = 0;
	virtual void OnExitComparisonMode() = 0;
	
	void RefreshComparison();
	void ExitComparisonMode();

	bool IsComparing() const;

	void SetOther(CComparableListing* pOther) { m_pOther = pOther; }
	CComparableListing* GetOther() { return m_pOther; }

protected:

	wxListItemAttr m_comparisonBackgrounds[3];

private:
	wxWindow* m_pParent;

	CComparableListing* m_pOther;
	CComparisonManager* m_pComparisonManager;
};

class CMainFrame;
class CComparisonManager
{
public:
	CComparisonManager(CMainFrame* pMainFrame, CComparableListing* pLeft, CComparableListing* pRight);

	bool CompareListings();
	bool IsComparing() const { return m_isComparing; }

	void ExitComparisonMode();

	void UpdateToolState();

protected:
	int CompareFiles(const int dirSortMode, const wxString& local, const wxString& remote, bool localDir, bool remoteDir);

	CMainFrame* m_pMainFrame;

	// Left/right, first/second, a/b, doesn't matter
	CComparableListing* m_pLeft;
	CComparableListing* m_pRight;

	bool m_isComparing;
};

#endif //__LISTINGCOMPARISON_H__
