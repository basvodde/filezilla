#ifndef __SERVERPATH_H__
#define __SERVERPATH_H__

class CServerPath
{
public:
	CServerPath();
	CServerPath(wxString path, ServerType type = DEFAULT);
	CServerPath(const CServerPath &path, wxString subdir = _T("")); // Ignores parent on absolute subdir
	virtual ~CServerPath();

	bool IsEmpty() const { return m_bEmpty; };
	void Clear();

	bool SetPath(wxString newPath);
	bool SetPath(wxString &newPath, bool isFile);
	bool SetSafePath(wxString path);

	bool ChangePath(wxString subdir);
	bool ChangePath(wxString &subdir, bool isFile);

	wxString GetPath() const;
	wxString GetSafePath() const;

	bool HasParent() const;
	CServerPath GetParent() const;
	wxString GetLastSegment() const;

	bool SetType(ServerType type);

	bool IsSubdirOf(const CServerPath &path, bool cmpNoCase) const;
	bool IsParentOf(const CServerPath &path, bool cmpNoCase) const;

protected:
	bool m_bEmpty;
	ServerType m_type;
	wxString m_prefix;

	typedef std::list<wxString> tSegmentList;
	typedef tSegmentList::iterator tSegmentIter;
	typedef tSegmentList::const_iterator tConstSegmentIter;
	tSegmentList m_Segments;
};

#endif
