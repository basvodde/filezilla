#include "FileZilla.h"
#include "serverpath.h"

#define FTP_MVS_DOUBLE_QUOTE (wxChar)0xDC

struct CServerTypeTraits
{
	wxChar separator;
	bool has_root; // Root = simply separator nothing else
	wxChar left_enclosure; // Example: VMS paths: [FOO.BAR]
	wxChar right_enclosure;
	bool filename_inside_enclosure; // MVS
	int prefixmode; //0 = normal prefix, 1 = suffix
	wxChar separatorEscape;
};

static const CServerTypeTraits traits[SERVERTYPE_MAX] = {
	{ '/',  true,  0,    0,    false, 0, 0   }, // Failsafe
	{ '/',  true,  0,    0,    false, 0, 0   },
	{ '.',  false, '[',  ']',  false, 0, '^' },
	{ '\\', false, 0,    0,    false, 0, 0   },
	{ '.',  false, '\'', '\'', true,  1, 0   },
	{ '/',  true,  0,    0,    false, 0, 0   },
	{ '/',  true,  0,    0,    false, 0, 0   }, // Same as Unix
	{ '.',  false, 0,    0,    false, 0, 0   },
	{ '\\', true,  0,    0,    false, 0, 0   }
};

CServerPath::CServerPath()
{
	m_type = DEFAULT;
	m_bEmpty = true;
}

CServerPath::CServerPath(const CServerPath &path, wxString subdir /*=_T("")*/)
{
	m_type = path.m_type;
	m_bEmpty = path.m_bEmpty;
	m_prefix = path.m_prefix;
	m_Segments = path.m_Segments;

	subdir.Trim(true);
	subdir.Trim(false);

	if (subdir == _T(""))
		return;

	if (!ChangePath(subdir))
		Clear();
}

CServerPath::CServerPath(wxString path, ServerType type /*=DEFAULT*/)
{
	m_type = type;

	SetPath(path);
}

CServerPath::~CServerPath()
{
}

void CServerPath::Clear()
{
	m_bEmpty = true;
	m_type = DEFAULT;
	m_prefix = _T("");
	m_Segments.clear();
}

bool CServerPath::SetPath(wxString newPath)
{
	return SetPath(newPath, false);
}

