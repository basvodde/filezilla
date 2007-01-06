#include "FileZilla.h"

CDirectoryListing::CDirectoryListing()
{
	m_pEntries = 0;
	m_entryCount = 0;
	m_hasUnsureEntries = false;
	m_failed = false;
	m_referenceCount = 0;
	m_hasDirs = false;
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

	return *this;
}

wxString CDirentry::dump() const
{
	wxString str = wxString::Format(_T("name=%s\nsize=%s\npermissions=%s\nownerGroup=%s\ndir=%d\nlink=%d\ntarget=%s\nhasDate=%d\nhasTime=%d\n"),
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

	if (time != op.time)
		return false;
	
	if (unsure != op.unsure)
		return false;

	return true;
}

void CDirectoryListing::SetCount(unsigned int count)
{
	if (count == m_entryCount)
		return;

	if (!count)
	{
		Unref();
		m_entryCount = 0;
		return;
	}
	else
		Copy();

	wxASSERT(m_pEntries);
	m_pEntries->resize(count);
	
	m_entryCount = count;
}

const CDirentry& CDirectoryListing::operator[](unsigned int index) const
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	return (*m_pEntries)[index];
}

CDirentry& CDirectoryListing::operator[](unsigned int index)
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);

	Copy();

	return (*m_pEntries)[index];
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
}

void CDirectoryListing::AddRef()
{
	if (!m_referenceCount)
	{
		// New object
		m_referenceCount = new int(1);
		m_pEntries = new std::vector<CDirentry>;
		return;
	}
	(*m_referenceCount)++;
}

void CDirectoryListing::Copy()
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
	

	std::vector<CDirentry>* pEntries = new std::vector<CDirentry>;
	*pEntries = *m_pEntries;
	m_pEntries = pEntries;
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
