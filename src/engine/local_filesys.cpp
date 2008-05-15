#include "FileZilla.h"
#include "local_filesys.h"

CLocalFileSystem::CLocalFileSystem()
{
	m_dirs_only = false;
	m_found = false;
#ifdef __WXMSW__
	m_hFind = INVALID_HANDLE_VALUE;
#endif
}

CLocalFileSystem::~CLocalFileSystem()
{
#ifdef __WXMSW__
	if (m_hFind != INVALID_HANDLE_VALUE)
		FindClose(m_hFind);
#endif
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
	if (path.Last() == wxFileName::GetPathSeparator() && path != wxFileName::GetPathSeparator())
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
		if (path.Last() == wxFileName::GetPathSeparator() && path != wxFileName::GetPathSeparator())
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

			const wxString& fullName = path + wxFileName::GetPathSeparator() + file;

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
	if (path.Last() == wxFileName::GetPathSeparator() && path != wxFileName::GetPathSeparator())
	{
		wxString tmp = path;
		tmp.RemoveLast();
		return GetFileInfo(tmp, isLink, size, modificationTime, mode);
	}

#ifdef __WXMSW__
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
	wxStructStat buf;
	int result = wxLstat(path, &buf);
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
		int result = wxStat(path, &buf);
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
#endif
}

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
	m_dirs_only = dirs_only;
#ifdef __WXMSW__
	if (m_hFind != INVALID_HANDLE_VALUE)
		FindClose(m_hFind);

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
	m_path = path;
	if (m_path.Last() != '/')
		m_path += '/';

	if (!m_find.Open(path) || !m_find.IsOpened())
	{
		m_found = false;
		return false;
	}

	int flags = wxDIR_DIRS | wxDIR_HIDDEN;
	if (!m_dirs_only)
		flags |= wxDIR_FILES;
	m_found = m_find.GetFirst(&m_file, wxEmptyString, flags);
	return m_found;
#endif
}

void CLocalFileSystem::EndFindFiles()
{
	m_found = false;
#ifdef __WXMSW__
	if (m_hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(m_hFind);
		m_hFind = INVALID_HANDLE_VALUE;
	}
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
	if (!m_found)
		return false;

	while (m_found)
	{
		name = m_file;
		m_found = m_find.GetNext(&m_file);

		if (name == _T(""))
			return true;

		if (m_dirs_only)
		{
			bool wasLink;
			if (GetFileInfo(m_path + name, wasLink, 0, 0, 0) != dir)
				continue;
		}

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
	if (!m_found)
		return false;

	while (m_found)
	{
		name = m_file;
		m_found = m_find.GetNext(&m_file);

		if (name == _T(""))
			return true;

		enum local_fileType type = GetFileInfo(m_path + name, isLink, size, modificationTime, mode);
		if (m_dirs_only && type != dir)
			continue;
		is_dir = type == dir;
		
		return true;
	}

	return false;
#endif
}

