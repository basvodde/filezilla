#include "FileZilla.h"

CDirectoryListing::CDirectoryListing()
{
	m_pEntries = 0;
	m_entryCount = 0;
	m_hasUnsureEntries = false;
	m_failed = false;
}

CDirectoryListing::~CDirectoryListing()
{
	delete [] m_pEntries;
}

CDirectoryListing& CDirectoryListing::operator=(const CDirectoryListing &a)
{
	if (&a == this)
		return *this;

	if (m_pEntries)
		delete [] m_pEntries;

	path = a.path;

	m_hasUnsureEntries = a.m_hasUnsureEntries;
	m_failed = a.m_failed;

	m_entryCount = a.m_entryCount;
	if (m_entryCount)
	{
		m_pEntries = new CDirentry[m_entryCount];
		for (unsigned int i = 0; i < m_entryCount; i++)
			m_pEntries[i] = a.m_pEntries[i];
	}
	else
		m_pEntries = 0;

	m_firstListTime = a.m_firstListTime;

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
		if (!m_entryCount)
			return;

		delete [] m_pEntries;
		m_pEntries = 0;
		m_entryCount = 0;
		return;
	}
	else if (count > m_entryCount)
	{
		CDirentry *entries = new CDirentry[count];
		
		if (m_entryCount)
		{
			for (unsigned int i = 0; i < m_entryCount; i++)
				entries[i] = m_pEntries[i];
			delete [] m_pEntries;
		}
		m_pEntries = entries;
	}

	m_entryCount = count;
}

const CDirentry& CDirectoryListing::operator[](unsigned int index) const
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	return m_pEntries[index];
}

CDirentry& CDirectoryListing::operator[](unsigned int index)
{
	// Commented out, too heavy speed penalty
	// wxASSERT(index < m_entryCount);
	return m_pEntries[index];
}
