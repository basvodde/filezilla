#include <filezilla.h>
#include "serverpath.h"

#define FTP_MVS_DOUBLE_QUOTE (wxChar)0xDC

struct CServerTypeTraits
{
	const wxChar* separators;
	bool has_root; // Root = simply separator nothing else
	wxChar left_enclosure; // Example: VMS paths: [FOO.BAR]
	wxChar right_enclosure;
	bool filename_inside_enclosure; // MVS
	int prefixmode; //0 = normal prefix, 1 = suffix
	wxChar separatorEscape;
	bool has_dots; // Special meaning for .. (parent) and . (self)
	bool separator_after_prefix;
};

static const CServerTypeTraits traits[SERVERTYPE_MAX] = {
	{ _T("/"),  true,  0,    0,    false, 0, 0,   true,  false }, // Failsafe
	{ _T("/"),  true,  0,    0,    false, 0, 0,   true,  false },
	{ _T("."),  false, '[',  ']',  false, 0, '^', false, false },
	{ _T("\\/"), false, 0,    0,    false, 0, 0,   true,  false },
	{ _T("."),  false, '\'', '\'', true,  1, 0,   false, false },
	{ _T("/"),  true,  0,    0,    false, 0, 0,   true,  false },
	{ _T("/"),  true,  0,    0,    false, 0, 0,   true,  false }, // Same as Unix
	{ _T("."),  false, 0,    0,    false, 0, 0,   false, false },
	{ _T("\\"), true,  0,    0,    false, 0, 0,   true,  false },
	{ _T("/"),  true,  0,    0,    false, 0, 0,   true,  true  } // Cygwin is like Unix but has optional prefix of form "//server"
};

CServerPathData::CServerPathData()
{
}

CServerPathData::CServerPathData(const CServerPathData& v)
{
	m_prefix = v.m_prefix;
	m_segments = v.m_segments;
}

bool CServerPathData::operator==(const CServerPathData& cmp) const
{
	if (m_prefix != cmp.m_prefix)
		return false;

	if (m_segments != cmp.m_segments)
		return false;

	return true;
}

CServerPath::CServerPath()
{
	m_type = DEFAULT;
	m_bEmpty = true;
}

CServerPath::CServerPath(const CServerPath &path, wxString subdir)
	: m_data(path.m_data)
{
	m_type = path.m_type;
	m_bEmpty = path.m_bEmpty;

	if (subdir == _T(""))
		return;

	if (!ChangePath(subdir))
		Clear();
}

CServerPath::CServerPath(const CServerPath &path)
	: m_data(path.m_data)
{
	m_type = path.m_type;
	m_bEmpty = path.m_bEmpty;
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
	m_data.clear();
}

bool CServerPath::SetPath(wxString newPath)
{
	return SetPath(newPath, false);
}

bool CServerPath::SetPath(wxString &newPath, bool isFile)
{
	wxString path = newPath;
	wxString file;

	if (path == _T(""))
		return false;

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

	m_data.clear();
	m_bEmpty = true;

	if (!ChangePath(path, isFile))
		return false;

	if (isFile)
		newPath = path;
	return true;
}

wxString CServerPath::GetPath() const
{
	if (m_bEmpty)
		return _T("");

	wxString path;

	if (!traits[m_type].prefixmode)
		path = m_data->m_prefix;

	if (traits[m_type].left_enclosure != 0)
		path += traits[m_type].left_enclosure;
	if (m_data->m_segments.empty() && (!traits[m_type].has_root || m_data->m_prefix.empty() || traits[m_type].separator_after_prefix))
		path += traits[m_type].separators[0];

	for (tConstSegmentIter iter = m_data->m_segments.begin(); iter != m_data->m_segments.end(); ++iter)
	{
		const wxString& segment = *iter;
		if (iter != m_data->m_segments.begin())
			path += traits[m_type].separators[0];
		else if (traits[m_type].has_root)
		{
			if (m_data->m_prefix.empty() || traits[m_type].separator_after_prefix)
				path += traits[m_type].separators[0];
		}

		if (traits[m_type].separatorEscape)
		{
			wxString tmp = segment;
			EscapeSeparators(m_type, tmp);
			path += tmp;
		}
		else
			path += segment;
	}

	if (traits[m_type].prefixmode)
		path += m_data->m_prefix;

	if (traits[m_type].right_enclosure != 0)
		path += traits[m_type].right_enclosure;

	// DOS is strange.
	// C: is current working dir on drive C, C:\ the drive root.
	if (m_type == DOS && m_data->m_segments.size() == 1)
		path += traits[m_type].separators[0];

	return path;
}