bool CServerPath::SetPath(wxString &newPath, bool isFile)
{
	m_Segments.clear();
	m_prefix = _T("");

	wxString path = newPath;
	wxString file;

	path.Trim(true);
	path.Trim(false);

	if (path == _T(""))
	{
		m_bEmpty = true;
		return false;
	}
	else
		m_bEmpty = false;

	if (m_type == DEFAULT)
	{
		int pos1 = path.Find(_T(":["));
		if (pos1 != -1)
		{
			int pos2 = path.Find(']', true);
			if (pos2 != -1 && static_cast<size_t>(pos2) == (path.Length() - 1) && !isFile)
				m_type = VMS;
			else if (isFile && pos2 > pos1)
				m_type = VMS;
		}
		else if (path.Length() >= 3 &&
			((path.c_str()[0] >= 'A' && path.c_str()[0] <= 'Z') || (path.c_str()[0] >= 'a' && path.c_str()[0] <= 'z')) &&
			path.c_str()[1] == ':' && (path.c_str()[2] == '\\' || path.c_str()[2] == '/'))
				m_type = DOS;
		else if (path.c_str()[0] == FTP_MVS_DOUBLE_QUOTE && path.Last() == FTP_MVS_DOUBLE_QUOTE)
			m_type = MVS;
		else if (path[0] == ':' && (pos1 = path.Mid(1).Find(':')) > 0)
		{
			int slash = path.Find('/');
			if (slash == -1 || slash > pos1)
				m_type = VXWORKS;
		}
		else if (path[0] == '\\')
			m_type = DOS_VIRTUAL;

		if (m_type == DEFAULT)
			m_type = UNIX;
	}

	int pos;
	switch (m_type)
	{
	case VMS:
		if (isFile)
		{
			pos = path.Find(']', true);
			if (pos == -1)
			{
				m_bEmpty = true;
				return false;
			}
			file = path.Mid(pos + 1);
			path = path.Left(pos + 1);
		}
		pos = path.Find(_T("["));
		if (pos == -1 || path.Right(1) != _T("]"))
		{
			m_bEmpty = true;
			return false;
		}
		else
		{
			path.RemoveLast();
			if (pos)
				m_prefix = path.Left(pos);
			path = path.Mid(pos + 1);
			pos = path.Find(_T("."));
			bool append = false;
			while (pos != -1)
			{
				bool wasAppend = append;
				wxString segment = path.Left(pos);
				if (segment.Last() == '^')
				{
					append = true;
					segment.RemoveLast();
					segment += _T(".");
				}
				else
					append = false;

				if (wasAppend)
					m_Segments.back() += segment;
				else
					m_Segments.push_back(segment);

				path = path.Mid(pos + 1);
				pos = path.Find(_T("."));
			}
			if (path != _T(""))
			{
				if (append)
					m_Segments.back() += path;
				else
					m_Segments.push_back(path);
			}
		}
		break;
	case MVS:
		{
			int i = 0;
			wxChar c = path.c_str()[i];
			while (c == FTP_MVS_DOUBLE_QUOTE || c == '\'' || c == '.')
				c = path.c_str()[++i];
			path.Remove(0, i);

			while (path != _T(""))
			{
				c = path.Last();
				if (c != FTP_MVS_DOUBLE_QUOTE && c != '\'')
					break;
				else
					path.RemoveLast();
			}

			while (path.Replace(_T(".."), _T(".")));

			int pos = path.Find(_T("."));
			while (pos != -1)
			{
				// What happens if pos is 0?
				m_Segments.push_back(path.Left(pos));
				path = path.Mid(pos + 1);
				pos = path.Find( _T(".") );
			}
			if (path != _T(""))
				m_Segments.push_back(path);
			else
				m_prefix = _T(".");

			if (isFile)
			{
				if (m_prefix == _T("."))
					return false;

				if (m_Segments.empty())
					return false;
				file = m_Segments.back();
				m_Segments.pop_back();

				int pos = file.Find('(');
				int pos2 = file.Find(')');
				if (pos != -1)
				{
					if (!pos || pos2 <= pos || pos2 != (int)file.Length() - 2)
						return false;
					m_Segments.push_back(file.Left(pos));
					file = file.Mid(pos + 1, pos2 - pos - 1);
					m_prefix = _T("");
				}
				else if (pos2 != -1)
					return false;
				else
					m_prefix = _T(".");
			}
		}
		break;
	case VXWORKS:
		{
			int colon2;
			if (path[0] != ':' || (colon2 = path.Mid(1).Find(':')) < 1)
			{
				m_bEmpty = true;
				return false;
			}
			int slash = path.Find('/');
			if (slash != -1 && slash <= colon2)
			{
				m_bEmpty = true;
				return false;
			}

			m_prefix = path.Left(colon2 + 2);
			path = path.Mid(colon2 + 2);
			goto set_path_default;
		}
		break;
	case HPNONSTOP:
		// Paths of the form \mysys.$myvol.mysubvol
		// Files of the form \mysys.$myvol.mysubvol.file
		if (path[0] != '\\')
		{
			m_bEmpty = true;
			return false;
		}
		while (true)
		{
			int pos = path.Find('.');
			if (!pos || pos == (int)path.Len() - 1)
			{
				m_bEmpty = true;
				return false;
			}
			if (pos == -1)
			{
				if (isFile)
				{
					if (m_Segments.empty())
					{
						m_bEmpty = true;
						return false;
					}
					newPath = path;
					return true;
				}
				m_Segments.push_back(path);
				return true;
			}

			m_Segments.push_back(path.Left(pos));
			path = path.Mid(pos + 1);
		}
		break;
	case DOS_VIRTUAL:
		path.Replace(_T("\\"), _T("/"));
		goto set_path_default;
		break;
	case DOS:
		// Check for starting drive letter
		path.Replace(_T("\\"), _T("/"));
		pos = path.Find('/');
		if (pos != 2 || path.c_str()[1] != ':' ||
			!((path.c_str()[0] >= 'A' && path.c_str()[0] <= 'Z') || (path.c_str()[0] >= 'a' && path.c_str()[0] <= 'z')))
		{
			m_bEmpty = true;
			return false;
		}
		// No break on purpose!
	default:
set_path_default:
		while (path.Replace(_T("//"), _T("/")));

		if (path.c_str()[0] == '/')
			path.Remove(0, 1);

		if (isFile)
		{
			pos = path.Find('/', true);
			if (pos == -1 || static_cast<size_t>(pos) == (path.Length() - 1))
			{
				m_bEmpty = true;
				return false;
			}
			file = path.Mid(pos + 1);
			path = path.Left(pos + 1);

			if (file == _T(".") || file == _T(".."))
			{
				m_bEmpty = true;
				return false;
			}
		}
		else if (path != _T("") && path.Right(1) != _T("/"))
			path = path + _T("/");

		pos = path.Find(_T("/"));
		while (pos != -1)
		{
			wxString segment = path.Left(pos);
			if (segment == _T(".."))
			{
				if (m_Segments.empty())
				{
					m_bEmpty = true;
					return false;
				}
				m_Segments.pop_back();
			}
			else if (segment != _T("."))
				m_Segments.push_back(segment);
			path = path.Mid(pos + 1);
			pos = path.Find(_T("/"));
		}

		break;
	}

	if (isFile)
		newPath = file;
	return true;
}

