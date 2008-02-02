#include "FileZilla.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"
#include <wx/tokenzr.h>
#include "ipcmutex.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

COptions* COptions::m_theOptions = 0;

enum Type
{
	string,
	number
};

struct t_Option
{
	char name[30];
	enum Type type;
	wxString defaultValue; // Default values are stored as string even for numerical options
};

static const t_Option options[OPTIONS_NUM] =
{
	// Engine settings
	{ "Use Pasv mode", number, _T("1") },
	{ "Limit local ports", number, _T("0") },
	{ "Limit ports low", number, _T("6000") },
	{ "Limit ports high", number, _T("7000") },
	{ "External IP mode", number, _T("0") },
	{ "External IP", string, _T("") },
	{ "External address resolver", string, _T("http://ip.filezilla-project.org/ip.php") },
	{ "Last resolved IP", string, _T("") },
	{ "No external ip on local conn", number, _T("1") },
	{ "Pasv reply fallback mode", number, _T("0") },
	{ "Timeout", number, _T("20") },
	{ "Logging Debug Level", number, _T("0") },
	{ "Logging Raw Listing", number, _T("0") },
	{ "fzsftp executable", string, _T("") },
	{ "Allow transfermode fallback", number, _T("1") },
	{ "Reconnect count", number, _T("2") },
	{ "Reconnect delay", number, _T("5") },
	{ "Speedlimit inbound", number, _T("0") },
	{ "Speedlimit outbound", number, _T("0") },
	{ "Speedlimit burst tolerance", number, _T("0") },
	{ "View hidden files", number, _T("0") },
	{ "Preserve timestamps", number, _T("0") },
	{ "Socket recv buffer size", number, _T("131072") }, // Make it large enough by default
														 // to enable a large TCP window scale
	{ "Socket send buffer size", number, _T("131072") },
	{ "FTP Keep-alive commands", number, _T("0") },
	{ "FTP Proxy type", number, _T("0") },
	{ "FTP Proxy host", string, _T("") },
	{ "FTP Proxy user", string, _T("") },
	{ "FTP Proxy password", string, _T("") },
	{ "FTP Proxy login sequence", string, _T("") },

	// Interface settings
	{ "Number of Transfers", number, _T("2") },
	{ "Ascii Binary mode", number, _T("0") },
	{ "Auto Ascii files", string, _T("am|asp|bat|c|cfm|cgi|conf|cpp|css|dhtml|diz|h|hpp|htm|html|in|inc|js|m4|mak|nfo|nsi|pas|patch|php|phtml|pl|po|py|qmail|sh|shtml|sql|tcl|tpl|txt|vbs|xml|xrc") },
	{ "Auto Ascii no extension", number, _T("1") },
	{ "Auto Ascii dotfiles", number, _T("1") },
	{ "Theme", string, _T("") },
	{ "Language Code", string, _T("") },
	{ "Last Server Path", string, _T("") },
	{ "Max Concurrent Uploads", number, _T("0") },
	{ "Max Concurrent Downloads", number, _T("0") },
	{ "Update Check", number, _T("1") },
	{ "Update Check Interval", number, _T("7") },
	{ "Last automatic update check", string, _T("") },
	{ "Update Check New Version", string, _T("") },
	{ "Update Check Package URL", string, _T("") },
	{ "Show debug menu", number, _T("0") },
	{ "File exists action download", number, _T("0") },
	{ "File exists action upload", number, _T("0") },
	{ "Allow ascii resume", number, _T("0") },
	{ "Greeting version", string, _T("") },
	{ "Onetime Dialogs", string, _T("") },
	{ "Show Tree Local", number, _T("1") },
	{ "Show Tree Remote", number, _T("1") },
	{ "File Pane Layout", number, _T("0") },
	{ "File Pane Swap", number, _T("0") },
	{ "Last local directory", string, _T("") },
	{ "Filelist directory sort", number, _T("0") },
	{ "Queue successful autoclear", number, _T("0") },
	{ "Queue column widths", string, _T("") },
	{ "Local filelist colwidths", string, _T("") },
	{ "Remote filelist colwidths", string, _T("") },
	{ "Window position and size", string, _T("") },
	{ "Splitter positions", string, _T("") },
	{ "Local filelist sortorder", string, _T("") },
	{ "Remote filelist sortorder", string, _T("") },
	{ "Time Format", string, _T("") },
	{ "Date Format", string, _T("") },
	{ "Show message log", number, _T("1") },
	{ "Show queue", number, _T("1") },
	{ "Size format", number, _T("0") },
	{ "Size thousands separator", number, _T("1") },
	{ "Default editor", string, _T("") },
	{ "Always use default editor", number, _T("0") },
	{ "Inherit system associations", number, _T("1") },
	{ "Custom file associations", string, _T("") },
	{ "Comparison mode", number, _T("1") },
	{ "Comparison threshold", number, _T("1") },
	{ "Site Manager position", string, _T("") },
	{ "Theme icon size", string, _T("") }
};

