#ifndef __TIMEEX_H__
#define __TIMEEX_H__

/* If called multiple times in a row, wxDateTime::Now may return the same 
 * time. This causes problems with the cache logic. This class implements
 * an extended time class in wich Now() never returns the same value.
 */

class CTimeEx
{
public:
	CTimeEx(wxDateTime time);
	CTimeEx();

	static CTimeEx Now();

	wxDateTime GetTime() { return m_time; }

	bool operator < (const CTimeEx& op) const; 
	bool operator <= (const CTimeEx& op) const;
	bool operator > (const CTimeEx& op) const; 
	bool operator >= (const CTimeEx& op) const;

protected:
	static wxDateTime m_lastTime;
	static int m_lastOffset;

	wxDateTime m_time;
	int m_offset;
};

#endif //__TIMEEX_H__
