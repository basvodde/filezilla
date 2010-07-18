#include <filezilla.h>
#include "Options.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"
#include <wx/tokenzr.h>
#include "ipcmutex.h"
#include "option_change_event_handler.h"
#include "sizeformatting.h"

#ifdef __WXMSW__
	#include <shlobj.h>

	// Needed for MinGW:
	#ifndef SHGFP_TYPE_CURRENT
		#define SHGFP_TYPE_CURRENT 0
	#endif
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

COptions* COptions::m_theOptions = 0;

enum Type
{
	string,
	number
};

enum Flags
{
	normal,
	internal,
	default_only
};

struct t_Option
{
	const char name[30];
	const enum Type type;
	const wxString defaultValue; // Default values are stored as string even for numerical options
	const Flags flags; // internal items won't get written to settings file nor loaded from there
};

static const t_Option options[OPTIONS_NUM] =
{
	// Note: A few options are versioned due to a changed
	// option syntax or past, unhealthy defaults

	// Engine settings
	{ "Use Pasv mode", number, _T("1"), normal },
	{ "Limit local ports", number, _T("0"), normal },
	{ "Limit ports low", number, _T("6000"), normal },
	{ "Limit ports high", number, _T("7000"), normal },
	{ "External IP mode", number, _T("0"), normal },
	{ "External IP", string, _T(""), normal },
	{ "External address resolver", string, _T("http://ip.filezilla-project.org/ip.php"), normal },
	{ "Last resolved IP", string, _T(""), normal },
	{ "No external ip on local conn", number, _T("1"), normal },
	{ "Pasv reply fallback mode", number, _T("0"), normal },
	{ "Timeout", number, _T("20"), normal },
	{ "Logging Debug Level", number, _T("0"), normal },
	{ "Logging Raw Listing", number, _T("0"), normal },
	{ "fzsftp executable", string, _T(""), internal },
	{ "Allow transfermode fallback", number, _T("1"), normal },
	{ "Reconnect count", number, _T("2"), normal },
	{ "Reconnect delay", number, _T("5"), normal },
	{ "Enable speed limits", number, _T("0"), normal },
	{ "Speedlimit inbound", number, _T("100"), normal },
	{ "Speedlimit outbound", number, _T("20"), normal },
	{ "Speedlimit burst tolerance", number, _T("0"), normal },
	{ "View hidden files", number, _T("0"), normal },
	{ "Preserve timestamps", number, _T("0"), normal },
	{ "Socket recv buffer size (v2)", number, _T("4194304"), normal }, // Make it large enough by default
														 // to enable a large TCP window scale
	{ "Socket send buffer size (v2)", number, _T("262144"), normal },
	{ "FTP Keep-alive commands", number, _T("0"), normal },
	{ "FTP Proxy type", number, _T("0"), normal },
	{ "FTP Proxy host", string, _T(""), normal },
	{ "FTP Proxy user", string, _T(""), normal },
	{ "FTP Proxy password", string, _T(""), normal },
	{ "FTP Proxy login sequence", string, _T(""), normal },
	{ "SFTP keyfiles", string, _T(""), normal },
	{ "Proxy type", number, _T("0"), normal },
	{ "Proxy host", string, _T(""), normal },
	{ "Proxy port", number, _T("0"), normal },
	{ "Proxy user", string, _T(""), normal },
	{ "Proxy password", string, _T(""), normal },
	{ "Logging file", string, _T(""), normal },
	{ "Logging filesize limit", number, _T("10"), normal },
	{ "Trusted root certificate", string, _T(""), internal },
	{ "Size format", number, _T("0"), normal },
	{ "Size thousands separator", number, _T("1"), normal },
	{ "Size decimal places", number, _T("0"), normal },

	// Interface settings
	{ "Number of Transfers", number, _T("2"), normal },
	{ "Ascii Binary mode", number, _T("0"), normal },
	{ "Auto Ascii files", string, _T("am|asp|bat|c|cfm|cgi|conf|cpp|css|dhtml|diz|h|hpp|htm|html|in|inc|js|jsp|m4|mak|md5|nfo|nsi|pas|patch|php|phtml|pl|po|py|qmail|sh|shtml|sql|svg|tcl|tpl|txt|vbs|xhtml|xml|xrc"), normal },
	{ "Auto Ascii no extension", number, _T("1"), normal },
	{ "Auto Ascii dotfiles", number, _T("1"), normal },
	{ "Theme", string, _T("opencrystal/"), normal },
	{ "Language Code", string, _T(""), normal },
	{ "Last Server Path", string, _T(""), normal },
	{ "Concurrent download limit", number, _T("0"), normal },
	{ "Concurrent upload limit", number, _T("0"), normal },
	{ "Update Check", number, _T("1"), normal },
	{ "Update Check Interval", number, _T("7"), normal },
	{ "Last automatic update check", string, _T(""), normal },
	{ "Update Check New Version", string, _T(""), normal },
	{ "Update Check Check Beta", number, _T("0"), normal },
	{ "Update Check Download Dir", string, _T(""), normal },
	{ "Show debug menu", number, _T("0"), normal },
	{ "File exists action download", number, _T("0"), normal },
	{ "File exists action upload", number, _T("0"), normal },
	{ "Allow ascii resume", number, _T("0"), normal },
	{ "Greeting version", string, _T(""), normal },
	{ "Onetime Dialogs", string, _T(""), normal },
	{ "Show Tree Local", number, _T("1"), normal },
	{ "Show Tree Remote", number, _T("1"), normal },
	{ "File Pane Layout", number, _T("0"), normal },
	{ "File Pane Swap", number, _T("0"), normal },
	{ "Last local directory", string, _T(""), normal },
	{ "Filelist directory sort", number, _T("0"), normal },
	{ "Queue successful autoclear", number, _T("0"), normal },
	{ "Queue column widths", string, _T(""), normal },
	{ "Local filelist colwidths", string, _T(""), normal },
	{ "Remote filelist colwidths", string, _T(""), normal },
	{ "Window position and size", string, _T(""), normal },
	{ "Splitter positions (v2)", string, _T(""), normal },
	{ "Local filelist sortorder", string, _T(""), normal },
	{ "Remote filelist sortorder", string, _T(""), normal },
	{ "Time Format", string, _T(""), normal },
	{ "Date Format", string, _T(""), normal },
	{ "Show message log", number, _T("1"), normal },
	{ "Show queue", number, _T("1"), normal },
	{ "Default editor", string, _T(""), normal },
	{ "Always use default editor", number, _T("0"), normal },
	{ "Inherit system associations", number, _T("1"), normal },
	{ "Custom file associations", string, _T(""), normal },
	{ "Comparison mode", number, _T("1"), normal },
	{ "Comparison threshold", number, _T("1"), normal },
	{ "Site Manager position", string, _T(""), normal },
	{ "Theme icon size", string, _T(""), normal },
	{ "Timestamp in message log", number, _T("0"), normal },
	{ "Sitemanager last selected", string, _T(""), normal },
	{ "Local filelist shown columns", string, _T(""), normal },
	{ "Remote filelist shown columns", string, _T(""), normal },
	{ "Local filelist column order", string, _T(""), normal },
	{ "Remote filelist column order", string, _T(""), normal },
	{ "Filelist status bar", number, _T("1"), normal },
	{ "Filter toggle state", number, _T("0"), normal },
	{ "Show quickconnect bar", number, _T("1"), normal },
	{ "Messagelog position", number, _T("0"), normal },
	{ "Last connected site", string, _T(""), normal },
	{ "File doubleclock action", number, _T("0"), normal },
	{ "Dir doubleclock action", number, _T("0"), normal },
	{ "Minimize to tray", number, _T("0"), normal },
	{ "Search column widths", string, _T(""), normal },
	{ "Search column shown", string, _T(""), normal },
	{ "Search column order", string, _T(""), normal },
	{ "Search window size", string, _T(""), normal },
	{ "Comparison hide identical", number, _T("0"), normal },
	{ "Search sort order", string, _T(""), normal },
	{ "Edit track local", number, _T("1"), normal },
	{ "Prevent idle sleep", number, _T("1"), normal },
	{ "Filteredit window size", string, _T(""), normal },
	{ "Enable invalid char filter", number, _T("1"), normal },
	{ "Invalid char replace", string, _T("_"), normal },
	{ "Already connected choice", number, _T("0"), normal },	
	{ "Edit status dialog size", string, _T(""), normal },
	{ "Display current speed", number, _T("0"), normal },
	{ "Toolbar hidden", number, _T("0"), normal },

	// Default/internal options
	{ "Config Location", string, _T(""), default_only },
	{ "Kiosk mode", number, _T("0"), default_only },
	{ "Disable update check", number, _T("0"), default_only }
};

