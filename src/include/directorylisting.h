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

	bool unsure; // May be true on cached items if any changes were made to the file
};

class CDirectoryListing
{
public:
	CDirectoryListing();
	~CDirectoryListing();

	unsigned int m_entryCount;

	CServerPath path;
	CDirentry *m_pEntries;
	CDirectoryListing& operator=(const CDirectoryListing &a);

	bool m_hasUnsureEntries;
};

#endif