wxString CServerPath::GetPath() const
{
	if (m_bEmpty)
		return _T("");

	wxString path;

	if (!traits[m_type].prefixmode)
		path = m_prefix;

	if (traits[m_type].left_enclosure != 0)
		path += traits[m_type].left_enclosure;
	if (m_Segments.empty() && (!traits[m_type].has_root || m_prefix.empty()))
		path += traits[m_type].separator;

	for (tConstSegmentIter iter = m_Segments.begin(); iter != m_Segments.end(); iter++)
	{
		const wxString& segment = *iter;
		if ((traits[m_type].has_root && m_prefix.empty()) || iter != m_Segments.begin())
			path += traits[m_type].separator;

		if (traits[m_type].separatorEscape)
		{
			wxString tmp = segment;
			tmp.Replace((wxString)traits[m_type].separator, (wxString)traits[m_type].separatorEscape + traits[m_type].separator);
			path += tmp;
		}
		else
			path += segment;
	}

	if (traits[m_type].prefixmode)
		path += m_prefix;

	if (traits[m_type].right_enclosure != 0)
		path += traits[m_type].right_enclosure;

	// DOS is strange.
	// C: is current working dir on drive C, C:\ the drive root.
	if (m_type == DOS && m_Segments.size() == 1)
		path += traits[m_type].separator;

	return path;
}

bool CServerPath::HasParent() const
{
	if (m_bEmpty)
		return false;

	if (!traits[m_type].has_root)
		return m_Segments.size() > 1;

	return !m_Segments.empty();
}

CServerPath CServerPath::GetParent() const
{
	if (!HasParent())
		return CServerPath();

	CServerPath parent = *this;
	parent.m_Segments.pop_back();

	if (m_type == MVS)
		parent.m_prefix = _T(".");

	return parent;
}

wxString CServerPath::GetLastSegment() const
{
	if (!HasParent())
		return _T("");

	if (!m_Segments.empty())
		return m_Segments.back();
	else
		return _T("");
}

wxString CServerPath::GetSafePath() const
{
	if (m_bEmpty)
		return _T("");

	wxString safepath;
	safepath.Printf(_T("%d %d "), m_type, m_prefix.Length());
	if (m_prefix != _T(""))
		safepath += m_prefix + _T(" ");

	for (tConstSegmentIter iter = m_Segments.begin(); iter != m_Segments.end(); iter++)
		safepath += wxString::Format(_T("%d %s "), iter->Length(), iter->c_str());

	if (!m_Segments.empty())
		safepath.RemoveLast();

	return safepath;
}

