#include "FileZilla.h"
#include "local_filesys.h"

#ifdef __WXMSW__
const wxChar CLocalFileSystem::path_separator = '\\';
#else
const wxChar CLocalFileSystem::path_separator = '/';
#endif

CLocalFileSystem::CLocalFileSystem()
{
	m_dirs_only = false;
#ifdef __WXMSW__
	m_found = false;
	m_hFind = INVALID_HANDLE_VALUE;
#else
	m_raw_path = 0;
	m_file_part = 0;
	m_dir = 0;
#endif
}

CLocalFileSystem::~CLocalFileSystem()
{
	EndFindFiles();
}

enum CLocalFileSystem::local_fileType CLocalFileSystem::GetFileType(const wxString& path)
{
#ifdef __WXMSW__
	DWORD result = GetFileAttributes(path);
	if (result == INVALID_FILE_ATTRIBUTES)
		return unknown;

	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return dir;

	return file;
#else
	if (path.Last() == '/' && path != _T("/"))
	{
		wxString tmp = path;
		tmp.RemoveLast();
		return GetFileType(tmp);
	}

	wxStructStat buf;
	int result = wxLstat(path, &buf);
	if (result)
		return unknown;

#ifdef S_ISLNK
	if (S_ISLNK(buf.st_mode))
		return link;
#endif

	if (S_ISDIR(buf.st_mode))
		return dir;

	return file;
#endif
}

bool CLocalFileSystem::RecursiveDelete(const wxString& path, wxWindow* parent)
{
	std::list<wxString> paths;
	paths.push_back(path);
	return RecursiveDelete(paths, parent);
}

bool CLocalFileSystem::RecursiveDelete(std::list<wxString> dirsToVisit, wxWindow* parent)
{
	// Under Windows use SHFileOperation to delete files and directories.
	// Under other systems, we have to recurse into subdirectories manually
	// to delete all contents.

#ifdef __WXMSW__
	// SHFileOperation accepts a list of null-terminated strings. Go through all
	// paths to get the required buffer length

	;
	int len = 1; // String list terminated by empty string

	for (std::list<wxString>::const_iterator const_iter = dirsToVisit.begin(); const_iter != dirsToVisit.end(); const_iter++)
		len += const_iter->Length() + 1;

	// Allocate memory
	wxChar* pBuffer = new wxChar[len];
	wxChar* p = pBuffer;

	for (std::list<wxString>::iterator iter = dirsToVisit.begin(); iter != dirsToVisit.end(); iter++)
	{
		wxString& path = *iter;
		if (path.Last() == wxFileName::GetPathSeparator())
			path.RemoveLast();
		if (GetFileType(path) == unknown)
			continue;

		_tcscpy(p, path);
		p += path.Length() + 1;
	}
	if (p != pBuffer)
	{
		*p = 0;

		// Now we can delete the files in the buffer
		SHFILEOPSTRUCT op;
		memset(&op, 0, sizeof(op));
		op.hwnd = parent ? (HWND)parent->GetHandle() : 0;
		op.wFunc = FO_DELETE;
		op.pFrom = pBuffer;

		if (parent)
		{
			// Move to trash if shift is not pressed, else delete
			op.fFlags = wxGetKeyState(WXK_SHIFT) ? 0 : FOF_ALLOWUNDO;
		}
		else
			op.fFlags = FOF_NOCONFIRMATION;

		SHFileOperation(&op);
	}
	delete [] pBuffer;

	return true;
#else
	if (parent)
	{
		if (wxMessageBox(_("Really delete all selected files and/or directories?"), _("Confirmation needed"), wxICON_QUESTION | wxYES_NO, parent) != wxYES)
			return true;
	}

	for (std::list<wxString>::iterator iter = dirsToVisit.begin(); iter != dirsToVisit.end(); iter++)
	{
		wxString& path = *iter;
		if (path.Last() == '/' && path != _T("/"))
			path.RemoveLast();
	}

	bool encodingError = false;

	// Remember the directories to delete after recursing into them
	std::list<wxString> dirsToDelete;

	// Process all dirctories that have to be visited
	while (!dirsToVisit.empty())
	{
		const wxString path = dirsToVisit.front();
		dirsToVisit.pop_front();
		dirsToDelete.push_front(path);

		if (GetFileType(path) != dir)
		{
			wxRemoveFile(path);
			continue;
		}

		wxDir dir;
		if (!dir.Open(path))
			continue;

		// Depending on underlying platform, wxDir does not handle
		// changes to the directory contents very well.
		// See bug [ 1946574 ]
		// To work around this, delete files after enumerating everything in current directory
		std::list<wxString> filesToDelete;

		wxString file;
		for (bool found = dir.GetFirst(&file); found; found = dir.GetNext(&file))
		{
			if (file == _T(""))
			{
				encodingError = true;
				continue;
			}

			const wxString& fullName = path + _T("/") + file;

			if (CLocalFileSystem::GetFileType(fullName) == CLocalFileSystem::dir)
				dirsToVisit.push_back(fullName);
			else
				filesToDelete.push_back(fullName);
		}

		// Delete all files and links in current directory enumerated before
		for (std::list<wxString>::const_iterator iter = filesToDelete.begin(); iter != filesToDelete.end(); iter++)
			wxRemoveFile(*iter);
	}

	// Delete the now empty directories
	for (std::list<wxString>::const_iterator iter = dirsToDelete.begin(); iter != dirsToDelete.end(); iter++)
		wxRmdir(*iter);

	return !encodingError;
#endif
}