BEGIN_EVENT_TABLE(COptions, wxEvtHandler)
EVT_TIMER(wxID_ANY, COptions::OnTimer)
END_EVENT_TABLE()

COptions::COptions()
{
	m_theOptions = this;

	m_pLastServer = 0;

	m_acquired = false;

	SetDefaultValues();

	m_save_timer.SetOwner(this);

	std::map<std::string, int> nameOptionMap;
	GetNameOptionMap(nameOptionMap);

	LoadGlobalDefaultOptions(nameOptionMap);

	InitSettingsDir();

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

	LoadOptions(nameOptionMap);
}

void COptions::GetNameOptionMap(std::map<std::string, int>& nameOptionMap) const
{
	for (int i = 0; i < OPTIONS_NUM; ++i)
	{
		if (options[i].flags != internal)
			nameOptionMap.insert(std::make_pair(options[i].name, i));
	}
}

COptions::~COptions()
{
	COptionChangeEventHandler::UnregisterAll();

	delete m_pLastServer;
	delete m_pXmlFile;
}

int COptions::GetOptionVal(unsigned int nID)
{
	if (nID >= OPTIONS_NUM)
		return 0;

	if (options[nID].type != number)
		return 0;

	return m_optionsCache[nID].numValue;
}

wxString COptions::GetOption(unsigned int nID)
{
	if (nID >= OPTIONS_NUM)
		return _T("");

	if (options[nID].type != string)
		return wxString::Format(_T("%d"), GetOptionVal(nID));

	return m_optionsCache[nID].strValue;
}

