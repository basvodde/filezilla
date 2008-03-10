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
		link
	};

	// If called with a symlink, these function stat the link, not 
	// the target.
	static enum local_fileType GetFileType(const wxString& path);

	// If parent window is given, display confirmation dialog
	// Returns falls iff there's an encoding error, e.g. program
	// started without UTF-8 locale.
	static bool RecursiveDelete(const wxString& path, wxWindow* parent);
	static bool RecursiveDelete(std::list<wxString> dirsToVisit, wxWindow* parent);
};

#endif //__LOCAL_FILESYS_H__