bool CServerPath::HasParent() const
{
	if (m_bEmpty)
		return false;

	if (!traits[m_type].has_root)
		return m_data->m_segments.size() > 1;

	return !m_data->m_segments.empty();
}

CServerPath CServerPath::GetParent() const
{
	if (!HasParent())
		return CServerPath();

	CServerPath parent(*this);
	CServerPathData& parent_data = parent.m_data.Get();

	parent_data.m_segments.pop_back();

	if (m_type == MVS)
		parent_data.m_prefix = _T(".");

	return parent;
}

wxString CServerPath::GetLastSegment() const
{
	if (!HasParent())
		return _T("");

	if (!m_data->m_segments.empty())
		return m_data->m_segments.back();
	else
		return _T("");
}

wxString CServerPath::GetSafePath() const
{
	if (m_bEmpty)
		return _T("");

	wxString safepath;
	safepath.Alloc(256);
	safepath.Printf(_T("%d %d"), m_type, (int)m_data->m_prefix.Length());
	if (m_data->m_prefix != _T(""))
		safepath += _T(" ") + m_data->m_prefix;

	wxChar buffer[20];
	for (tConstSegmentIter iter = m_data->m_segments.begin(); iter != m_data->m_segments.end(); ++iter)
	{
		wxPrintf(buffer, " %u ", static_cast<unsigned int>(iter->Length()));
		safepath += buffer;
		safepath += *iter;
	}

	return safepath;
}

