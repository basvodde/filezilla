#include "FileZilla.h"

CDirectoryListing::CDirectoryListing()
{
	m_entryCount = 0;
	m_hasUnsureEntries = 0;
	m_failed = false;
	m_hasDirs = false;
	m_has_perms = false;
	m_has_usergroup = false;
}

CDirectoryListing::CDirectoryListing(const CDirectoryListing& listing)
	: m_entries(listing.m_entries), m_searchmap_case(listing.m_searchmap_case), m_searchmap_nocase(listing.m_searchmap_nocase)
{
	path = listing.path;

	m_hasUnsureEntries = listing.m_hasUnsureEntries;
	m_failed = listing.m_failed;

	m_entryCount = listing.m_entryCount;

	m_firstListTime = listing.m_firstListTime;

	m_hasDirs = listing.m_hasDirs;

	m_has_perms = listing.m_has_perms;
	m_has_usergroup = listing.m_has_usergroup;

}

CDirectoryListing& CDirectoryListing::operator=(const CDirectoryListing &a)
{
	if (&a == this)
		return *this;

	m_entries = a.m_entries;

	path = a.path;

	m_hasUnsureEntries = a.m_hasUnsureEntries;
	m_failed = a.m_failed;

	m_entryCount = a.m_entryCount;

	m_firstListTime = a.m_firstListTime;

	m_hasDirs = a.m_hasDirs;

	m_searchmap_case = a.m_searchmap_case;
	m_searchmap_nocase = a.m_searchmap_nocase;

	m_has_perms = a.m_has_perms;
	m_has_usergroup = a.m_has_usergroup;

	return *this;
}

wxString CDirentry::dump() const
{
	wxString str = wxString::Format(_T("name=%s\nsize=%s\npermissions=%s\nownerGroup=%s\ndir=%d\nlink=%d\ntarget=%s\nhasTimestamp=%d\nunsure=%d\n"),
				name.c_str(), size.ToString().c_str(), permissions.c_str(), ownerGroup.c_str(), dir, link,
				target.c_str(), hasTimestamp, unsure);

	if (hasTimestamp != timestamp_none)
		str += _T("date=") + time.FormatISODate() + _T("\n");
	if (hasTimestamp >= timestamp_time)
		str += _T("time=") + time.FormatISOTime() + _T("\n");
	str += wxString::Format(_T("unsure=%d\n"), unsure);
	return str;
}

bool CDirentry::operator==(const CDirentry &op) const
{
	if (name != op.name)
		return false;

	if (size != op.size)
		return false;

	if (permissions != op.permissions)
		return false;

	if (ownerGroup != op.ownerGroup)
		return false;

	if (dir != op.dir)
		return false;

	if (link != op.link)
		return false;

	if (target != op.target)
		return false;

	if (hasTimestamp != op.hasTimestamp)
		return false;

	if (hasTimestamp != timestamp_none)
	{
		if (time != op.time)
			return false;
	}
	
	if (unsure != op.unsure)
		return false;

	return true;
}

void CDirectoryListing::SetCount(unsigned int count)
{
	if (count == m_entryCount)
		return;

	const unsigned int old_count = m_entryCount;

	if (!count)
	{
		m_entryCount = 0;
		return;
	}

	if (count < old_count)
	{
		m_searchmap_case.clear();
		m_searchmap_nocase.clear();
	}

	m_entries.Get().resize(count);
	
	m_entryCount = count;
}

const CDirentry& CDirectoryListing::operator[](unsigned int index) const
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	return *(*m_entries)[index];
}

CDirentry& CDirectoryListing::operator[](unsigned int index)
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	return m_entries.Get()[index].Get();
}

