#include "FileZilla.h"
#include "Options.h"
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
	bool internal; // internal items won't get written to settings file nor loaded from there
};

static const t_Option options[OPTIONS_NUM] =
{
	// Engine settings
	{ "Use Pasv mode", number, _T("1"), false },
	{ "Limit local ports", number, _T("0"), false },
	{ "Limit ports low", number, _T("6000"), false },
	{ "Limit ports high", number, _T("7000"), false },
	{ "External IP mode", number, _T("0"), false },
	{ "External IP", string, _T(""), false },
	{ "External address resolver", string, _T("http://ip.filezilla-project.org/ip.php"), false },
	{ "Last resolved IP", string, _T(""), false },
	{ "No external ip on local conn", number, _T("1"), false },
	{ "Pasv reply fallback mode", number, _T("0"), false },
	{ "Timeout", number, _T("20"), false },
	{ "Logging Debug Level", number, _T("0"), false },
	{ "Logging Raw Listing", number, _T("0"), false },
	{ "fzsftp executable", string, _T(""), false },
	{ "Allow transfermode fallback", number, _T("1"), false },
	{ "Reconnect count", number, _T("2"), false },
	{ "Reconnect delay", number, _T("5"), false },
	{ "Speedlimit inbound", number, _T("0"), false },
	{ "Speedlimit outbound", number, _T("0"), false },
	{ "Speedlimit burst tolerance", number, _T("0"), false },
	{ "View hidden files", number, _T("0"), false },
	{ "Preserve timestamps", number, _T("0"), false },
	{ "Socket recv buffer size", number, _T("131072"), false }, // Make it large enough by default
														 // to enable a large TCP window scale
	{ "Socket send buffer size", number, _T("131072"), false },
	{ "FTP Keep-alive commands", number, _T("0"), false },
	{ "FTP Proxy type", number, _T("0"), false },
	{ "FTP Proxy host", string, _T(""), false },
	{ "FTP Proxy user", string, _T(""), false },
	{ "FTP Proxy password", string, _T(""), false },
	{ "FTP Proxy login sequence", string, _T(""), false },
	{ "SFTP keyfiles", string, _T(""), false },
	{ "Proxy type", number, _T("0"), false },
	{ "Proxy host", string, _T(""), false },
	{ "Proxy port", number, _T("0"), false },
	{ "Proxy user", string, _T(""), false },
	{ "Proxy password", string, _T(""), false },
	{ "Logging file", string, _T(""), false },
	{ "Logging filesize limit", number, _T("10"), false },
	{ "Trusted root certificate", string, _T(""), true },

	// Interface settings
	{ "Number of Transfers", number, _T("2"), false },
	{ "Ascii Binary mode", number, _T("0"), false },
	{ "Auto Ascii files", string, _T("am|asp|bat|c|cfm|cgi|conf|cpp|css|dhtml|diz|h|hpp|htm|html|in|inc|js|jsp|m4|mak|md5|nfo|nsi|pas|patch|php|phtml|pl|po|py|qmail|sh|shtml|sql|svg|tcl|tpl|txt|vbs|xhtml|xml|xrc"), false },
	{ "Auto Ascii no extension", number, _T("1"), false },
	{ "Auto Ascii dotfiles", number, _T("1"), false },
	{ "Theme", string, _T("opencrystal/"), false },
	{ "Language Code", string, _T(""), false },
	{ "Last Server Path", string, _T(""), false },
	{ "Concurrent download limit", number, _T("0"), false },
	{ "Concurrent upload limit", number, _T("0"), false },
	{ "Update Check", number, _T("1"), false },
	{ "Update Check Interval", number, _T("7"), false },
	{ "Last automatic update check", string, _T(""), false },
	{ "Update Check New Version", string, _T(""), false },
	{ "Update Check Check Beta", number, _T("0"), false },
	{ "Show debug menu", number, _T("0"), false },
	{ "File exists action download", number, _T("0"), false },
	{ "File exists action upload", number, _T("0"), false },
	{ "Allow ascii resume", number, _T("0"), false },
	{ "Greeting version", string, _T(""), false },
	{ "Onetime Dialogs", string, _T(""), false },
	{ "Show Tree Local", number, _T("1"), false },
	{ "Show Tree Remote", number, _T("1"), false },
	{ "File Pane Layout", number, _T("0"), false },
	{ "File Pane Swap", number, _T("0"), false },
	{ "Last local directory", string, _T(""), false },
	{ "Filelist directory sort", number, _T("0"), false },
	{ "Queue successful autoclear", number, _T("0"), false },
	{ "Queue column widths", string, _T(""), false },
	{ "Local filelist colwidths", string, _T(""), false },
	{ "Remote filelist colwidths", string, _T(""), false },
	{ "Window position and size", string, _T(""), false },
	{ "Splitter positions (v2)", string, _T(""), false },
	{ "Local filelist sortorder", string, _T(""), false },
	{ "Remote filelist sortorder", string, _T(""), false },
	{ "Time Format", string, _T(""), false },
	{ "Date Format", string, _T(""), false },
	{ "Show message log", number, _T("1"), false },
	{ "Show queue", number, _T("1"), false },
	{ "Size format", number, _T("0"), false },
	{ "Size thousands separator", number, _T("1"), false },
	{ "Default editor", string, _T(""), false },
	{ "Always use default editor", number, _T("0"), false },
	{ "Inherit system associations", number, _T("1"), false },
	{ "Custom file associations", string, _T(""), false },
	{ "Comparison mode", number, _T("1"), false },
	{ "Comparison threshold", number, _T("1"), false },
	{ "Site Manager position", string, _T(""), false },
	{ "Theme icon size", string, _T(""), false },
	{ "Timestamp in message log", number, _T("0"), false },
	{ "Sitemanager last selected", string, _T(""), false },
	{ "Local filelist shown columns", string, _T(""), false },
	{ "Remote filelist shown columns", string, _T(""), false },
	{ "Local filelist column order", string, _T(""), false },
	{ "Remote filelist column order", string, _T(""), false },
	{ "Filelist status bar", number, _T("1"), false },
	{ "Filter toggle state", number, _T("0"), false },
	{ "Size decimal places", number, _T("0"), false },
	{ "Show quickconnect bar", number, _T("1"), false },
	{ "Messagelog position", number, _T("0"), false },
	{ "Last connected site", string, _T(""), false },
	{ "File doubleclock action", number, _T("0"), false },
	{ "Dir doubleclock action", number, _T("0"), false },
	{ "Minimize to tray", number, _T("0"), false },
	{ "Search column widths", string, _T(""), false },
	{ "Search column shown", string, _T(""), false },
	{ "Search column order", string, _T(""), false },
	{ "Search window size", string, _T(""), false },
	{ "Comparison hide identical", number, _T("0"), false },
	{ "Search sort order", string, _T(""), false },
	{ "Edit track local", number, _T("1"), false },
	{ "Prevent idle sleep", number, _T("1"), false },
	{ "Filteredit window size", string, _T(""), false },
	{ "Enable invalid char filter", number, _T("1"), false },
	{ "Invalid char replace", string, _T("_"), false },
};