COptions::COptions()
{
	m_pLastServer = 0;

	m_acquired = false;

	for (unsigned int i = 0; i < OPTIONS_NUM; i++)
		m_optionsCache[i].cached = false;

	CInterProcessMutex mutex(MUTEX_OPTIONS);
	m_pXmlFile = new CXmlFile(_T("filezilla"));
	if (!m_pXmlFile->Load())
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", make sure the file is valid.\nFor this session, default settings will be used and any changes to the settings are not persistent."), m_pXmlFile->GetFileName().GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);
		delete m_pXmlFile;
		m_pXmlFile = 0;
	}
	else
		CreateSettingsXmlElement();
}

COptions::~COptions()
{
	delete m_pLastServer;
	delete m_pXmlFile;
}

int COptions::GetOptionVal(unsigned int nID)
{
	if (nID >= OPTIONS_NUM)
		return 0;

	if (options[nID].type != number)
		return 0;

	if (m_optionsCache[nID].cached)
		return m_optionsCache[nID].numValue;

	wxString value;
	long numValue = 0;
	if (!GetXmlValue(nID, value))
		options[nID].defaultValue.ToLong(&numValue);
	else
	{
		value.ToLong(&numValue);
		numValue = Validate(nID, numValue);
	}

	m_optionsCache[nID].numValue = numValue;
	m_optionsCache[nID].cached = true;

	return numValue;
}

wxString COptions::GetOption(unsigned int nID)
{
	if (nID >= OPTIONS_NUM)
		return _T("");

	if (options[nID].type != string)
		return wxString::Format(_T("%d"), GetOptionVal(nID));

	if (m_optionsCache[nID].cached)
		return m_optionsCache[nID].strValue;

	wxString value;
	if (!GetXmlValue(nID, value))
		value = options[nID].defaultValue;
	else
		Validate(nID, value);

	m_optionsCache[nID].strValue = value;
	m_optionsCache[nID].cached = true;

	return value;
}

bool COptions::SetOption(unsigned int nID, int value)
{
	if (!SetOptionNoSave(nID, value))
		return false;

	if (m_pXmlFile)
	{
		CInterProcessMutex mutex(MUTEX_OPTIONS);
		m_pXmlFile->Save();
	}

	return true;
}

bool COptions::SetOptionNoSave(unsigned int nID, int value)
{
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != number)
		return false;

	value = Validate(nID, value);

	m_optionsCache[nID].cached = true;
	m_optionsCache[nID].numValue = value;

	if (m_pXmlFile)
		SetXmlValue(nID, wxString::Format(_T("%d"), value));

	return true;
}

bool COptions::SetOption(unsigned int nID, wxString value)
{
	if (!SetOptionNoSave(nID, value))
		return false;

	if (m_pXmlFile)
	{
		CInterProcessMutex mutex(MUTEX_OPTIONS);
		m_pXmlFile->Save();
	}

	return true;
}

bool COptions::SetOptionNoSave(unsigned int nID, wxString value)
{
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != string)
	{
		long tmp;
		if (!value.ToLong(&tmp))
			return false;

		return SetOption(nID, tmp);
	}

	Validate(nID, value);

	m_optionsCache[nID].cached = true;
	m_optionsCache[nID].strValue = value;

	if (m_pXmlFile)
		SetXmlValue(nID, value);


	return true;
}

void COptions::CreateSettingsXmlElement()
{
	if (!m_pXmlFile)
		return;

	TiXmlElement *element = m_pXmlFile->GetElement();
	if (element->FirstChildElement("Settings"))
		return;

	TiXmlElement settings("Settings");
	element->InsertEndChild(settings);

	for (int i = 0; i < OPTIONS_NUM; i++)
	{
		m_optionsCache[i].cached = true;
		if (options[i].type == string)
			m_optionsCache[i].strValue = options[i].defaultValue;
		else
		{
			long numValue = 0;
			options[i].defaultValue.ToLong(&numValue);
			m_optionsCache[i].numValue = numValue;
		}
		SetXmlValue(i, options[i].defaultValue);
	}

	m_pXmlFile->Save();
}

