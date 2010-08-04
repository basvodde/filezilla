#include <filezilla.h>
#include "local_path.h"
#ifndef __WXMSW__
#include <errno.h>
#endif

#ifdef __WXMSW__
const wxChar CLocalPath::path_separator = '\\';
#else
const wxChar CLocalPath::path_separator = '/';
#endif

CLocalPath::CLocalPath(const wxString& path, wxString* file /*=0*/)
{
	SetPath(path, file);
}

CLocalPath::CLocalPath(const CLocalPath &path)
	: m_path(path.m_path)
{
}

bool CLocalPath::SetPath(const wxString& path, wxString* file /*=0*/)
{
	// This function ensures that the path is in canonical form on success.

	if (path.empty())
	{
		m_path.clear();
		return false;
	}

	std::list<wxChar*> segments; // List to store the beginnings of segments

	const wxChar* in = path.c_str();

#ifdef __WXMSW__
	if (path == _T("\\"))
	{
		m_path = _T("\\");
		if (file)
			file->clear();
		return true;
	}

	wxChar* out;
	if (*in == '\\')
	{
		// possibly UNC

		in++;
		if (*in++ != '\\')
		{
			m_path.clear();
			return false;
		}

		wxChar* buf = m_path.GetWriteBuf(path.Len() + 2);
		out = buf;
		*out++ = '\\';
		*out++ = '\\';

		// UNC path
		while (*in)
		{
			if (*in == '/' || *in == '\\')
				break;
			*out++ = *in++;
		}
		*out++ = path_separator;

		if (out - buf <= 3)
		{
			// not a valid UNC path
			*buf = 0;
			m_path.UngetWriteBuf();
			return false;
		}
		segments.push_back(out);
	}
	else if ((*in >= 'a' && *in <= 'z') || (*in >= 'A' || *in <= 'Z'))
	{
		// Regular path

		wxChar* buf = m_path.GetWriteBuf(path.Len() + 2);
		out = buf;
		*out++ = *in++;

		if (*in++ != ':')
		{
			*buf = 0;
			m_path.UngetWriteBuf();
			return false;
		}
		*out++ = ':';
		if (*in != '/' && *in != '\\' && *in)
		{
			*buf = 0;
			m_path.UngetWriteBuf();
			return false;
		}
		*out++ = path_separator;
		segments.push_back(out);
	}
	else
	{
		m_path.clear();
		return false;
	}
#else
	if (*in++ != '/')
	{
		// SetPath only accepts absolute paths
		m_path.clear();
		return false;
	}

	wxChar* out = m_path.GetWriteBuf(path.Len() + 2);
	*out++ = '/';
	segments.push_back(out);
#endif

	enum _last
	{
		separator,
		dot,
		dotdot,
		segment
	};
	enum _last last = separator;

	while (*in)
	{
		if (*in == '/'
#ifdef __WXMSW__
			|| *in == '\\'
#endif
			)
		{
			in++;
			if (last == separator)
			{
				// /foo//bar is equal to /foo/bar
				continue;
			}
			else if (last == dot)
			{
				// /foo/./bar is equal to /foo/bar
				last = separator;
				out = segments.back();
				continue;
			}
			else if (last == dotdot)
			{
				last = separator;

				// Go two segments back if possible
				if (segments.size() > 1)
					segments.pop_back();
				wxASSERT(!segments.empty());
				out = segments.back();
				continue;
			}

			// Ordinary segment just ended.
			*out++ = path_separator;
			segments.push_back(out);
			last = separator;
			continue;
		}
		else if (*in == '.')
		{
			if (last == separator)
				last = dot;
			else if (last == dot)
				last = dotdot;
			else if (last == dotdot)
				last = segment;
		}
		else
			last = segment;

		*out++ = *in++;
	}
	if (last == dot)
		out = segments.back();
	else if (last == dotdot)
	{
		if (segments.size() > 1)
			segments.pop_back();
		out = segments.back();
	}
	else if (last == segment)
	{
		if (file)
		{
			*out = 0;
			out = segments.back();
			*file = out;
		}
		else
			*out++ = path_separator;
	}

	*out = 0;

	m_path.UngetWriteBuf();
	return true;
}

