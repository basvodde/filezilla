#include "FileZilla.h"
#include "timeex.h"

wxDateTime CTimeEx::m_lastTime = wxDateTime::Now();
int CTimeEx::m_lastOffset = 0;

CTimeEx::CTimeEx()
{
	m_offset = 0;
}

CTimeEx::CTimeEx(wxDateTime time)
{
	m_time = time;
	m_offset = 0;
}

CTimeEx CTimeEx::Now()
{
	CTimeEx time;
	time.m_time = wxDateTime::UNow();
	if (time.m_time == m_lastTime)
		time.m_offset = ++m_lastOffset;
	else
	{
		m_lastTime = time.m_time;
		time.m_offset = m_lastOffset = 0;
	}
	return time;
}

bool CTimeEx::operator < (const CTimeEx& op) const
{
	if (m_time < op.m_time)
		return true;
	if (m_time > op.m_time)
		return false;
	
	return m_offset < op.m_offset;
}

bool CTimeEx::operator <= (const CTimeEx& op) const
{
	if (m_time < op.m_time)
		return true;
	if (m_time > op.m_time)
		return false;
	
	return m_offset <= op.m_offset;
}

bool CTimeEx::operator > (const CTimeEx& op) const
{
	if (m_time > op.m_time)
		return true;
	if (m_time < op.m_time)
		return false;
	
	return m_offset > op.m_offset;
}

bool CTimeEx::operator >= (const CTimeEx& op) const
{
	if (m_time > op.m_time)
		return true;
	if (m_time < op.m_time)
		return false;
	
	return m_offset >= op.m_offset;
}

bool CTimeEx::operator == (const CTimeEx& op) const
{
	if (m_time != op.m_time)
		return false;
	
	return m_offset == op.m_offset;
}