bool CServerPath::SetSafePath(wxString path)
{
	m_bEmpty = true;
	m_prefix = _T("");
	m_Segments.clear();

	int pos = path.Find(' ');
	if (pos < 1)
		return false;

	long type;
	if (!path.Left(pos).ToLong(&type))
		return false;
	m_type = (ServerType)type;
	path = path.Mid(pos + 1);

	pos = path.Find(' ');
	if (pos == -1)
	{
		if (path != _T("0"))
			return false;
		else
		{
			// Is root folder, like / on unix like systems.
			m_bEmpty = false;
			return true;
		}
	}
	if (pos < 1)
		return false;

	unsigned long len;
	if (!path.Left(pos).ToULong(&len))
		return false;
	path = path.Mid(pos + 1);
	if (path.Length() < len)
		return false;

	if (len)
	{
		m_prefix = path.Left(len);
		path = path.Mid(len + 1);
	}

	while (path != _T(""))
	{
		pos = path.Find(' ');
		if (pos == -1)
			return false;
		if (!pos)
		{
			path = path.Mid(1);
			continue;
		}

		if (!path.Left(pos).ToULong(&len) || !len)
			return false;
		path = path.Mid(pos + 1);
		if (path.Length() < len)
			return false;

		m_Segments.push_back(path.Left(len));
		path = path.Mid(len + 1);
	}

	m_bEmpty = false;

	return true;
}

bool CServerPath::SetType(enum ServerType type)
{
	if (!m_bEmpty && m_type != DEFAULT)
		return false;

	m_type = type;

	return true;
}

enum ServerType CServerPath::GetType() const
{
	return m_type;
}

bool CServerPath::IsSubdirOf(const CServerPath &path, bool cmpNoCase) const
{
	if (m_bEmpty || path.m_bEmpty)
		return false;

	if (m_type != path.m_type)
		return false;

	if (!HasParent())
		return false;

	if (traits[m_type].prefixmode != 1)
	{
		if (cmpNoCase && m_prefix.CmpNoCase(path.m_prefix))
			return false;
		if (!cmpNoCase && m_prefix != path.m_prefix)
			return false;
	}

	// On MVS, dirs like 'FOO.BAR' without trailing dot cannot have
	// subdirectories
	if (traits[m_type].prefixmode == 1 && path.m_prefix == _T(""))
		return false;

	tConstSegmentIter iter1 = m_Segments.begin();
	tConstSegmentIter iter2 = path.m_Segments.begin();
	while (iter1 != m_Segments.end())
	{
		if (iter2 == path.m_Segments.end())
			return true;
		if (cmpNoCase)
		{
			if (iter1->CmpNoCase(*iter2))
				return false;
		}
		else if (*iter1 != *iter2)
			return false;

		iter1++;
		iter2++;
	}

	return false;
}

bool CServerPath::IsParentOf(const CServerPath &path, bool cmpNoCase) const
{
	if (!this)
		return false;

	return path.IsSubdirOf(*this, cmpNoCase);
}

bool CServerPath::ChangePath(wxString subdir)
{
	wxString subdir2 = subdir;
	return ChangePath(subdir2, false);
}

