#include "FileZilla.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	"Use Pasv mode", number, _T("1"),
	"Number of Transfers", number, _T("2"),
	"Transfer Retry Count", number, _T("5")
};

COptions::COptions()
{
	m_acquired = false;
	
	for (unsigned int i = 0; i < OPTIONS_NUM; i++)
		m_optionsCache[i].cached = false;

	m_pXmlDocument = 0;

	extern wxString dataPath;
	wxFileName file = wxFileName(dataPath, _T("filezilla.xml"));
	if (wxFileExists(file.GetFullPath()))
	{
		m_pXmlDocument = new TiXmlDocument();
		if (!m_pXmlDocument->LoadFile(file.GetFullPath().mb_str()))
		{
			wxString msg = wxString::Format(_("Could not load \"%s\", make sure the file is valid.\nFor this session, default settings will be used and any changes to the settings are not persistent."), file.GetFullPath().c_str());
			wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);
			m_allowSave = false;
			return;
		}
		if (!m_pXmlDocument->FirstChildElement("FileZilla3"))
		{
			wxString msg = wxString::Format(_("Could not load \"%s\", make sure the file is valid.\nFor this session, default settings will be used and any changes to the settings are not persistent."), file.GetFullPath().c_str());
			wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);
			m_allowSave = false;
			return;
		}
		m_allowSave = true;
	}
	else
	{
		CreateNewXmlDocument();
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
		m_allowSave = true;
	}
}

COptions::~COptions()
{
	delete m_pXmlDocument;
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
		Validate(nID, numValue);
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
		return _T("");

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
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != number)
		return false;

	Validate(nID, value);

	m_optionsCache[nID].cached = true;
	m_optionsCache[nID].numValue = value;

	SetXmlValue(nID, wxString::Format(_T("%d"), value));
	if (m_allowSave)
	{
		extern wxString dataPath;
		wxFileName file = wxFileName(dataPath, _T("filezilla.xml"));
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
	}

	return true;
}

bool COptions::SetOption(unsigned int nID, wxString value)
{
	if (nID >= OPTIONS_NUM)
		return false;

	if (options[nID].type != string)
		return false;

	Validate(nID, value);

	m_optionsCache[nID].cached = true;
	m_optionsCache[nID].strValue = value;

	SetXmlValue(nID, value);
	if (m_allowSave)
	{
		extern wxString dataPath;
		wxFileName file = wxFileName(dataPath, _T("filezilla.xml"));
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
	}

	return true;
}

void COptions::CreateNewXmlDocument()
{
	delete m_pXmlDocument;
	m_pXmlDocument = new TiXmlDocument();
	m_pXmlDocument->InsertEndChild(TiXmlDeclaration("1.0", "UTF-8", "yes"));
	
	TiXmlElement element("FileZilla3");
	TiXmlElement settings("Settings");
	element.InsertEndChild(settings);
	m_pXmlDocument->InsertEndChild(element);

	for (int i = 0; i < OPTIONS_NUM; i++)
	{
		m_optionsCache[i].cached = true;
		if (options[i].type == string)
			m_optionsCache[i].strValue = options[i].defaultValue;
		else
			options[i].defaultValue.ToLong(&m_optionsCache[i].numValue);
		SetXmlValue(i, options[i].defaultValue);
	}
}

void COptions::SetXmlValue(unsigned int nID, wxString value)
{
	// No checks are made about the validity of the value, that's done in SetOption
	
	char *utf8 = ConvUTF8(value);
	if (!utf8)
		return;

	TiXmlElement *settings = m_pXmlDocument->FirstChildElement("FileZilla3")->FirstChildElement("Settings");
	if (!settings)
	{
		TiXmlNode *node = m_pXmlDocument->FirstChildElement("FileZilla3")->InsertEndChild(TiXmlElement("Settings"));
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

bool COptions::GetXmlValue(unsigned int nID, wxString &value)
{
	TiXmlElement *settings = m_pXmlDocument->FirstChildElement("FileZilla3")->FirstChildElement("Settings");
	if (!settings)
	{
		TiXmlNode *node = m_pXmlDocument->FirstChildElement("FileZilla3")->InsertEndChild(TiXmlElement("Settings"));
		if (!node)
			return false;
		settings = node->ToElement();
		if (!settings)
			return false;
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

void COptions::Validate(unsigned int nID, int &value)
{
}

void COptions::Validate(unsigned int nID, wxString &value)
{
}

TiXmlElement *COptions::GetXml()
{
	if (m_acquired)
		return 0;
	
	return m_pXmlDocument->FirstChildElement("FileZilla3");
}

void COptions::FreeXml()
{
	m_acquired = false;
	
	if (m_allowSave)
	{
		extern wxString dataPath;
		wxFileName file = wxFileName(dataPath, _T("filezilla.xml"));
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
	}
}

void COptions::SetServer(TiXmlElement *node, const CServer& server)
{
	if (!node)
		return;
	
	node->Clear();
	
	TiXmlElement element("");

	char *utf8;
	
	element = TiXmlElement("Host");
	utf8 = ConvUTF8(server.GetHost());
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);

	element = TiXmlElement("Port");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.GetPort()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);
	
	element = TiXmlElement("Protocol");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.GetProtocol()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);
	
	element = TiXmlElement("Type");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.GetType()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);
	
	element = TiXmlElement("Logontype");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.GetLogonType()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);
	
	if (server.GetLogonType() != ANONYMOUS)
	{
		element = TiXmlElement("User");
		utf8 = ConvUTF8(server.GetUser());
		if (!utf8)
			return;
		element.InsertEndChild(TiXmlText(utf8));
		delete [] utf8;
		node->InsertEndChild(element);

		if (server.GetLogonType() == NORMAL)
		{
			element = TiXmlElement("Pass");
			utf8 = ConvUTF8(server.GetPass());
			if (!utf8)
				return;
			element.InsertEndChild(TiXmlText(utf8));
			delete [] utf8;
			node->InsertEndChild(element);
		}
	}

	element = TiXmlElement("TimezoneOffset");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.GetTimezoneOffset()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);

	element = TiXmlElement("PasvMode");
	switch (server.GetPasvMode())
	{
	case MODE_PASSIVE:
		utf8 = ConvUTF8(_T("MODE_PASSIVE"));
		break;
	case MODE_ACTIVE:
		utf8 = ConvUTF8(_T("MODE_ACTIVE"));
		break;
	default:
		utf8 = ConvUTF8(_T("MODE_DEFAULT"));
		break;
	}
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);

	element = TiXmlElement("MaximumMultipleConnections");
	utf8 = ConvUTF8(wxString::Format(_T("%d"), server.MaximumMultipleConnections()));
	if (!utf8)
		return;
	element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;
	node->InsertEndChild(element);
}