bool CLocalPath::empty() const
{
	return m_path.empty();
}

void CLocalPath::clear()
{
	m_path.clear();
}

bool CLocalPath::IsWriteable() const
{
	if (m_path.empty())
		return false;

#ifdef __WXMSW__
	if (m_path == _T("\\"))
		// List of drives not writeable
		return false;

	if (m_path.Left(2) == _T("\\\\"))
	{
		int pos = m_path.Mid(2).Find('\\');
		if (pos == -1 || pos + 3 == (int)m_path.Len())
			// List of shares on a computer not writeable
			return false;
	}
#endif

	return true;
}

bool CLocalPath::HasParent() const
{
#ifdef __WXMSW__
	// C:\f\ has parent
	// C:\ does not
	// \\x\y\ shortest UNC
	//   ^ min
	const int min = 2;
#else
	const int min = 0;
#endif
	for (int i = (int)m_path.Len() - 2; i >= min; i--)
	{
		if (m_path[i] == path_separator)
			return true;
	}

	return false;
}

bool CLocalPath::HasLogicalParent() const
{
#ifdef __WXMSW__
	if (m_path.Len() == 3 && m_path[0] != '\\') // Drive root
		return true;
#endif
	return HasParent();
}

CLocalPath CLocalPath::GetParent(wxString* last_segment /*=0*/) const
{
	CLocalPath parent;

#ifdef __WXMSW__
	if (m_path.Len() == 3 && m_path[0] != '\\') // Drive root
	{
		if (last_segment)
			last_segment->clear();
		return CLocalPath(_T("\\"));
	}

	// C:\f\ has parent
	// C:\ does not
	// \\x\y\ shortest UNC
	//   ^ min
	const int min = 2;
#else
	const int min = 0;
#endif
	for (int i = (int)m_path.Len() - 2; i >= min; i--)
	{
		if (m_path[i] == path_separator)
		{
			if (last_segment)
			{
				*last_segment = m_path.Mid(i + 1);
				last_segment->RemoveLast();
			}
			return CLocalPath(m_path.Left(i + 1));
		}
	}

	return CLocalPath();
}

bool CLocalPath::MakeParent(wxString* last_segment /*=0*/)
{
#ifdef __WXMSW__
	if (m_path.Len() == 3 && m_path[0] != '\\') // Drive root
	{
		m_path = _T("\\");
		return true;
	}

	// C:\f\ has parent
	// C:\ does not
	// \\x\y\ shortest UNC
	//   ^ min
	const int min = 2;
#else
	const int min = 0;
#endif
	for (int i = (int)m_path.Len() - 2; i >= min; i--)
	{
		if (m_path[i] == path_separator)
		{
			if (last_segment)
			{
				*last_segment = m_path.Mid(i + 1);
				last_segment->RemoveLast();
			}
			m_path = m_path.Left(i + 1);
			return true;
		}
	}

	return false;
}

void CLocalPath::AddSegment(const wxString& segment)
{
	wxASSERT(!m_path.empty());
	wxASSERT(segment.Find(_T("/")) == -1);
#ifdef __WXMSW__
	wxASSERT(segment.Find(_T("\\")) == -1);
#endif

	if (!segment.empty())
	{
		m_path += segment;
		m_path += path_separator;
	}
}

bool CLocalPath::ChangePath(const wxString& path)
{
	if (path.empty())
		return false;
#ifdef __WXMSW__
	if (path == _T("\\") || path == _T("/"))
	{
		m_path = _T("\\");
		return true;
	}

	if (path[0] == '\\' && path[1] == '\\')
	{
		// Absolute UNC
		return SetPath(path);
	}
	if (path[0] && path[1] == ':')
	{
		// Absolute path
		return SetPath(path);
	}

	// Relative path
	if (m_path.empty())
		return false;

	return SetPath(m_path + path);
#else
	if (path[0] == path_separator)
	{
		// Absolute path
		return SetPath(path);
	}
	else
	{
		// Relative path

		if (m_path.empty())
			return false;

		return SetPath(m_path + path);
	}
#endif
}

