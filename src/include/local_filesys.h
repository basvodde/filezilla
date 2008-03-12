#ifndef __LOCAL_FILESYS_H__
#define __LOCAL_FILESYS_H__

// This class adds an abstraction layer for the local filesystem.
class CLocalFileSystem
{
public:
	enum local_fileType
	{
		unknown = -1,
		file,
		dir,
		link,
		link_file = link,
		link_dir
	};

	// If called with a symlink, GetFileType stats the link, not 
	// the target.
	static enum local_fileType GetFileType(const wxString& path);

	// Follows symlinks and stats the target, sets isLink to true if path was
	// a link.
	static enum local_fileType GetFileInfo(const wxString& path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int* mode);

	// If parent window is given, display confirmation dialog
	// Returns falls iff there's an encoding error, e.g. program
	// started without UTF-8 locale.
	static bool RecursiveDelete(const wxString& path, wxWindow* parent);
	static bool RecursiveDelete(std::list<wxString> dirsToVisit, wxWindow* parent);
	
protected:
#ifdef __WXMSW__
	static bool ConvertFileTimeToWxDateTime(wxDateTime& time, const FILETIME &ft);
#endif
};

#endif //__LOCAL_FILESYS_H__
