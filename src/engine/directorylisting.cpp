#include "FileZilla.h"

CDirectoryListing::CDirectoryListing()
{
	m_pEntries = 0;
	m_entryCount = 0;
	m_hasUnsureEntries = 0;
	m_failed = false;
	m_referenceCount = 0;
	m_hasDirs = false;
	m_searchmap_case = 0;
	m_searchmap_nocase = 0;
}

CDirectoryListing::CDirectoryListing(const CDirectoryListing& listing)
{
	m_referenceCount = listing.m_referenceCount;
	m_pEntries = listing.m_pEntries;
	if (m_referenceCount)
		(*m_referenceCount)++;

	path = listing.path;

	m_hasUnsureEntries = listing.m_hasUnsureEntries;
	m_failed = listing.m_failed;

	m_entryCount = listing.m_entryCount;

	m_firstListTime = listing.m_firstListTime;

	m_hasDirs = listing.m_hasDirs;

	m_searchmap_case = listing.m_searchmap_case;
	m_searchmap_nocase = listing.m_searchmap_nocase;
}

CDirectoryListing::~CDirectoryListing()
{
	Unref();
}

CDirectoryListing& CDirectoryListing::operator=(const CDirectoryListing &a)
{
	if (&a == this)
		return *this;

	if (m_referenceCount && m_referenceCount == a.m_referenceCount)
	{
		// References the same listing
		return *this;
	}

	Unref();

	m_referenceCount = a.m_referenceCount;
	m_pEntries = a.m_pEntries;
	if (m_referenceCount)
		(*m_referenceCount)++;

	path = a.path;

	m_hasUnsureEntries = a.m_hasUnsureEntries;
	m_failed = a.m_failed;

	m_entryCount = a.m_entryCount;

	m_firstListTime = a.m_firstListTime;

	m_hasDirs = a.m_hasDirs;

	m_searchmap_case = a.m_searchmap_case;
	m_searchmap_nocase = a.m_searchmap_nocase;

	return *this;
}

wxString CDirentry::dump() const
{
	wxString str = wxString::Format(_T("name=%s\nsize=%s\npermissions=%s\nownerGroup=%s\ndir=%d\nlink=%d\ntarget=%s\nhasDate=%d\nhasTime=%d\nunsure=%d\n"),
				name.c_str(), size.ToString().c_str(), permissions.c_str(), ownerGroup.c_str(), dir, link,
				target.c_str(), hasDate, hasTime, unsure);

	if (hasDate)
		str += _T("date=") + time.FormatISODate() + _T("\n");
	if (hasTime)
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

	if (hasDate != op.hasDate)
		return false;

	if (hasTime != op.hasTime)
		return false;

	if (hasDate)
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
		Unref();
		m_entryCount = 0;
		return;
	}

	if (count < old_count)
	{
		Copy(false);
		m_searchmap_case->clear();
		m_searchmap_nocase->clear();
	}
	else
		Copy(true);

	wxASSERT(m_pEntries);
	m_pEntries->resize(count);
	
	m_entryCount = count;
}

const CDirentry& CDirectoryListing::operator[](unsigned int index) const
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	const CDirentryObject& entryObject = (*m_pEntries)[index];
	return entryObject.GetEntry();
}

CDirentry& CDirectoryListing::operator[](unsigned int index)
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);

	Copy(true);

	return (*m_pEntries)[index].GetEntry();
}

void CDirectoryListing::Unref()
{
	if (!m_referenceCount)
		return;

	wxASSERT(*m_referenceCount > 0);

	if (*m_referenceCount > 1)
	{
		(*m_referenceCount)--;
		return;
	}

	delete m_pEntries;
	m_pEntries = 0;

	delete m_referenceCount;
	m_referenceCount = 0;

	delete m_searchmap_case;
	m_searchmap_case = 0;

	delete m_searchmap_nocase;
	m_searchmap_nocase = 0;
}

void CDirectoryListing::AddRef()
{
	if (!m_referenceCount)
	{
		// New object
		m_referenceCount = new int(1);
		m_pEntries = new std::vector<CDirentryObject>;

		m_searchmap_case = new std::multimap<wxString, unsigned int>;
		m_searchmap_nocase = new std::multimap<wxString, unsigned int>;

		return;
	}
	(*m_referenceCount)++;
}

void CDirectoryListing::Copy(bool copy_cache)
{
	if (!m_referenceCount)
	{
		AddRef();
		return;
	}

	if (*m_referenceCount == 1)
	{
		// Only instance
		return;
	}

	(*m_referenceCount)--;
	m_referenceCount = new int(1);
	
	std::vector<CDirentryObject>* pEntries = new std::vector<CDirentryObject>;
	*pEntries = *m_pEntries;
	m_pEntries = pEntries;

	if (copy_cache)
	{
		std::multimap<wxString, unsigned int>* searchmap_case = new std::multimap<wxString, unsigned int>;
		*searchmap_case = *searchmap_case;
		m_searchmap_case = searchmap_case;

		std::multimap<wxString, unsigned int>* searchmap_nocase = new std::multimap<wxString, unsigned int>;
		*searchmap_nocase = *searchmap_nocase;
		m_searchmap_nocase = searchmap_nocase;
	}
	else
	{
		m_searchmap_case = new std::multimap<wxString, unsigned int>;
		m_searchmap_nocase = new std::multimap<wxString, unsigned int>;
	}
}

