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

class CDirectoryListing
{
public:
	CDirectoryListing();
	~CDirectoryListing();

	unsigned int m_entryCount;

	CServerPath path;
	CDirentry *m_pEntries;
	CDirectoryListing& operator=(const CDirectoryListing &a);

	bool m_hasUnsureEntries;
	bool m_failed;
};

#endif