bool CServerPath::ChangePath(wxString &subdir, bool isFile)
{
	wxString dir = subdir;
	wxString file;

	dir.Trim(true);
	dir.Trim(false);

	if (dir == _T(""))
	{
		if (IsEmpty() || isFile)
			return false;
		else
			return true;
	}

	switch (m_type)
	{
	case VMS:
		{
			int pos1 = subdir.Find(_T("["));
			if (pos1 == -1)
			{
				int pos2 = dir.Find(']', true);
				if (pos2 != -1)
					return false;

				if (isFile)
				{
					if (IsEmpty())
						return false;

					subdir = dir;
					return true;
				}

				while (dir.Replace(_T(".."), _T(".")));
			}
			else
			{
				int pos2 = dir.Find(']', true);
				if (pos2 == -1)
					return false;

				if (isFile && static_cast<size_t>(pos2) == (dir.Length() - 1))
					return false;
				if (isFile && static_cast<size_t>(pos2) != (dir.Length() - 1))
					return false;
				if (pos2 <= pos1)
					return false;

				if (isFile)
					file = dir.Mid(pos2 + 1);
				dir = dir.Left(pos2);

				if (pos1)
					m_prefix = dir.Left(pos1);
				else
					m_prefix = _T("");
				dir = dir.Mid(pos1 + 1);

				m_Segments.clear();
			}
			int pos = dir.Find(_T("."));
			bool append = false;
			while (pos != -1)
			{
				bool wasAppend = append;
				wxString segment = dir.Left(pos);
				if (segment.Last() == '^')
				{
					append = true;
					segment.RemoveLast();
					segment += _T(".");
				}
				else
					append = false;

				if (wasAppend)
					m_Segments.back() += segment;
				else
					m_Segments.push_back(segment);

				dir = dir.Mid(pos + 1);
				pos = dir.Find(_T("."));
			}
			if (dir != _T(""))
			{
				if (append)
					m_Segments.back() += dir;
				else
					m_Segments.push_back(dir);
			}
		}
		break;
	case DOS:
		{
			dir.Replace(_T("\\"), _T("/"));
			while (dir.Replace(_T("//"), _T("/")));
			if (dir.Length() >= 2 && dir.c_str()[1] == ':')
				m_Segments.clear();
			else if (dir.Left(1) == _T("/"))
			{
				if (m_Segments.empty())
				{
					Clear();
					return false;
				}
				wxString first = m_Segments.front();
				m_Segments.clear();
				m_Segments.push_back(first);
				dir = dir.Mid(1);
			}

			if (isFile)
			{
				int pos = dir.Find('/', true);
				if (pos == (int)dir.Length() - 1)
				{
					Clear();
					return false;
				}
				if (pos == -1)
				{
					subdir = dir;
					return true;
				}
				else
				{
					file = dir.Mid(pos + 1);
					dir = dir.Left(pos + 1);
				}
			}
			else if (dir != _T("") && dir.Right(1) != _T("/"))
				dir += _T("/");

			int pos = dir.Find(_T("/"));
			while (pos != -1)
			{
				wxString segment = dir.Left(pos);
				if (segment == _T(".."))
				{
					if (m_Segments.size() <= 1)
					{
						Clear();
						return false;
					}
					m_Segments.pop_back();
				}
				else if (segment != _T("."))
					m_Segments.push_back(segment);
				dir = dir.Mid(pos + 1);
				pos = dir.Find(_T("/"));
			}
		}
		break;
	case MVS:
		{
			int i = 0;
			wxChar c = subdir.c_str()[i];
			while (c == FTP_MVS_DOUBLE_QUOTE)
				c = subdir.c_str()[++i];
			subdir.Remove(0, i);

			while (subdir != _T(""))
			{
				c = subdir.Last();
				if (c != FTP_MVS_DOUBLE_QUOTE)
					break;
				else
					subdir.RemoveLast();
			}
		}
		if (subdir == _T(""))
			return false;

		while (subdir.Replace(_T(".."), _T(".")));

		if (subdir.c_str()[0] == '\'')
		{
			if (subdir.Last() != '\'')
				return false;

			if (SetPath(subdir, isFile))
				file = subdir;
			else
				return false;
		}
		else if (subdir.Last() == '\'')
			return false;
		else if (!IsEmpty())
		{
			if (m_prefix != _T("."))
				return false;

			if (subdir.c_str()[0] == '.')
				subdir.Remove(0, 1);

			int pos = subdir.Find('.');
			while (pos != -1)
			{
				m_Segments.push_back(subdir.Left(pos));
				subdir = subdir.Mid(pos + 1);
			}
			if (subdir != _T(""))
			{
				m_Segments.push_back(subdir);
				m_prefix = _T("");
			}
			else
				m_prefix = _T(".");

			if (isFile)
			{
				if (m_prefix == _T("."))
					return false;

				if (m_Segments.empty())
					return false;
				file = m_Segments.back();
				m_Segments.pop_back();

				int pos = file.Find('(');
				int pos2 = file.Find(')');
				if (pos != -1)
				{
					if (!pos || pos2 <= pos || pos2 != (int)file.Length() - 2)
						return false;
					m_Segments.push_back(file.Left(pos));
					file = file.Mid(pos + 1, pos2 - pos - 1);
				}
				else if (pos2 != -1)
					return false;
				else
					m_prefix = _T(".");
			}
		}
		else if (SetPath(subdir, isFile))
			file = subdir;
		else
			return false;
		break;
	case HPNONSTOP:
		if (dir[0] == '\\')
			m_Segments.clear();
		while (true)
		{
			int pos = dir.Find('.');
			if (!pos || pos == (int)dir.Len() - 1)
			{
				m_bEmpty = true;
				return false;
			}
			if (pos == -1)
			{
				if (isFile)
				{
					if (m_Segments.empty())
					{
						m_bEmpty = true;
						return false;
					}
					subdir = dir;
					return true;
				}
				m_Segments.push_back(dir);
				return true;
			}

			m_Segments.push_back(dir.Left(pos));
			dir = dir.Mid(pos + 1);
		}
		break;
	case VXWORKS:
		{
			if (dir[0] != ':')
			{
				if (IsEmpty())
					return false;
			}
			else
			{
				int colon2;
				if ((colon2 = dir.Mid(1).Find(':')) < 1)
					return false;

				int slash = dir.Find('/');
				if (slash != -1 && slash <= colon2)
					return false;

				m_prefix = dir.Left(colon2 + 2);
				dir = dir.Mid(colon2 + 1);

				if (dir[0] == '/')
					return false;

				m_Segments.clear();

			}
			if (isFile && dir.Find('/') == -1)
				return false;
		}
		// No break on purpose
	default:
		{
			while (dir.Replace(_T("//"), _T("/")));
			if (dir.c_str()[0] == '/')
			{
				m_prefix = _T("");
				m_Segments.clear();
				dir = dir.Mid(1);
			}

			if (isFile)
			{
				int pos = dir.Find('/', true);
				if (pos == (int)dir.Length() - 1)
				{
					Clear();
					return false;
				}
				if (pos == -1)
				{
					subdir = dir;
					return true;
				}
				else
				{
					file = dir.Mid(pos + 1);
					dir = dir.Left(pos + 1);
				}
			}
			else if (dir != _T("") && dir.Right(1) != _T("/"))
				dir += _T("/");

			int pos = dir.Find(_T("/"));
			while (pos != -1)
			{
				wxString segment = dir.Left(pos);
				if (segment == _T(".."))
				{
					if (m_Segments.empty())
					{
						Clear();
						return false;
					}
					m_Segments.pop_back();
				}
				else if (segment != _T("."))
					m_Segments.push_back(segment);
				dir = dir.Mid(pos + 1);
				pos = dir.Find(_T("/"));
			}
		}
		break;
	}

	if (isFile)
		subdir = file;

	m_bEmpty = false;
	return true;

}

