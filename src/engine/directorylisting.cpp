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

	return *this;

}

wxString CDirentry::dump() const
{
	wxString str = wxString::Format(_T("name=%s\nsize=%s\npermissions=%s\nownerGroup=%s\ndir=%d\nlink=%d\ntarget=%s\nhasDate=%d\nhasTime=%d\n"),
				name.c_str(), size.ToString().c_str(), permissions.c_str(), ownerGroup.c_str(), dir, link,
				target.c_str(), hasDate, hasTime, unsure);

	if (hasDate)
		str += wxString::Format(_T("date={%d, %d, %d}\n"), date.year, date.month, date.day);
	if (hasTime)
		str += wxString::Format(_T("time={%d, %d}\n"), time.hour, time.minute);
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
		if (date.day != op.date.day ||
			date.month != op.date.month ||
			date.year != op.date.year)
			return false;
	}

	if (hasTime)
	{
		if (time.hour != op.time.hour ||
			time.minute != op.time.minute)
			return false;
	}

	if (unsure != op.unsure)
		return false;

	return true;
}