enum CLocalFileSystem::local_fileType CLocalFileSystem::GetFileInfo(const wxString& path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int *mode)
{
#ifdef __WXMSW__
	if (path.Last() == wxFileName::GetPathSeparator() && path != wxFileName::GetPathSeparator())
	{
		wxString tmp = path;
		tmp.RemoveLast();
		return GetFileInfo(tmp, isLink, size, modificationTime, mode);
	}

	isLink = false;

	WIN32_FILE_ATTRIBUTE_DATA attributes;
	BOOL result = GetFileAttributesEx(path, GetFileExInfoStandard, &attributes);
	if (!result)
	{
		if (size)
			*size = -1;
		if (mode)
			*mode = 0;
		if (modificationTime)
			*modificationTime = wxDateTime();
		return unknown;
	}

	if (modificationTime)
		ConvertFileTimeToWxDateTime(*modificationTime, attributes.ftLastWriteTime);

	if (mode)
		*mode = (int)attributes.dwFileAttributes;

	if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (size)
			*size = -1;
		return dir;
	}

	if (size)
		*size = wxLongLong(attributes.nFileSizeHigh, attributes.nFileSizeLow);

	return file;
#else
	if (path.Last() == '/' && path != _T("/"))
	{
		wxString tmp = path;
		tmp.RemoveLast();
		return GetFileInfo(tmp, isLink, size, modificationTime, mode);
	}

	const wxCharBuffer p = path.fn_str();
	return GetFileInfo((const char*)p, isLink, size, modificationTime, mode);
#endif
}

#ifndef __WXMSW__
enum CLocalFileSystem::local_fileType CLocalFileSystem::GetFileInfo(const char* path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int *mode)
{
	struct stat buf;
	int result = lstat(path, &buf);
	if (result)
	{
		isLink = false;
		if (size)
			*size = -1;
		if (mode)
			*mode = -1;
		if (modificationTime)
			*modificationTime = wxDateTime();
		return unknown;
	}

#ifdef S_ISLNK
	if (S_ISLNK(buf.st_mode))
	{
		isLink = true;
		int result = stat(path, &buf);
		if (result)
		{
			if (size)
				*size = -1;
			if (mode)
				*mode = -1;
			if (modificationTime)
				*modificationTime = wxDateTime();
			return unknown;
		}
	}
	else
#endif
		isLink = false;

	if (modificationTime)
		modificationTime->Set(buf.st_mtime);

	if (mode)
		*mode = buf.st_mode & 0x777;

	if (S_ISDIR(buf.st_mode))
	{
		if (size)
			*size = -1;
		return dir;
	}

	if (size)
		*size = buf.st_size;

	return file;
}
#endif

#ifdef __WXMSW__
bool CLocalFileSystem::ConvertFileTimeToWxDateTime(wxDateTime& time, const FILETIME &ft)
{
	if (!ft.dwHighDateTime && !ft.dwLowDateTime)
		return false;

	FILETIME ftLocal;
	if (!::FileTimeToLocalFileTime(&ft, &ftLocal))
		return false;

	SYSTEMTIME st;
	if (!::FileTimeToSystemTime(&ftLocal, &st))
		return false;

	wxDateTime tmp;
	if (!tmp.Set(st.wDay, wxDateTime::Month(st.wMonth - 1), st.wYear,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds).IsValid())
		return false;
	time = tmp;

	return true;
}
#endif

bool CLocalFileSystem::BeginFindFiles(wxString path, bool dirs_only)
{
	EndFindFiles();

	m_dirs_only = dirs_only;
#ifdef __WXMSW__
	if (path.Last() != '/' && path.Last() != '\\')
		path += _T("\\*");
	else
		path += '*';

	m_hFind = FindFirstFileEx(path, FindExInfoStandard, &m_find_data, dirs_only ? FindExSearchLimitToDirectories : FindExSearchNameMatch, 0, 0);
	if (m_hFind == INVALID_HANDLE_VALUE)
	{
		m_found = false;
		return false;
	}
	
	m_found = true;	
	return true;
#else
	if (path != _T("/") && path.Last() == '/')
		path.RemoveLast();

	const wxCharBuffer s = path.fn_str();

	m_dir = opendir(s);
	if (!m_dir)
		return false;

	const wxCharBuffer p = path.fn_str();
	const int len = strlen(p);
	m_raw_path = new char[len + 2048 + 2];
	m_buffer_length = len + 2048 + 2;
	strcpy(m_raw_path, p);
	if (len > 1)
	{
		m_raw_path[len] = '/';
		m_file_part = m_raw_path + len + 1;
	}
	else
		m_file_part = m_raw_path + len;

	return true;
#endif
}

