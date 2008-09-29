#ifndef __DIRECTORYLISTING_H__
#define __DIRECTORYLISTING_H__

#include <map>

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

#include "refcount.h"

class CDirectoryListing
{
public:
	CDirectoryListing();
	CDirectoryListing(const CDirectoryListing& listing);

	CServerPath path;
	CDirectoryListing& operator=(const CDirectoryListing &a);

	const CDirentry& operator[](unsigned int index) const;

	// Word of caution: You MUST NOT change the name of the returned
	// entry if you do not call ClearFindMap afterwards
	CDirentry& operator[](unsigned int index);

	void SetCount(unsigned int count);
	unsigned int GetCount() const { return m_entryCount; }

	int FindFile_CmpCase(const wxString& name) const;
	int FindFile_CmpNoCase(wxString name) const;

	void ClearFindMap();

	enum
	{
		unsure_file_added = 0x01,
		unsure_file_removed = 0x02,
		unsure_file_changed = 0x04,
		unsure_file_mask = 0x07,
		unsure_dir_added = 0x08,
		unsure_dir_removed = 0x10,
		unsure_dir_changed = 0x20,
		unsure_dir_mask = 0x38,
		unsure_unknown = 0x40,
		unsure_invalid = 0x80 // Recommended action: Do a full refresh
	};
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

	bool RemoveEntry(unsigned int index);

	void GetFilenames(std::vector<wxString> &names) const;

protected:

	CRefcountObject_Uninitialized<std::vector<CRefcountObject<CDirentry> > > m_entries;

	mutable CRefcountObject_Uninitialized<std::multimap<wxString, unsigned int> > m_searchmap_case;
	mutable CRefcountObject_Uninitialized<std::multimap<wxString, unsigned int> > m_searchmap_nocase;

	unsigned int m_entryCount;
};

#endif
