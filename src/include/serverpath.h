#ifndef __SERVERPATH_H__
#define __SERVERPATH_H__

#include "refcount.h"

class CServerPathData
{
public:
	CServerPathData();
	CServerPathData(const CServerPathData& v);
	std::list<wxString> m_segments;
	wxString m_prefix;

	bool operator==(const CServerPathData& cmp) const;
};

class CServerPath
{
public:
	CServerPath();
	CServerPath(wxString path, ServerType type = DEFAULT);
	CServerPath(const CServerPath &path, wxString subdir); // Ignores parent on absolute subdir
	CServerPath(const CServerPath &path);
	virtual ~CServerPath();

	bool IsEmpty() const { return m_bEmpty; };
	void Clear();

	bool SetPath(wxString newPath);
	bool SetPath(wxString &newPath, bool isFile);
	bool SetSafePath(wxString path);

	// If ChangePath returns false, the object will be left an
	// undefined state
	bool ChangePath(wxString subdir);
	bool ChangePath(wxString &subdir, bool isFile);

	wxString GetPath() const;
	wxString GetSafePath() const;

	bool HasParent() const;
	CServerPath GetParent() const;
	wxString GetLastSegment() const;

	CServerPath GetCommonParent(const CServerPath& path) const;

	bool SetType(enum ServerType type);
	enum ServerType GetType() const;

	bool IsSubdirOf(const CServerPath &path, bool cmpNoCase) const;
	bool IsParentOf(const CServerPath &path, bool cmpNoCase) const;

	bool operator==(const CServerPath &op) const;
	bool operator!=(const CServerPath &op) const;
	bool operator<(const CServerPath &op) const;

	int CmpNoCase(const CServerPath &op) const;

	// omitPath is just a hint. For example dataset member names on MVS servers
	// always use absolute filenames including the full path
	wxString FormatFilename(const wxString &filename, bool omitPath = false) const;

	// Returns identity on all but VMS. On VMS it esscapes dots
	wxString FormatSubdir(const wxString &subdir) const;

	bool AddSegment(const wxString& segment);

protected:
	bool m_bEmpty;
	ServerType m_type;

	typedef std::list<wxString> tSegmentList;
	typedef tSegmentList::iterator tSegmentIter;
	typedef tSegmentList::const_iterator tConstSegmentIter;

	bool Segmentize(wxString str, tSegmentList& segments);
	bool ExtractFile(wxString& dir, wxString& file);

	CRefcountObject<CServerPathData> m_data;
};

#endif