bool CServerPath::SetSafePath(const wxString& path)
{
	CServerPathData& data = m_data.Get();
	m_bEmpty = true;
	data.m_prefix.clear();
	data.m_segments.clear();

	// Optimized for speed, avoid expensive wxString functions
	// Before the optimization this function was responsible for
	// most CPU cycles used during loading of transfer queues
	// from file
	const int len = (int)path.Len();
	wxChar* begin = new wxChar[len + 1];
	CSharedPointerArray<wxChar> tmp(begin);
	memcpy(begin, (const wxChar*)path, (len + 1) * sizeof(wxChar));
	wxChar* p = begin;

	int type = 0;
	do
	{
		if (*p < '0' || *p > '9')
			return false;
		type *= 10;
		type += *p - '0';

		if (type >= SERVERTYPE_MAX)
			return false;
		++p;
	} while (*p != ' ');

	m_type = (ServerType)type;
	++p;

	int prefix_len = 0;
	do
	{
		if (*p < '0' || *p > '9')
			return false;
		prefix_len *= 10;
		prefix_len += *p - '0';

		if (prefix_len > 32767) // Should be sane enough
			return false;
		++p;
	}
	while (*p && *p != ' ');

	if (!*p)
	{
		if (prefix_len != 0)
			return false;
		else
		{
			// Is root directory, like / on unix like systems.
			m_bEmpty = false;
			return true;
		}
	}

	++p;

	if (len - (p - begin) < prefix_len)
		return false;
	if (prefix_len)
	{
		*(p + prefix_len) = 0;
		data.m_prefix = p;

		p += prefix_len + 1;
	}
	
	while (len > (p - begin))
	{
		int segment_len = 0;
		do
		{
			if (*p < '0' || *p > '9')
				return false;
			segment_len *= 10;
			segment_len += *p - '0';

			if (segment_len > 32767) // Should be sane enough
				return false;
			++p;
		}
		while (*p != ' ');

		if (!segment_len)
			return false;
		++p;

		if (len - (p - begin) < segment_len)
			return false;
		*(p + segment_len) = 0;
		data.m_segments.push_back(p);

		p += segment_len + 1;
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
		if (cmpNoCase && m_data->m_prefix.CmpNoCase(path.m_data->m_prefix))
			return false;
		if (!cmpNoCase && m_data->m_prefix != path.m_data->m_prefix)
			return false;
	}

	// On MVS, dirs like 'FOO.BAR' without trailing dot cannot have
	// subdirectories
	if (traits[m_type].prefixmode == 1 && path.m_data->m_prefix == _T(""))
		return false;

	tConstSegmentIter iter1 = m_data->m_segments.begin();
	tConstSegmentIter iter2 = path.m_data->m_segments.begin();
	while (iter1 != m_data->m_segments.end())
	{
		if (iter2 == path.m_data->m_segments.end())
			return true;
		if (cmpNoCase)
		{
			if (iter1->CmpNoCase(*iter2))
				return false;
		}
		else if (*iter1 != *iter2)
			return false;

		++iter1;
		++iter2;
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

	if (dir == _T(""))
	{
		if (IsEmpty() || isFile)
			return false;
		else
			return true;
	}

	CServerPathData& data = m_data.Get();

	switch (m_type)
	{
	case VMS:
		{
			int pos1 = dir.Find(traits[m_type].left_enclosure);
			if (pos1 == -1)
			{
				int pos2 = dir.Find(traits[m_type].right_enclosure, true);
				if (pos2 != -1)
					return false;

				if (isFile)
				{
					if (IsEmpty())
						return false;

					file = dir;
					break;
				}
			}
			else
			{
				int pos2 = dir.Find(traits[m_type].right_enclosure, true);
				if (pos2 == -1 || pos2 <= pos1 + 1)
					return false;

				bool enclosure_is_last = static_cast<size_t>(pos2) == (dir.Length() - 1);
				if (isFile == enclosure_is_last)
					return false;

				if (isFile)
					file = dir.Mid(pos2 + 1);
				dir = dir.Left(pos2);

				if (pos1)
					data.m_prefix = dir.Left(pos1);
				dir = dir.Mid(pos1 + 1);

				data.m_segments.clear();
			}

			if (!Segmentize(dir, data.m_segments))
				return false;
			if (data.m_segments.empty() && m_bEmpty)
				return false;
		}
		break;
	case DOS:
		{
			bool is_relative = false;
			int sep = dir.find_first_of(traits[m_type].separators);
			if (sep == -1)
				sep = dir.Len();
			int colon = dir.Find(':');
			if (colon > 0 && colon == sep - 1)
				is_relative = true;

			if (is_relative)
				data.m_segments.clear();
			else if (wxString(traits[m_type].separators).Contains(dir[0]))
			{
				if (data.m_segments.empty())
					return false;
				wxString first = data.m_segments.front();
				data.m_segments.clear();
				data.m_segments.push_back(first);
				dir = dir.Mid(1);
			}

			if (isFile && !ExtractFile(dir, file))
				return false;

			if (!Segmentize(dir, data.m_segments))
				return false;
			if (data.m_segments.empty() && m_bEmpty)
				return false;
		}
		break;
	case MVS:
		{
			// Remove the double quoation some servers send in PWD reply
			int i = 0;
			wxChar c = dir.c_str()[i];
			while (c == FTP_MVS_DOUBLE_QUOTE)
				c = dir.c_str()[++i];
			dir.Remove(0, i);

			while (dir != _T(""))
			{
				c = dir.Last();
				if (c != FTP_MVS_DOUBLE_QUOTE)
					break;
				else
					dir.RemoveLast();
			}
		}
		if (dir == _T(""))
			return false;

		if (dir.c_str()[0] == traits[m_type].left_enclosure)
		{
			if (dir.Last() != traits[m_type].right_enclosure)
				return false;

			dir.RemoveLast();
			dir = dir.Mid(1);

			data.m_segments.clear();
		}
		else if (dir.Last() == traits[m_type].right_enclosure)
			return false;
		else if (m_bEmpty)
			return false;

		if (dir.Last() == ')')
		{
			// Partitioned dataset member
			if (!isFile)
				return false;

			int pos = dir.Find('(');
			if (pos == -1)
				return false;
			dir.RemoveLast();
			file = dir.Mid(pos + 1);
			dir = dir.Left(pos);

			if (!m_bEmpty && data.m_prefix == _T("") && !dir.empty())
				return false;

			data.m_prefix.clear();
		}
		else
		{
			if (!m_bEmpty && data.m_prefix == _T(""))
			{
				if (dir.Find('.') != -1 || !isFile)
					return false;
			}

			if (isFile)
			{
				if (!ExtractFile(dir, file))
					return false;
				data.m_prefix = _T(".");
			}
			else if (dir.Last() == '.')
				data.m_prefix = _T(".");
			else
				data.m_prefix.clear();
		}

		if (!Segmentize(dir, data.m_segments))
			return false;
		break;
	case HPNONSTOP:
		if (dir[0] == '\\')
			data.m_segments.clear();

		if (isFile && !ExtractFile(dir, file))
			return false;

		if (!Segmentize(dir, data.m_segments))
			return false;
		if (data.m_segments.empty() && m_bEmpty)
			return false;

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
				data.m_prefix = dir.Left(colon2 + 2);
				dir = dir.Mid(colon2 + 2);

				data.m_segments.clear();
			}

			if (isFile && !ExtractFile(dir, file))
				return false;

			if (!Segmentize(dir, data.m_segments))
				return false;
		}
		break;
	case CYGWIN:
		{
			if (wxString(traits[m_type].separators).Contains(dir[0]))
			{
				data.m_segments.clear();
				data.m_prefix.clear();
			}
			else if (m_bEmpty)
				return false;
			if (dir.Left(2) == _T("//"))
			{
				data.m_prefix = traits[m_type].separators[0];
				dir = dir.Mid(1);
			}

			if (isFile && !ExtractFile(dir, file))
				return false;

			if (!Segmentize(dir, data.m_segments))
				return false;
		}
		break;
	default:
		{
			if (wxString(traits[m_type].separators).Contains(dir[0]))
				data.m_segments.clear();
			else if (m_bEmpty)
				return false;

			if (isFile && !ExtractFile(dir, file))
				return false;

			if (!Segmentize(dir, data.m_segments))
				return false;
		}
		break;
	}

	if (!traits[m_type].has_root && data.m_segments.empty())
		return false;

	if (isFile)
	{
		if (traits[m_type].has_dots)
		{
			if (file == _T("..") || file == _T("."))
				return false;
		}
		subdir = file;
	}

	m_bEmpty = false;
	return true;
}

