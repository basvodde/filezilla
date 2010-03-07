#ifndef __LOCAL_FILESYS_H__
#define __LOCAL_FILESYS_H__

// This class adds an abstraction layer for the local filesystem.
// Although wxWidgets provides functions for this, they are in
// general too slow.
// This class offers exactly what's needed by FileZilla and
// exploits some platform-specific features.
class CLocalFileSystem
{
public:
	CLocalFileSystem();
	virtual ~CLocalFileSystem();
	enum local_fileType
	{
		unknown = -1,
		file,
		dir,
		link,
		link_file = link,
		link_dir
	};

	static const wxChar path_separator;

	// If called with a symlink, GetFileType stats the link, not 
	// the target.
	static enum local_fileType GetFileType(const wxString& path);

	// Follows symlinks and stats the target, sets isLink to true if path was
	// a link.
	static enum local_fileType GetFileInfo(const wxString& path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int* mode);

	// If parent window is given, display confirmation dialog
	// Returns false iff there's an encoding error, e.g. program
	// started without UTF-8 locale.
	static bool RecursiveDelete(const wxString& path, wxWindow* parent);
	static bool RecursiveDelete(std::list<wxString> dirsToVisit, wxWindow* parent);

	bool BeginFindFiles(wxString path, bool dirs_only);
	bool GetNextFile(wxString& name);
	bool GetNextFile(wxString& name, bool &isLink, bool &is_dir, wxLongLong* size, wxDateTime* modificationTime, int* mode);
	void EndFindFiles();
	
protected:
#ifdef __WXMSW__
	static bool ConvertFileTimeToWxDateTime(wxDateTime& time, const FILETIME &ft);
#endif

#ifndef __WXMSW__
	static enum local_fileType GetFileInfo(const char* path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int* mode);
	void AllocPathBuffer(const char* file);  // Ensures m_raw_path is large enough to hold path and filename
#endif

	// State for directory enumeration
	bool m_dirs_only;
#ifdef __WXMSW__
	WIN32_FIND_DATA m_find_data;
	HANDLE m_hFind;
	bool m_found;
#else
	char* m_raw_path;
	char* m_file_part; // Points into m_raw_path past the trailing slash of the path part
	int m_buffer_length;
	DIR* m_dir;
#endif
};

#endif //__LOCAL_FILESYS_H__