void COptions::SetXmlValue(unsigned int nID, wxString value)
{
	if (!m_pXmlFile)
		return;

	// No checks are made about the validity of the value, that's done in SetOption

	char *utf8 = ConvUTF8(value);
	if (!utf8)
		return;

	TiXmlElement *settings = m_pXmlFile->GetElement()->FirstChildElement("Settings");
	if (!settings)
	{
		TiXmlNode *node = m_pXmlFile->GetElement()->InsertEndChild(TiXmlElement("Settings"));
		if (!node)
		{
			delete [] utf8;
			return;
		}
		settings = node->ToElement();
		if (!settings)
		{
			delete [] utf8;
			return;
		}
	}
	else
	{
		TiXmlNode *node = 0;
		while ((node = settings->IterateChildren("Setting", node)))
		{
			TiXmlElement *setting = node->ToElement();
			if (!setting)
				continue;

			const char *attribute = setting->Attribute("name");
			if (!attribute)
				continue;
			if (strcmp(attribute, options[nID].name))
				continue;

			setting->RemoveAttribute("type");
			setting->Clear();
			setting->SetAttribute("type", (options[nID].type == string) ? "string" : "number");
			setting->InsertEndChild(TiXmlText(utf8));

			delete [] utf8;
			return;
		}
	}
	TiXmlElement setting("Setting");
	setting.SetAttribute("name", options[nID].name);
	setting.SetAttribute("type", (options[nID].type == string) ? "string" : "number");
	setting.InsertEndChild(TiXmlText(utf8));
	settings->InsertEndChild(setting);

	delete [] utf8;
}

bool COptions::GetXmlValue(unsigned int nID, wxString &value, TiXmlElement *settings /*=0*/)
{
	if (!settings)
	{
		if (!m_pXmlFile)
			return false;

		settings = m_pXmlFile->GetElement()->FirstChildElement("Settings");
		if (!settings)
		{
			TiXmlNode *node = m_pXmlFile->GetElement()->InsertEndChild(TiXmlElement("Settings"));
			if (!node)
				return false;
			settings = node->ToElement();
			if (!settings)
				return false;
		}
	}

	TiXmlNode *node = 0;
	while ((node = settings->IterateChildren("Setting", node)))
	{
		TiXmlElement *setting = node->ToElement();
		if (!setting)
			continue;

		const char *attribute = setting->Attribute("name");
		if (!attribute)
			continue;
		if (strcmp(attribute, options[nID].name))
			continue;

		TiXmlNode *text = setting->FirstChild();
		if (!text || !text->ToText())
			return false;

		value = ConvLocal(text->Value());

		return true;
	}

	return false;
}

int COptions::Validate(unsigned int nID, int value)
{
	switch (nID)
	{
	case OPTION_UPDATECHECK_INTERVAL:
		if (value < 7 || value > 9999)
			value = 7;
		break;
	case OPTION_LOGGING_DEBUGLEVEL:
		if (value < 0 || value > 4)
			value = 0;
		break;
	case OPTION_RECONNECTCOUNT:
		if (value < 0 || value > 99)
			value = 5;
		break;
	case OPTION_RECONNECTDELAY:
		if (value < 0 || value > 999)
			value = 5;
		break;
	case OPTION_FILEPANE_LAYOUT:
		if (value < 0 || value > 3)
			value = 0;
		break;
	case OPTION_SPEEDLIMIT_INBOUND:
	case OPTION_SPEEDLIMIT_OUTBOUND:
		if (value < 0)
			value = 0;
		break;
	case OPTION_SPEEDLIMIT_BURSTTOLERANCE:
		if (value < 0 || value > 2)
			value = 0;
		break;
	case OPTION_FILELIST_DIRSORT:
		if (value < 0 || value > 2)
			value = 0;
		break;
	case OPTION_SOCKET_BUFFERSIZE_RECV:
		if (value != -1 && (value < 4096 || value > 4096 * 1024))
			value = -1;
		break;
	case OPTION_SOCKET_BUFFERSIZE_SEND:
		if (value != -1 && (value < 4096 || value > 4096 * 1024))
			value = 131072;
		break;
	case OPTION_COMPARISONMODE:
		if (value < 0 || value > 0)
			value = 1;
		break;
	case OPTION_COMPARISON_THRESHOLD:
		if (value < 0 || value > 1440)
			value = 1;
		break;
	}
	return value;
}