bool COptions::SetOption(unsigned int nID, int value)
{
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != number)
		return false;

	value = Validate(nID, value);

	if (m_optionsCache[nID].numValue == value)
	{
		// Nothing to do
		return true;
	}

	m_optionsCache[nID].numValue = value;

	if (m_pXmlFile && options[nID].flags == normal)
	{
		SetXmlValue(nID, wxString::Format(_T("%d"), value));

		if (!m_save_timer.IsRunning())
			m_save_timer.Start(15000, true);
	}

	COptionChangeEventHandler::DoNotify(nID);

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

	if (m_optionsCache[nID].strValue == value)
	{
		// Nothing to do
		return true;
	}
	m_optionsCache[nID].strValue = value;

	if (m_pXmlFile && options[nID].flags == normal)
	{
		SetXmlValue(nID, value);

		if (!m_save_timer.IsRunning())
			m_save_timer.Start(15000, true);
	}

	COptionChangeEventHandler::DoNotify(nID);

	return true;
}

void COptions::CreateSettingsXmlElement()
{
	if (!m_pXmlFile)
		return;

	TiXmlElement *element = m_pXmlFile->GetElement();
	if (element->FirstChildElement("Settings"))
		return;

	TiXmlElement *settings = new TiXmlElement("Settings");
	element->LinkEndChild(settings);

	for (int i = 0; i < OPTIONS_NUM; i++)
	{
		if (options[i].type == string)
			SetXmlValue(i, m_optionsCache[i].strValue);
		else
		{
			wxString s(wxString::Format(_T("%d"), m_optionsCache[i].numValue));
			SetXmlValue(i, s);
		}
		
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
		TiXmlNode *node = m_pXmlFile->GetElement()->LinkEndChild(new TiXmlElement("Settings"));
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

			//setting->RemoveAttribute("type");
			setting->Clear();
			//setting->SetAttribute("type", (options[nID].type == string) ? "string" : "number");
			setting->LinkEndChild(new TiXmlText(utf8));

			delete [] utf8;
			return;
		}
	}
	wxASSERT(options[nID].name[0]);
	TiXmlElement *setting = new TiXmlElement("Setting");
	setting->SetAttribute("name", options[nID].name);
	//setting->SetAttribute("type", (options[nID].type == string) ? "string" : "number");
	setting->LinkEndChild(new TiXmlText(utf8));
	settings->LinkEndChild(setting);

	delete [] utf8;
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
	case OPTION_SIZE_FORMAT:
		if (value < 0 || value >= CSizeFormat::formats_count)
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
			TiXmlNode *node = element->LinkEndChild(new TiXmlElement(utf8));
			delete [] utf8;
			if (!node || !node->ToElement())
				return;
			element = node->ToElement();
		}
	}

	::SetServer(element, server);

	if (GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
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
		new COptions(); // It sets m_theOptions internally itself
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
	std::map<std::string, int> nameOptionMap;
	GetNameOptionMap(nameOptionMap);
	LoadOptions(nameOptionMap, pElement);
}