void CDirectoryListing::Assign(const std::list<CDirentry> &entries)
{
	Unref();
	AddRef();

	m_entryCount = entries.size();
	m_pEntries->reserve(m_entryCount);
	
	m_hasDirs = false;
	
	for (std::list<CDirentry>::const_iterator iter = entries.begin(); iter != entries.end(); iter++)
	{
		if (iter->dir)
			m_hasDirs = true;
		m_pEntries->push_back(*iter);
	}
}

bool CDirectoryListing::RemoveEntry(unsigned int index)
{
	if (index >= GetCount())
		return false;

	Copy(false);
	m_searchmap_case->clear();
	m_searchmap_nocase->clear();

	std::vector<CDirentryObject>::iterator iter = m_pEntries->begin() + index;
	if (iter->GetEntry().dir)
		m_hasUnsureEntries |= CDirectoryListing::unsure_dir_removed;
	else
		m_hasUnsureEntries |= CDirectoryListing::unsure_file_removed;
	m_pEntries->erase(iter);

	m_entryCount--;

	return true;
}

CDirentryObject::CDirentryObject()
{
	m_pEntry = 0;
	m_pReferenceCount = 0;
}

CDirentryObject::CDirentryObject(const CDirentryObject& entryObject)
{
	m_pEntry = entryObject.m_pEntry;
	m_pReferenceCount = entryObject.m_pReferenceCount;
	if (m_pReferenceCount)
		(*m_pReferenceCount)++;
}

CDirentryObject::~CDirentryObject()
{
	Unref();
}

CDirentryObject& CDirentryObject::operator=(const CDirentryObject &a)
{
	if (&a == this)
		return *this;

	if (m_pReferenceCount && m_pReferenceCount == a.m_pReferenceCount)
	{
		// References the same listing
		return *this;
	}

	Unref();

	m_pEntry = a.m_pEntry;
	m_pReferenceCount = a.m_pReferenceCount;
	if (m_pReferenceCount)
		(*m_pReferenceCount)++;

	return *this;
}

void CDirentryObject::Unref()
{
	if (!m_pReferenceCount)
		return;

	if (*m_pReferenceCount > 1)
	{
		(*m_pReferenceCount)--;
		return;
	}

	delete m_pReferenceCount;
	m_pReferenceCount = 0;
	delete m_pEntry;
	m_pEntry = 0;
}

void CDirentryObject::Copy()
{
	if (!m_pReferenceCount)
	{
		m_pEntry = new CDirentry;
		m_pReferenceCount = new int(1);
		return;
	}
	if (*m_pReferenceCount == 1)
		return;
	
	(*m_pReferenceCount)--;
	m_pEntry = new CDirentry(*m_pEntry);
	m_pReferenceCount = new int(1);
}

CDirentryObject::CDirentryObject(const CDirentry& entry)
{
	m_pEntry = new CDirentry(entry);
	m_pReferenceCount = new int(1);
}

const CDirentry& CDirentryObject::GetEntry() const
{
	return *m_pEntry;
}

CDirentry& CDirentryObject::GetEntry()
{
	Copy();
	return *m_pEntry;
}

void CDirectoryListing::GetFilenames(std::vector<wxString> &names) const
{
	names.reserve(GetCount());
	for (unsigned int i = 0; i < GetCount(); i++)
		names.push_back((*this)[i].name);
}

int CDirectoryListing::FindFile_CmpCase(const wxString& name) const
{
	if (!m_pEntries)
		return -1;

	// Search map
	std::multimap<wxString, unsigned int>::const_iterator iter = m_searchmap_case->find(name);
	if (iter != m_searchmap_case->end())
		return iter->second;

	unsigned int i = m_searchmap_case->size();

	// Build map if not yet complete
	std::vector<CDirentryObject>::const_iterator entry_iter = m_pEntries->begin() + i;
	for (; entry_iter != m_pEntries->end(); entry_iter++, i++)
	{
		const wxString& entry_name = entry_iter->GetEntry().name;
		m_searchmap_case->insert(std::pair<wxString, unsigned int>(entry_name, i));

		if (entry_name == name)
			return i;
	}

	// Map is complete, item not in it
	return -1;
}

int CDirectoryListing::FindFile_CmpNoCase(wxString name) const
{
	if (!m_pEntries)
		return -1;

	name.MakeLower();

	// Search map
	std::multimap<wxString, unsigned int>::const_iterator iter = m_searchmap_nocase->find(name);
	if (iter != m_searchmap_nocase->end())
		return iter->second;

	unsigned int i = m_searchmap_nocase->size();

	// Build map if not yet complete
	std::vector<CDirentryObject>::const_iterator entry_iter = m_pEntries->begin() + i;
	for (; entry_iter != m_pEntries->end(); entry_iter++, i++)
	{
		wxString entry_name = entry_iter->GetEntry().name;
		entry_name.MakeLower();
		m_searchmap_nocase->insert(std::pair<wxString, unsigned int>(entry_name, i));

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

	m_searchmap_case->clear();
	m_searchmap_nocase->clear();
}