struct t_default_option
{
	const wxChar name[30];
	enum Type type;
	wxString value_str;
	int value_number;
};

static t_default_option default_options[DEFAULTS_NUM] =
{
	{ _T("Config Location"), string, _T(""), 0 },
	{ _T("Kiosk mode"), number, _T(""), 0 },
	{ _T("Disable update check"), number, _T(""), 0 }
};

BEGIN_EVENT_TABLE(COptions, wxEvtHandler)
EVT_TIMER(wxID_ANY, COptions::OnTimer)
END_EVENT_TABLE()

COptions::COptions()
{
	m_pLastServer = 0;

	m_acquired = false;

	for (unsigned int i = 0; i < OPTIONS_NUM; i++)
		m_optionsCache[i].cached = false;

	m_save_timer.SetOwner(this);

	CInterProcessMutex mutex(MUTEX_OPTIONS);
	m_pXmlFile = new CXmlFile(_T("filezilla"));
	if (!m_pXmlFile->Load())
	{
		wxString msg = m_pXmlFile->GetError() + _T("\n\n") + _("For this session the default settings will be used. Any changes to the settings will not be saved.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);
		delete m_pXmlFile;
		m_pXmlFile = 0;
	}
	else
		CreateSettingsXmlElement();

	LoadGlobalDefaultOptions();
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
	if (options[nID].internal || !GetXmlValue(nID, value))
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
	if (options[nID].internal || !GetXmlValue(nID, value))
		value = options[nID].defaultValue;
	else
		Validate(nID, value);

	m_optionsCache[nID].strValue = value;
	m_optionsCache[nID].cached = true;

	return value;
}

bool COptions::SetOption(unsigned int nID, int value)
{
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != number)
		return false;

	value = Validate(nID, value);

	m_optionsCache[nID].cached = true;
	m_optionsCache[nID].numValue = value;

	if (m_pXmlFile && !options[nID].internal)
	{
		SetXmlValue(nID, wxString::Format(_T("%d"), value));

		if (!m_save_timer.IsRunning())
			m_save_timer.Start(15000, true);
	}

	return true;
}

