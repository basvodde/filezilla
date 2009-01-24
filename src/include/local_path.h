#ifndef __LOCAL_PATH_H__
#define __LOCAL_PATH_H__

// This class encapsulates local paths.
// On Windows it uses the C:\foo\bar syntax and also supports
// UNC paths.
// On all other systems it uses /foo/bar/baz

class CLocalPath
{
public:
	CLocalPath() { }
	CLocalPath(const CLocalPath &path);

	// Creates path. If the path is not syntactically
	// correct, empty() will return true.
	// If file is given and path not terminated by a separator,
	// the filename portion is returned in file.
	CLocalPath(const wxString& path, wxString* file = 0);
	bool SetPath(const wxString& path, wxString* file = 0);

	wxString GetPath() const { return m_path; }

	bool empty() const;
	void clear();
	
	// On failure the path s undefined
	bool ChangePath(const wxString& path);

	// Do not call with separators in the segment
	void AddSegment(const wxString& segment);

	// HasParent() and HasLogicalParent() only return different values on
	// MSW: C:\ is the drive root but has \ as logical parent, the drive list.
	bool HasParent() const;
	bool HasLogicalParent() const;

	CLocalPath GetParent(wxString* last_segment = 0) const;

	// If it fails, the path is undefined
	bool MakeParent(wxString* last_segment = 0);

	/* Calling GetLastSegment() only returns non-empty string if
	 * HasParent() returns true
	 */
	wxString GetLastSegment() const;

	bool IsSubdirOf(const CLocalPath &path) const;
	bool IsParentOf(const CLocalPath &path) const;

	/* Checks if the directory is writeable purely on a syntactical level.
	 * Currently only works on MSW where some logical paths
	 * are not writeable, e.g. the drive list \ or a remote computer \\foo
	 */
	bool IsWriteable() const;

	// Checks if the directory exists.
	bool Exists(wxString *error = 0) const;

	static const wxChar path_separator;

	bool operator==(const CLocalPath& op) const;
	bool operator!=(const CLocalPath& op) const;
protected:

	wxString m_path;
};

#endif //__LOCAL_PATH_H__
