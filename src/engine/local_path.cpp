#include "FileZilla.h"
#include "local_path.h"

#ifdef __WXMSW__
const wxChar CLocalPath::path_separator = '\\';
#else
const wxChar CLocalPath::path_separator = '/';
#endif

CLocalPath::CLocalPath(const wxString& path)
{
	SetPath(path);
}

CLocalPath::CLocalPath(const CLocalPath &path)
	: m_path(path.m_path)
{
}

bool CLocalPath::SetPath(const wxString& path)
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
		*out++ = path_separator;

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

CLocalPath CLocalPath::GetParent() const
{
	CLocalPath parent;

#ifdef __WXMSW__
	if (m_path.Len() == 3 && m_path[0] != '\\') // Drive root
		return CLocalPath(_T("\\"));

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
			return CLocalPath(m_path.Left(i + 1));
	}

	return CLocalPath();
}

bool CLocalPath::MakeParent()
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

	m_path += segment;
	m_path += path_separator;
}