bool COptions::SetOption(unsigned int nID, wxString value)
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

	if (m_pXmlFile && !options[nID].internal)
	{
		SetXmlValue(nID, value);

		if (!m_save_timer.IsRunning())
			m_save_timer.Start(15000, true);
	}


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
	wxASSERT(options[nID].name[0]);
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
		if (!text)
		{
			value.clear();
			return true;
		}

		if (!text->ToText())
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
	case OPTION_SIZE_DECIMALPLACES:
		if (value < 0 || value > 3)
			value = 0;
		break;
	case OPTION_MESSAGELOG_POSITION:
		if (value < 0 || value > 2)
			value = 0;
		break;
	case OPTION_DOUBLECLICK_ACTION_FILE:
	case OPTION_DOUBLECLICK_ACTION_DIRECTORY:
		if (value < 0 || value > 3)
			value = 0;
		break;
	}
	return value;
}

wxString COptions::Validate(unsigned int nID, wxString value)
{
	if (nID == OPTION_INVALID_CHAR_REPLACE)
	{
		if (value.Len() > 1)
			value = _T("_");
	}
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

	if (GetDefaultVal(DEFAULT_KIOSKMODE) == 2)
		return;
	
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
		if (options[i].internal)
			continue;
		wxString value;
		if (!GetXmlValue(i, value, pElement))
			continue;

		SetOption(i, value);
	}
}

void COptions::LoadGlobalDefaultOptions()
{
	const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
	if (defaultsDir == _T(""))
		return;
	
	wxFileName name(defaultsDir, _T("fzdefaults.xml"));
	CXmlFile file(name);
	if (!file.Load())
		return;

	TiXmlElement* pElement = file.GetElement();
	if (!pElement)
		return;

	pElement = pElement->FirstChildElement("Settings");
	if (!pElement)
		return;

	for (TiXmlElement* pSetting = pElement->FirstChildElement("Setting"); pSetting; pSetting = pSetting->NextSiblingElement("Setting"))
	{
		wxString name = GetTextAttribute(pSetting, "name");
		for (int i = 0; i < DEFAULTS_NUM; i++)
		{
			if (name != default_options[i].name)
				continue;

			wxString value = GetTextElement(pSetting);
			if (default_options[i].type == string)
				default_options[i].value_str = value;
			else
			{
				long v = 0;
				if (!value.ToLong(&v))
					v = 0;
				default_options[i].value_number = v;
			}

		}
	}
}

int COptions::GetDefaultVal(unsigned int nID) const
{
	if (nID >= DEFAULTS_NUM)
		return 0;

	return default_options[nID].value_number;
}

wxString COptions::GetDefault(unsigned int nID) const
{
	if (nID >= DEFAULTS_NUM)
		return _T("");

	return default_options[nID].value_str;
}

void COptions::OnTimer(wxTimerEvent& event)
{
	Save();
}

void COptions::Save()
{
	if (GetDefaultVal(DEFAULT_KIOSKMODE) == 2)
		return;

	if (!m_pXmlFile)
		return;

	CInterProcessMutex mutex(MUTEX_OPTIONS);
	m_pXmlFile->Save();
}

void COptions::SaveIfNeeded()
{
	if (!m_save_timer.IsRunning())
		return;

	m_save_timer.Stop();
	Save();
}