bool CServerPath::operator==(const CServerPath &op) const
{
	if (m_bEmpty != op.m_bEmpty)
		return false;
	else if (m_prefix != op.m_prefix)
		return false;
	else if (m_type != op.m_type)
		return false;
	else if (m_Segments != op.m_Segments)
		return false;

	return true;
}

bool CServerPath::operator!=(const CServerPath &op) const
{
	if (!this)
		return false;

	return !(*this == op);
}

bool CServerPath::operator<(const CServerPath &op) const
{
	if (m_bEmpty)
	{
		if (!op.m_bEmpty)
			return false;
	}
	else if (op.m_bEmpty)
		return true;

	const int cmp = m_prefix.Cmp(op.m_prefix);
	if (cmp < 0)
		return true;
	if (cmp > 0)
		return false;

	if (m_type > op.m_type)
		return false;
	else if (m_type < op.m_type)
		return true;

	std::list<wxString>::const_iterator iter1, iter2;
	for (iter1 = m_Segments.begin(), iter2 = op.m_Segments.begin(); iter1 != m_Segments.end(); iter1++, iter2++)
	{
		if (iter2 == op.m_Segments.end())
			return false;

		const int cmp = iter1->Cmp(*iter2);
		if (cmp < 0)
			return true;
		if (cmp > 0)
			return false;
	}

	return iter2 != op.m_Segments.end();
}