bool CServerPath::operator==(const CServerPath &op) const
{
	if (m_bEmpty != op.m_bEmpty)
		return false;
	else if (m_type != op.m_type)
		return false;
	else if (m_data != op.m_data)
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

	const int cmp = m_data->m_prefix.Cmp(op.m_data->m_prefix);
	if (cmp < 0)
		return true;
	if (cmp > 0)
		return false;

	if (m_type > op.m_type)
		return false;
	else if (m_type < op.m_type)
		return true;

	std::list<wxString>::const_iterator iter1, iter2;
	for (iter1 = m_data->m_segments.begin(), iter2 = op.m_data->m_segments.begin(); iter1 != m_data->m_segments.end(); ++iter1, ++iter2)
	{
		if (iter2 == op.m_data->m_segments.end())
			return false;

		const int cmp = iter1->Cmp(*iter2);
		if (cmp < 0)
			return true;
		if (cmp > 0)
			return false;
	}

	return iter2 != op.m_data->m_segments.end();
}

wxString CServerPath::FormatFilename(const wxString &filename, bool omitPath /*=false*/) const
{
	if (m_bEmpty)
		return filename;

	if (filename == _T(""))
		return _T("");

	if (m_bEmpty)
		return _T("");

	if (omitPath && (!traits[m_type].prefixmode || m_data->m_prefix == _T(".")))
		return filename;

	wxString result = GetPath();
	if (traits[m_type].left_enclosure && traits[m_type].filename_inside_enclosure)
		result.RemoveLast();

	switch (m_type)
	{
		case VXWORKS:
			if (!wxString(traits[m_type].separators).Contains(result.Last()) && !m_data->m_segments.empty())
				result += traits[m_type].separators[0];
			break;
		case VMS:
		case MVS:
			break;
		default:
			if (!wxString(traits[m_type].separators).Contains(result.Last()))
				result += traits[m_type].separators[0];
			break;
	}

	if (traits[m_type].prefixmode == 1 && m_data->m_prefix.empty())
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
	else if (m_data->m_prefix != op.m_data->m_prefix)
		return 1;
	else if (m_type != op.m_type)
		return 1;

	if (m_data->m_segments.size() > op.m_data->m_segments.size())
		return 1;
	else if (m_data->m_segments.size() < op.m_data->m_segments.size())
		return -1;

	tConstSegmentIter iter = m_data->m_segments.begin();
	tConstSegmentIter iter2 = op.m_data->m_segments.begin();
	while (iter != m_data->m_segments.end())
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
	m_data.Get().m_segments.push_back(segment);

	return true;
}