void CDirectoryListing::Assign(const std::list<CDirentry> &entries)
{
	m_entryCount = entries.size();

	std::vector<CRefcountObject<CDirentry> >& own_entries = m_entries.Get();
	own_entries.clear();
	own_entries.reserve(m_entryCount);
	
	m_hasDirs = false;
	m_has_perms = false;
	m_has_usergroup = false;
	
	for (std::list<CDirentry>::const_iterator iter = entries.begin(); iter != entries.end(); iter++)
	{
		if (iter->dir)
			m_hasDirs = true;
		if (!iter->permissions.empty())
			m_has_usergroup = true;
		if (!iter->ownerGroup.empty())
			m_has_perms = true;
		own_entries.push_back(CRefcountObject<CDirentry>(*iter));
	}

	m_searchmap_case.clear();
	m_searchmap_nocase.clear();
}

bool CDirectoryListing::RemoveEntry(unsigned int index)
{
	if (index >= GetCount())
		return false;

	m_searchmap_case.clear();
	m_searchmap_nocase.clear();

	std::vector<CRefcountObject<CDirentry> >& entries = m_entries.Get();
	std::vector<CRefcountObject<CDirentry> >::iterator iter = entries.begin() + index;
	if ((*iter)->dir)
		m_hasUnsureEntries |= CDirectoryListing::unsure_dir_removed;
	else
		m_hasUnsureEntries |= CDirectoryListing::unsure_file_removed;
	entries.erase(iter);

	m_entryCount--;

	return true;
}

void CDirectoryListing::GetFilenames(std::vector<wxString> &names) const
{
	names.reserve(GetCount());
	for (unsigned int i = 0; i < GetCount(); i++)
		names.push_back((*m_entries)[i]->name);
}

int CDirectoryListing::FindFile_CmpCase(const wxString& name) const
{
	if (!m_entryCount)
		return -1;

	if (!m_searchmap_case)
		m_searchmap_case.Get();

	// Search map
	std::multimap<wxString, unsigned int>::const_iterator iter = m_searchmap_case->find(name);
	if (iter != m_searchmap_case->end())
		return iter->second;

	unsigned int i = m_searchmap_case->size();
	if (i == m_entryCount)
		return -1;

	std::multimap<wxString, unsigned int>& searchmap_case = m_searchmap_case.Get();

	// Build map if not yet complete
	std::vector<CRefcountObject<CDirentry> >::const_iterator entry_iter = m_entries->begin() + i;
	for (; entry_iter != m_entries->end(); entry_iter++, i++)
	{
		const wxString& entry_name = (*entry_iter)->name;
		searchmap_case.insert(std::pair<const wxString, unsigned int>(entry_name, i));

		if (entry_name == name)
			return i;
	}

	// Map is complete, item not in it
	return -1;
}

int CDirectoryListing::FindFile_CmpNoCase(wxString name) const
{
	if (!m_entryCount)
		return -1;

	if (!m_searchmap_nocase)
		m_searchmap_nocase.Get();

	name.MakeLower();

	// Search map
	std::multimap<wxString, unsigned int>::const_iterator iter = m_searchmap_nocase->find(name);
	if (iter != m_searchmap_nocase->end())
		return iter->second;

	unsigned int i = m_searchmap_nocase->size();
	if (i == m_entryCount)
		return -1;

	std::multimap<wxString, unsigned int>& searchmap_nocase = m_searchmap_nocase.Get();

	// Build map if not yet complete
	std::vector<CRefcountObject<CDirentry> >::const_iterator entry_iter = m_entries->begin() + i;
	for (; entry_iter != m_entries->end(); entry_iter++, i++)
	{
		wxString entry_name = (*entry_iter)->name;
		entry_name.MakeLower();
		searchmap_nocase.insert(std::pair<const wxString, unsigned int>(entry_name, i));

		if (entry_name == name)
			return i;
	}

	// Map is complete, item not in it
	return -1;
}

void CDirectoryListing::ClearFindMap()
{
	if (!m_searchmap_case)
		return;

	m_searchmap_case.clear();
	m_searchmap_nocase.clear();
}