wxString COptions::Validate(unsigned int nID, wxString value)
{
	return value;
}

void COptions::SetServer(wxString path, const CServer& server)
{
	if (!m_pXmlFile)
		return;

	if (path == _T(""))
		return;

	TiXmlElement *element = m_pXmlFile->GetElement();

	while (path != _T(""))
	{
		wxString sub;
		int pos = path.Find('/');
		if (pos != -1)
		{
			sub = path.Left(pos);
			path = path.Mid(pos + 1);
		}
		else
		{
			sub = path;
			path = _T("");
		}
		char *utf8 = ConvUTF8(sub);
		if (!utf8)
			return;
		TiXmlElement *newElement = element->FirstChildElement(utf8);
		delete [] utf8;
		if (newElement)
			element = newElement;
		else
		{
			char *utf8 = ConvUTF8(sub);
			if (!utf8)
				return;
			TiXmlNode *node = element->InsertEndChild(TiXmlElement(utf8));
			delete [] utf8;
			if (!node || !node->ToElement())
				return;
			element = node->ToElement();
		}
	}

	::SetServer(element, server);

	CInterProcessMutex mutex(MUTEX_OPTIONS);
	m_pXmlFile->Save();
}

bool COptions::GetServer(wxString path, CServer& server)
{
	if (path == _T(""))
		return false;

	if (!m_pXmlFile)
		return false;
	TiXmlElement *element = m_pXmlFile->GetElement();

	while (path != _T(""))
	{
		wxString sub;
		int pos = path.Find('/');
		if (pos != -1)
		{
			sub = path.Left(pos);
			path = path.Mid(pos + 1);
		}
		else
		{
			sub = path;
			path = _T("");
		}
		char *utf8 = ConvUTF8(sub);
		if (!utf8)
			return false;
		element = element->FirstChildElement(utf8);
		delete [] utf8;
		if (!element)
			return false;
	}

	bool res = ::GetServer(element, server);

	return res;
}

void COptions::SetLastServer(const CServer& server)
{
	if (!m_pLastServer)
		m_pLastServer = new CServer(server);
	else
		*m_pLastServer = server;
	SetServer(_T("Settings/LastServer"), server);
}

bool COptions::GetLastServer(CServer& server)
{
	if (!m_pLastServer)
	{
		bool res = GetServer(_T("Settings/LastServer"), server);
		if (res)
			m_pLastServer = new CServer(server);
		return res;
	}
	else
	{
		server = *m_pLastServer;
		if (server == CServer())
			return false;

		return true;
	}
}

void COptions::Init()
{
	if (!m_theOptions)
		m_theOptions = new COptions();
}

void COptions::Destroy()
{
	if (!m_theOptions)
		return;

	delete m_theOptions;
	m_theOptions = 0;
}

COptions* COptions::Get()
{
	return m_theOptions;
}

void COptions::Import(TiXmlElement* pElement)
{
	for (int i = 0; i < OPTIONS_NUM; i++)
	{
		wxString value;
		if (!GetXmlValue(i, value, pElement))
			continue;

		SetOptionNoSave(i, value);
	}

	if (m_pXmlFile)
	{
		CInterProcessMutex mutex(MUTEX_OPTIONS);
		m_pXmlFile->Save();
	}
}

void COptions::SaveColumnWidths(const wxListCtrl* const pListCtrl, unsigned int optionId)
{
	wxCHECK_RET(pListCtrl, _T("SaveColumnWidths called with !pListCtrl"));

	wxString widths;
	for (int i = 0; i < pListCtrl->GetColumnCount(); i++)
		widths += wxString::Format(_T("%d "), pListCtrl->GetColumnWidth(i));
	widths.RemoveLast();

	SetOption(optionId, widths);
}

bool COptions::ReadColumnWidths(unsigned int optionId, unsigned int count, unsigned long* widths)
{
	wxString savedWidths = GetOption(optionId);
	wxStringTokenizer tokens(savedWidths, _T(" "));
	if (tokens.CountTokens() != count)
		return false;

	unsigned long* newWidths = new unsigned long[count];
	for (unsigned int i = 0; i < count; i++)
	{
		wxString token = tokens.GetNextToken();
		if (!token.ToULong(newWidths + i) || newWidths[i] > 5000)
		{
			delete [] newWidths;
			return false;
		}
	}

	for (unsigned int i = 0; i < count; i++)
		widths[i] = newWidths[i];

	delete [] newWidths;
	return true;
}