CServerPath CServerPath::GetCommonParent(const CServerPath& path) const
{
	if (*this == path)
		return *this;

	if (m_bEmpty || path.m_bEmpty)
		return CServerPath();

	if (m_type != path.m_type ||
		(!traits[m_type].prefixmode && m_data->m_prefix != path.m_data->m_prefix))
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

	CServerPathData& parentData = parent.m_data.Get();
	
	std::list<wxString>::const_iterator last = m_data->m_segments.end();
	std::list<wxString>::const_iterator last2 = path.m_data->m_segments.end();
	if (traits[m_type].prefixmode == 1)
	{
		if (m_data->m_prefix.empty())
			--last;
		if (path.m_data->m_prefix.empty())
			--last2;
		parentData.m_prefix = GetParent().m_data->m_prefix;
	}
	else
		parentData.m_prefix = m_data->m_prefix;

	std::list<wxString>::const_iterator iter = m_data->m_segments.begin();
	std::list<wxString>::const_iterator iter2 = path.m_data->m_segments.begin();
	while (iter != last && iter2 != last2)
	{
		if (*iter != *iter2)
		{
			if (!traits[m_type].has_root && parentData.m_segments.empty())
				return CServerPath();
			else
				return parent;
		}

		parentData.m_segments.push_back(*iter);

		++iter;
		++iter2;
	}

	return parent;
}

wxString CServerPath::FormatSubdir(const wxString &subdir) const
{
	if (!traits[m_type].separatorEscape)
		return subdir;

	wxString res = subdir;
	EscapeSeparators(m_type, res);

	return res;
}

bool CServerPath::Segmentize(wxString str, tSegmentList& segments)
{
	bool append = false;
	while (!str.empty())
	{
		wxString segment;
		int pos = str.find_first_of(traits[m_type].separators);
		if (pos == -1)
		{
			segment = str,
			str.clear();
		}
		else if (!pos)
		{
			str = str.Mid(1);
			continue;
		}
		else
		{
			segment = str.Left(pos);
			str = str.Mid(pos + 1);
		}

		if (traits[m_type].has_dots)
		{
			if (segment == _T("."))
				continue;
			else if (segment == _T(".."))
			{
				if (segments.empty())
					return false;
				else
				{
					segments.pop_back();
					continue;
				}
			}
		}

		bool append_next = false;
		if (traits[m_type].separatorEscape && segment.Last() == traits[m_type].separatorEscape)
		{
			append_next = true;
			segment.RemoveLast();
			segment += traits[m_type].separators[0];
		}

		if (append)
			segments.back() += segment;
		else
			segments.push_back(segment);

		append = append_next;
	}
	if (append)
		return false;

	return true;
}

bool CServerPath::ExtractFile(wxString& dir, wxString& file)
{
	int pos = dir.find_last_of(traits[m_type].separators);
	if (pos == (int)dir.Length() - 1)
		return false;

	if (pos == -1)
	{
		file = dir;
		dir.clear();
		return true;
	}

	file = dir.Mid(pos + 1);
	dir = dir.Left(pos + 1);

	return true;
}

void CServerPath::EscapeSeparators(ServerType type, wxString& subdir)
{
	if (traits[type].separatorEscape)
	{
		for (const wxChar* p = traits[type].separators; *p; ++p)
			subdir.Replace((wxString)*p, (wxString)traits[type].separatorEscape + traits[type].separators[0]);
	}
}