bool CLocalPath::Exists(wxString *error /*=0*/) const
{
	wxASSERT(!m_path.empty());
	if (m_path.empty())
		return false;

#ifdef __WXMSW__
	if (m_path == _T("\\"))
	{
		// List of drives always exists
		return true;
	}

	if (m_path[0] == '\\')
	{
		// UNC path

		// \\server\share\ 

		size_t pos;

		// Search for backslash separating server from share
		for (pos = 3; pos < m_path.Len(); pos++)
			if (m_path[pos] == '\\')
				break;
		pos++;
		if (pos >= m_path.Len())
		{
			// Partial UNC path
			return true;
		}
	}

	wxString path = m_path;
	if (path.Len() > 3)
		path.RemoveLast();
	DWORD ret = ::GetFileAttributes(path);
	if (ret == INVALID_FILE_ATTRIBUTES)
	{
		if (!error)
			return false;

		error->Printf(_("'%s' does not exist or cannot be accessed."), path.c_str());

		if (m_path[0] == '\\')
			return false;

		// Check for removable drive, display a more specific error message in that case
		if (::GetLastError() != ERROR_NOT_READY)
			return false;
		int type = GetDriveType(m_path.Left(3));
		if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM)
			error->Printf(_("Cannot access '%s', no media inserted or drive not ready."), path.c_str());
		return false;
	}
	else if (!(ret & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (error)
			error->Printf(_("'%s' is not a directory."), path.c_str());
		return false;
	}

	return true;
#else
	wxString path = m_path;
	if (path.Len() > 1)
		path.RemoveLast();

	const wxCharBuffer s = path.fn_str();

	struct stat buf;
	int result = stat(s, &buf);

	if (!result)
	{
		if (S_ISDIR(buf.st_mode))
			return true;

		if (error)
			error->Printf(_("'%s' is not a directory."), path.c_str());

		return false;
	}
	else if (result == ENOTDIR)
	{
		if (error)
			error->Printf(_("'%s' is not a directory."), path.c_str());
		return false;
	}
	else
	{
		if (error)
			error->Printf(_("'%s' does not exist or cannot be accessed."), path.c_str());
		return false;
	}
#endif
}

bool CLocalPath::operator==(const CLocalPath& op) const
{
#ifdef __WXMSW__
	return m_path.CmpNoCase(op.m_path) == 0;
#else
	return m_path == op.m_path;
#endif
}

bool CLocalPath::operator!=(const CLocalPath& op) const
{
#ifdef __WXMSW__
	return m_path.CmpNoCase(op.m_path) != 0;
#else
	return m_path != op.m_path;
#endif
}

bool CLocalPath::IsParentOf(const CLocalPath &path) const
{
	if (empty() || path.empty())
		return false;

	if (path.m_path.Len() < m_path.Len())
		return false;

#ifdef __WXMSW__
	if (m_path.CmpNoCase(path.m_path.Left(m_path.Len())))
		return false;
#else
	if (m_path != path.m_path.Left(m_path.Len()))
		return false;
#endif

	return true;
}

bool CLocalPath::IsSubdirOf(const CLocalPath &path) const
{
	if (empty() || path.empty())
		return false;

	if (path.m_path.Len() > m_path.Len())
		return false;

#ifdef __WXMSW__
	if (path.m_path.CmpNoCase(m_path.Left(path.m_path.Len())))
		return false;
#else
	if (path.m_path != m_path.Left(path.m_path.Len()))
		return false;
#endif

	return true;
}

wxString CLocalPath::GetLastSegment() const
{
	wxASSERT(HasParent());

#ifdef __WXMSW__
	// C:\f\ has parent
	// C:\ does not
	// \\x\y\ shortest UNC
	//   ^ min
	const int min = 2;
#else
	const int min = 0;
#endif
	for (int i = (int)m_path.Len() - 2; i >= min; i--)
	{
		if (m_path[i] == path_separator)
		{
			wxString last = m_path.Mid(i + 1);
			last.RemoveLast();
			return last;
		}
	}

	return _T("");
}