wxString CServerPath::FormatFilename(const wxString &filename, bool omitPath /*=false*/) const
{
	if (m_bEmpty)
		return filename;

	if (filename == _T(""))
		return _T("");

	if (m_bEmpty)
		return _T("");

	if (omitPath && (!traits[m_type].prefixmode || m_prefix == _T(".")))
		return filename;

	wxString result = GetPath();
	if (traits[m_type].left_enclosure && traits[m_type].filename_inside_enclosure)
		result.RemoveLast();

	switch (m_type)
	{
		case VXWORKS:
			if (result.Last() != traits[m_type].separator && !m_Segments.empty())
				result += traits[m_type].separator;
			break;
		case VMS:
		case MVS:
			break;
		default:
			if (result.Last() != traits[m_type].separator)
				result += traits[m_type].separator;
			break;
	}

	if (traits[m_type].prefixmode == 1 && m_prefix.empty())
		result += _T("(") + filename + _T(")");
	else
		result += filename;

	if (traits[m_type].left_enclosure && traits[m_type].filename_inside_enclosure)
		result += traits[m_type].right_enclosure;

	return result;
}

int CServerPath::CmpNoCase(const CServerPath &op) const
{
	if (m_bEmpty != op.m_bEmpty)
		return 1;
	else if (m_prefix != op.m_prefix)
		return 1;
	else if (m_type != op.m_type)
		return 1;

	if (m_Segments.size() > op.m_Segments.size())
		return 1;
	else if (m_Segments.size() < op.m_Segments.size())
		return -1;

	tConstSegmentIter iter = m_Segments.begin();
	tConstSegmentIter iter2 = op.m_Segments.begin();
	while (iter != m_Segments.end())
	{
		int res = iter++->CmpNoCase(*iter2++);
		if (res)
			return res;
	}

	return 0;
}

bool CServerPath::AddSegment(const wxString& segment)
{
	if (IsEmpty())
		return false;

	// TODO: Check for invalid characters
	m_Segments.push_back(segment);

	return true;
}

CServerPath CServerPath::GetCommonParent(const CServerPath& path) const
{
	if (*this == path)
		return *this;

	if (m_bEmpty || path.m_bEmpty)
		return CServerPath();

	if (m_type != path.m_type ||
		(!traits[m_type].prefixmode && m_prefix != path.m_prefix))
	{
		return CServerPath();
	}

	if (!HasParent())
	{
		if (path.IsSubdirOf(*this, false))
			return *this;
		else
			return CServerPath();
	}
	else if (!path.HasParent())
	{
		if (IsSubdirOf(path, false))
			return path;
		else
			return CServerPath();
	}

	CServerPath parent;
	parent.m_bEmpty = false;
	parent.m_type = m_type;
	parent.m_prefix = m_prefix;

	std::list<wxString>::const_iterator last = m_Segments.end();
	std::list<wxString>::const_iterator last2 = path.m_Segments.end();
	if (traits[m_type].prefixmode == 1)
	{
		if (m_prefix.empty())
			last--;
		if (path.m_prefix.empty())
			last2--;
		parent.m_prefix = GetParent().m_prefix;
	}

	std::list<wxString>::const_iterator iter = m_Segments.begin();
	std::list<wxString>::const_iterator iter2 = path.m_Segments.begin();
	while (iter != last && iter2 != last2)
	{
		if (*iter != *iter2)
		{
			if (!traits[m_type].has_root && parent.m_Segments.empty())
				return CServerPath();
			else
				return parent;
		}

		parent.m_Segments.push_back(*iter);

		iter++;
		iter2++;
	}

	return parent;
}

wxString CServerPath::FormatSubdir(const wxString &subdir) const
{
	if (!traits[m_type].separatorEscape)
		return subdir;

	wxString res = subdir;
	res.Replace((wxString)traits[m_type].separator, (wxString)traits[m_type].separatorEscape + traits[m_type].separator);

	return res;
}
