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

	bool hasDate;
	bool hasTime;

	struct t_date
	{
		int year;
		int month;
		int day;
	} date;

	struct t_time
	{
		int hour;
		int minute;
	} time;
};

class CDirectoryListing
{
public:
	int m_entryCount;

	CDirentry *m_pEntries;
};

#endif