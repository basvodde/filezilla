#ifndef __LISTINGCOMPARISON_H__
#define __LISTINGCOMPARISON_H__

class CComparableListing
{
public:
	CComparableListing(wxWindow* pParent);
	virtual ~CComparableListing() {}

	enum t_fileEntryFlags
	{
		normal,
		fill,
		different,
		lonely
	};

	virtual bool CanStartComparison(wxString* pError) = 0;
	virtual void StartComparison() = 0;
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size) = 0;
	virtual void CompareAddFile(t_fileEntryFlags flags) = 0;
	virtual void FinishComparison() = 0;
	virtual void ScrollTopItem(int item) = 0;

	void SetOther(CComparableListing* pOther) { m_pOther = pOther; }
	CComparableListing* GetOther() { return m_pOther; }

protected:

	wxListItemAttr m_comparisonBackgrounds[2];

private:
	wxWindow* m_pParent;

	CComparableListing* m_pOther;
};

class CComparisonManager
{
public:
	CComparisonManager(CComparableListing* pLeft, CComparableListing* pRight);

	bool CompareListings();

protected:
	int CompareFiles(const int dirSortMode, const wxString& local, const wxString& remote, bool localDir, bool remoteDir);

	// Left/right, first/second, a/b, doesn't matter
	CComparableListing* m_pLeft;
	CComparableListing* m_pRight;
};

#endif //__LISTINGCOMPARISON_H__