bool COptions::GetServer(TiXmlElement *node, CServer& server)
{
	if (!node)
		return false;
	
	TiXmlHandle handle(node);
	
	TiXmlText *text;
		
	text = handle.FirstChildElement("Host").FirstChild().Text();
	if (!text)
		return false;
	wxString host = ConvLocal(text->Value());
	if (host == _T(""))
		return false;
	
	text = handle.FirstChildElement("Port").FirstChild().Text();
	if (!text)
		return false;
	long port = 0;
	if (!ConvLocal(text->Value()).ToLong(&port))
		return false;
	if (port < 1 || port > 65535)
		return false;

	if (!server.SetHost(host, port))
		return false;
	
	text = handle.FirstChildElement("Protocol").FirstChild().Text();
	if (!text)
		return false;
	long protocol = 0;
	if (!ConvLocal(text->Value()).ToLong(&protocol))
		return false;

	server.SetProtocol((enum ServerProtocol)protocol);
	
	text = handle.FirstChildElement("Type").FirstChild().Text();
	if (!text)
		return false;
	long type = 0;
	if (!ConvLocal(text->Value()).ToLong(&type))
		return false;

	server.SetType((enum ServerType)type);
	
	text = handle.FirstChildElement("Logontype").FirstChild().Text();
	if (!text)
		return false;
	long logonType = 0;
	if (!ConvLocal(text->Value()).ToLong(&logonType))
		return false;

	server.SetLogonType((enum LogonType)logonType);
	
	if (server.GetLogonType() != ANONYMOUS)
	{
		text = handle.FirstChildElement("User").FirstChild().Text();
		if (!text)
			return false;
		wxString user = ConvLocal(text->Value());
		if (!user)
			return false;
	
		wxString pass;
		if ((long)NORMAL == logonType)
		{
			text = handle.FirstChildElement("Pass").FirstChild().Text();
			if (text)
				pass = ConvLocal(text->Value());
		}
		
		if (!server.SetUser(user, pass))
			return false;
	}

	text = handle.FirstChildElement("TimezoneOffset").FirstChild().Text();
	if (text)
	{
		long timezoneOffset = 0;
		if (!ConvLocal(text->Value()).ToLong(&timezoneOffset))
			return false;
		if (!server.SetTimezoneOffset(timezoneOffset))
			return false;
	}
	
	text = handle.FirstChildElement("PasvMode").FirstChild().Text();
	if (text)
	{
		wxString pasvMode = ConvLocal(text->Value());
		if (pasvMode == _T("MODE_PASSIVE"))
			server.SetPasvMode(MODE_PASSIVE);
		else if (pasvMode == _T("MODE_ACTIVE"))
			server.SetPasvMode(MODE_ACTIVE);
		else
			server.SetPasvMode(MODE_DEFAULT);
	}
	
	text = handle.FirstChildElement("MaximumMultipleConnections").FirstChild().Text();
	if (text)
	{
		unsigned long maximumMultipleConnections = 0;
		if (ConvLocal(text->Value()).ToULong(&maximumMultipleConnections) && maximumMultipleConnections <= 10)
			server.MaximumMultipleConnections(maximumMultipleConnections);
	}
	
	return true;
}

char* COptions::ConvUTF8(wxString value) const
{
	const wxWCharBuffer buffer = wxConvCurrent->cWX2WC(value);

	wxMBConvUTF8 conv;
	int len = conv.WC2MB(0, buffer, 0);
	char *utf8 = new char[len + 1];
	conv.WC2MB(utf8, buffer, len + 1);
	return utf8;
}

wxString COptions::ConvLocal(const char *value) const
{
	return wxString(wxConvUTF8.cMB2WC(value), *wxConvCurrent);
}

void COptions::SetServer(wxString path, const CServer& server)
{
	if (path == _T(""))
		return;
	
	TiXmlElement *element = GetXml();
	
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
	
	SetServer(element, server);
	
	FreeXml();
}

bool COptions::GetServer(wxString path, CServer& server)
{
	if (path == _T(""))
		return false;
	
	TiXmlElement *element = GetXml();
	
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
	
	bool res = GetServer(element, server);
	
	FreeXml();
	
	return res;
}
