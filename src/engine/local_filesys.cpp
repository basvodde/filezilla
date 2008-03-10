#include "FileZilla.h"
#include "local_filesys.h"

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
		wxString tmp = file;
		tmp.RemoveLast();
		return GetFileType(tmp);
	}

	wxStructStat buf;
	int result = wxLstat(file, &buf);
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
				wxRemoveFile(fullName);
		}
	}

	// Delete the now empty directories
	for (std::list<wxString>::const_iterator iter = dirsToDelete.begin(); iter != dirsToDelete.end(); iter++)
		wxRmdir(*iter);

	return !encodingError;
#endif
}
