#ifndef __AUTO_ASCII_FILES_H__
#define __AUTO_ASCII_FILES_H__

class CAutoAsciiFiles
{
public:
	static bool TransferLocalAsAscii(wxString local_file, enum ServerType server_type);
	static bool TransferRemoteAsAscii(wxString remote_file, enum ServerType server_type);

	static void SettingsChanged();
protected:
	static std::list<wxString> m_ascii_extensions;
};

#endif //__AUTO_ASCII_FILES_H__
