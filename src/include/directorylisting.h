#ifndef __DIRECTORYLISTING_H__
#define __DIRECTORYLISTING_H__

class CDirentry
{
public:
	wxString name;
	wxLongLong size;
	wxString permissions;
	wxString ownerGroup;
	bool dir;
	bool link;
	wxString target; // Set to linktarget it link is true

	bool hasDate;
	bool hasTime;
	wxDateTime time;

	bool unsure; // May be true on cached items if any changes were made to the file

	wxString dump() const;
	bool operator==(const CDirentry &op) const;
};

#define UNSURE_UNSURE	0x01
#define UNSURE_ADD		(0x02|UNSURE_UNSURE)
#define UNSURE_REMOVE	(0x04|UNSURE_UNSURE)
#define UNSURE_CHANGE	(0x08|UNSURE_UNSURE)
#define UNSURE_CONFUSED	(0x10|UNSURE_UNSURE)

class CDirectoryListing
{
public:
	CDirectoryListing();
	~CDirectoryListing();

	CServerPath path;
	CDirectoryListing& operator=(const CDirectoryListing &a);

	const CDirentry& operator[](unsigned int index) const;
	CDirentry& operator[](unsigned int index);

	void SetCount(unsigned int count);
	unsigned int GetCount() const { return m_entryCount; }

	// Lowest bit indicates a file got added
	// Next bit indicates a file got removed
	// 3rd bit indicates a file got changed.
	// 4th bit is set if an update cannot be applied to
	// one of the other categories.
	// 
	// These bits should help the user interface to choose an appropriate sorting
	// algorithm for modified listings
	int m_hasUnsureEntries;
	bool m_failed;
	bool m_hasDirs;

	CTimeEx m_firstListTime;

	void Assign(const std::list<CDirentry> &entries);

protected:

	void AddRef();
	void Unref();
	void Copy();

	std::vector<CDirentry> *m_pEntries;

	unsigned int m_entryCount;

	int *m_referenceCount;
};

#endif