void COptions::LoadOptions(const std::map<std::string, int>& nameOptionMap, TiXmlElement* settings /*=0*/)
{
	if (!settings)
	{
		if (!m_pXmlFile)
			return;

		settings = m_pXmlFile->GetElement()->FirstChildElement("Settings");
		if (!settings)
		{
			TiXmlNode *node = m_pXmlFile->GetElement()->LinkEndChild(new TiXmlElement("Settings"));
			if (!node)
				return;
			settings = node->ToElement();
			if (!settings)
				return;
		}
	}

	TiXmlNode *node = 0;
	while ((node = settings->IterateChildren("Setting", node)))
	{
		TiXmlElement *setting = node->ToElement();
		if (!setting)
			continue;
		LoadOptionFromElement(setting, nameOptionMap, false);
	}
}

void COptions::LoadOptionFromElement(TiXmlElement* pOption, const std::map<std::string, int>& nameOptionMap, bool allowDefault)
{
	const char* name = pOption->Attribute("name");
	if (!name)
		return;

	std::map<std::string, int>::const_iterator iter = nameOptionMap.find(name);
	if (iter != nameOptionMap.end())
	{
		if (!allowDefault && options[iter->second].flags == default_only)
			return;

		TiXmlNode *text = pOption->FirstChild();
		if (!text)
			return;

		if (!text->ToText())
			return;

		wxString value(ConvLocal(text->Value()));

		if (options[iter->second].type == number)
		{
			long numValue = 0;
			value.ToLong(&numValue);
			numValue = Validate(iter->second, numValue);
			m_optionsCache[iter->second].numValue = numValue;
		}
		else
		{
			value = Validate(iter->second, value);
			m_optionsCache[iter->second].strValue = value;
		}
	}
}

void COptions::LoadGlobalDefaultOptions(const std::map<std::string, int>& nameOptionMap)
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
		LoadOptionFromElement(pSetting, nameOptionMap, true);
	}
}

void COptions::OnTimer(wxTimerEvent& event)
{
	Save();
}

void COptions::Save()
{
	if (GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
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

void COptions::InitSettingsDir()
{
	wxFileName fn;

	wxString dir(GetOption(OPTION_DEFAULT_SETTINGSDIR));
	if (!dir.empty())
	{
		wxStringTokenizer tokenizer(dir, _T("/\\"), wxTOKEN_RET_EMPTY_ALL);
		dir = _T("");
		while (tokenizer.HasMoreTokens())
		{
			wxString token = tokenizer.GetNextToken();
			if (token[0] == '$')
			{
				if (token[1] == '$')
					token = token.Mid(1);
				else
				{
					wxString value;
					if (wxGetEnv(token.Mid(1), &value))
						token = value;
				}
			}
			dir += token;
			const wxChar delimiter = tokenizer.GetLastDelimiter();
			if (delimiter)
				dir += delimiter;
		}

		fn = wxFileName(dir, _T(""));
		fn.Normalize(wxPATH_NORM_ALL, wxGetApp().GetDefaultsDir());
	}
	else
	{
#ifdef __WXMSW__
		wxChar buffer[MAX_PATH * 2 + 1];

		if (SUCCEEDED(SHGetFolderPath(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, buffer)))
		{
			fn = wxFileName(buffer, _T(""));
			fn.AppendDir(_T("FileZilla"));
		}
		else
		{
			// Fall back to directory where the executable is
			if (GetModuleFileName(0, buffer, MAX_PATH * 2))
				fn = buffer;
		}
#else
		fn = wxFileName(wxGetHomeDir(), _T(""));
		fn.AppendDir(_T(".filezilla"));
#endif
	}
	if (!fn.DirExists())
		wxMkdir(fn.GetPath(), 0700);
	SetOption(OPTION_DEFAULT_SETTINGSDIR, fn.GetPath());
}

void COptions::SetDefaultValues()
{
	for (int i = 0; i < OPTIONS_NUM; ++i)
	{
		if (options[i].type == string)
			m_optionsCache[i].strValue = options[i].defaultValue;
		else
		{
			long numValue = 0;
			options[i].defaultValue.ToLong(&numValue);
			m_optionsCache[i].numValue = numValue;
		}
	}
}
