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

	enum _flags
	{
		flag_dir = 1,
		flag_link = 2,
		flag_unsure = 4, // May be set on cached items if any changes were made to the file

		flag_timestamp_date = 0x10,
		flag_timestamp_time = 0x20,
		flag_timestamp_seconds = 0x40,

		flag_timestamp_mask = 0x70
	};
	int flags;

	inline bool is_dir() const
	{
		return (flags & flag_dir) != 0;
	}
	inline bool is_link() const
	{
		return (flags & flag_link) != 0;
	}

	inline bool is_unsure() const
	{
		return (flags & flag_unsure) != 0;
	}

	inline bool has_date() const
	{
		return (flags & flag_timestamp_date) != 0;
	}
	inline bool has_time() const
	{
		return (flags & flag_timestamp_time) != 0;
	}
	inline bool has_seconds() const
	{
		return (flags & flag_timestamp_seconds) != 0;
	}


	wxString target; // Set to linktarget it link is true

	wxDateTime time;

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

	bool m_has_perms;
	bool m_has_usergroup;

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
