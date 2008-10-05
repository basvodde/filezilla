#include "FileZilla.h"
#include "auto_ascii_files.h"
#include "Options.h"
#include "local_filesys.h"

std::list<wxString> CAutoAsciiFiles::m_ascii_extensions;

void CAutoAsciiFiles::SettingsChanged()
{
	m_ascii_extensions.clear();
	wxString extensions = COptions::Get()->GetOption(OPTION_ASCIIFILES);
	wxString ext;
	int pos = extensions.Find(_T("|"));
	while (pos != -1)
	{
		if (!pos)
		{
			if (ext != _T(""))
			{
				ext.Replace(_T("\\\\"), _T("\\"));
				m_ascii_extensions.push_back(ext);
				ext = _T("");
			}
		}
		else if (extensions.c_str()[pos - 1] != '\\')
		{
			ext += extensions.Left(pos);
			ext.Replace(_T("\\\\"), _T("\\"));
			m_ascii_extensions.push_back(ext);
			ext = _T("");
		}
		else
		{
			ext += extensions.Left(pos - 1) + _T("|");
		}
		extensions = extensions.Mid(pos + 1);
		pos = extensions.Find(_T("|"));
	}
	ext += extensions;
	ext.Replace(_T("\\\\"), _T("\\"));
	m_ascii_extensions.push_back(ext);
}

// Defined in RemoteListView.cpp
wxString StripVMSRevision(const wxString& name);

bool CAutoAsciiFiles::TransferLocalAsAscii(wxString local_file, enum ServerType server_type)
{
	int pos = local_file.Find(CLocalFileSystem::path_separator, true);
	if (pos != -1)
		local_file = local_file.Mid(pos + 1);

	// Identical implementation, only difference is for the local one to strip path.
	return TransferRemoteAsAscii(local_file, server_type);
}

bool CAutoAsciiFiles::TransferRemoteAsAscii(wxString remote_file, enum ServerType server_type)
{
	int mode = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
	if (mode == 1)
		return true;
	else if (mode == 2)
		return false;

	if (server_type == VMS)
		remote_file = StripVMSRevision(remote_file);

	if (remote_file[0] == '.')
		return COptions::Get()->GetOptionVal(OPTION_ASCIIDOTFILE) != 0;

	int pos = remote_file.Find('.', true);
	if (pos == -1)
		return COptions::Get()->GetOptionVal(OPTION_ASCIINOEXT) != 0;
	remote_file = remote_file.Mid(pos + 1);

	if (remote_file == _T(""))
		return false;

	for (std::list<wxString>::const_iterator iter = m_ascii_extensions.begin(); iter != m_ascii_extensions.end(); iter++)
		if (!remote_file.CmpNoCase(*iter))
			return true;

	return false;
}

