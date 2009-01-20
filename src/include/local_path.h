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
	// Path will not be canonicalized
	CLocalPath(const wxString& path);
	bool SetPath(const wxString& path);

	wxString GetPath() const { return m_path; }

	bool empty() const;
	void clear();
	
	// On success, resulting path will be in canonical form.
	bool ChangePath(const wxString& path);

	// Do not call with separators in the segment
	void AddSegment(const wxString& segment);

	// HasParent() and HasLogicalParent() only return different values on
	// MSW: C:\ is the drive root but has \ as logical parent, the drive list.
	bool HasParent() const;
	bool HasLogicalParent() const;

	CLocalPath GetParent() const;

	// If it fails, the path is undefined
	bool MakeParent();

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

	static const wxChar path_separator;
protected:

	wxString m_path;
};

#endif //__LOCAL_PATH_H__