void CLocalFileSystem::EndFindFiles()
{
#ifdef __WXMSW__
	m_found = false;
	if (m_hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(m_hFind);
		m_hFind = INVALID_HANDLE_VALUE;
	}
#else
	if (m_dir)
	{
		closedir(m_dir);
		m_dir = 0;
	}
	delete [] m_raw_path;
	m_raw_path = 0;
	m_file_part = 0;
#endif
}

bool CLocalFileSystem::GetNextFile(wxString& name)
{
#ifdef __WXMSW__
	if (!m_found)
		return false;
	do
	{
		name = m_find_data.cFileName;
		if (name == _T(""))
		{
			m_found = FindNextFile(m_hFind, &m_find_data) != 0;
			return true;
		}
		if (name == _T(".") || name == _T(".."))
			continue;

		if (m_dirs_only && !(m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		m_found = FindNextFile(m_hFind, &m_find_data) != 0;
		return true;
	} while ((m_found = FindNextFile(m_hFind, &m_find_data) != 0));
	
	return false;
#else
	if (!m_dir)
		return false;

	struct dirent* entry;
	while ((entry = readdir(m_dir)))
	{
		if (!entry->d_name[0] ||
			!strcmp(entry->d_name, ".") ||
			!strcmp(entry->d_name, ".."))
			continue;

		if (m_dirs_only)
		{
#ifdef _DIRENT_HAVE_D_TYPE
			if (entry->d_type == DT_LNK)
			{
				bool wasLink;
				AllocPathBuffer(entry->d_name);
				strcpy(m_file_part, entry->d_name);
				if (GetFileInfo(m_raw_path, wasLink, 0, 0, 0) != dir)
					continue;
			}
			else if (entry->d_type != DT_DIR)
				continue;
#else
			// Solaris doesn't have d_type
			bool wasLink;
			AllocPathBuffer(entry->d_name);
			strcpy(m_file_part, entry->d_name);
			if (GetFileInfo(m_raw_path, wasLink, 0, 0, 0) != dir)
				continue;
#endif
		}

		name = wxString(entry->d_name, *wxConvFileName);

		return true;
	}

	return false;
#endif
}

bool CLocalFileSystem::GetNextFile(wxString& name, bool &isLink, bool &is_dir, wxLongLong* size, wxDateTime* modificationTime, int* mode)
{
#ifdef __WXMSW__
	if (!m_found)
		return false;
	do
	{
		name = m_find_data.cFileName;
		if (name == _T(""))
		{
			m_found = FindNextFile(m_hFind, &m_find_data) != 0;
			return true;
		}
		if (name == _T(".") || name == _T(".."))
			continue;

		if (m_dirs_only && !(m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		isLink = false;

		if (modificationTime)
			ConvertFileTimeToWxDateTime(*modificationTime, m_find_data.ftLastWriteTime);

		if (mode)
			*mode = (int)m_find_data.dwFileAttributes;

		if (m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			is_dir = true;
			if (size)
				*size = -1;
		}
		else
		{
			is_dir = false;
			*size = wxLongLong(m_find_data.nFileSizeHigh, m_find_data.nFileSizeLow);
		}

		m_found = FindNextFile(m_hFind, &m_find_data) != 0;
		return true;
	} while ((m_found = FindNextFile(m_hFind, &m_find_data) != 0));
	
	return false;
#else
	if (!m_dir)
		return false;

	struct dirent* entry;
	while ((entry = readdir(m_dir)))
	{
		if (!entry->d_name[0] ||
			!strcmp(entry->d_name, ".") ||
			!strcmp(entry->d_name, ".."))
			continue;

#ifdef _DIRENT_HAVE_D_TYPE
		if (m_dirs_only)
		{
			if (entry->d_type == DT_LNK)
			{
				AllocPathBuffer(entry->d_name);
				strcpy(m_file_part, entry->d_name);
				enum local_fileType type = GetFileInfo(m_raw_path, isLink, size, modificationTime, mode);
				if (type != dir)
					continue;

				name = wxString(entry->d_name, *wxConvFileName);
				is_dir = true;
				return true;
			}
			else if (entry->d_type != DT_DIR)
				continue;
		}
#endif

		AllocPathBuffer(entry->d_name);
		strcpy(m_file_part, entry->d_name);
		enum local_fileType type = GetFileInfo(m_raw_path, isLink, size, modificationTime, mode);

#ifndef _DIRENT_HAVE_D_TYPE
		// Solaris doesn't have d_type
		if (m_dirs_only && type != dir)
			continue;
#endif
		is_dir = type == dir;
		
		name = wxString(entry->d_name, *wxConvFileName);

		return true;
	}

	return false;
#endif
}

#ifndef __WXMSW__
void CLocalFileSystem::AllocPathBuffer(const char* file)
{
	int len = strlen(file);
	int pathlen = m_file_part - m_raw_path;

	if (len + pathlen >= m_buffer_length)
	{
		m_buffer_length = (len + pathlen) * 2;
		char* tmp = new char[m_buffer_length];
		memcpy(tmp, m_raw_path, pathlen);
		delete [] m_raw_path;
		m_raw_path = tmp;
		m_file_part = m_raw_path + pathlen;
	}
}
#endif
