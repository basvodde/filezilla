#include "FileZilla.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"

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
	"Use Pasv mode", number, _T("1")
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
	int numValue = 0;
	if (!GetXmlValue(nID, value))
		options[nID].defaultValue.ToLong((long *)&numValue);
	else
	{
		value.ToLong((long *)&numValue);
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
			options[i].defaultValue.ToLong((long *)&m_optionsCache[i].numValue);
		SetXmlValue(i, options[i].defaultValue);
	}
}

void COptions::SetXmlValue(unsigned int nID, wxString value)
{
	// No checks are made about the validity of the value, that's done in SetOption
	
	char *utf8 = ConvUTF8(value);

	TiXmlElement *settings = m_pXmlDocument->FirstChildElement("FileZilla3")->FirstChildElement("Settings");
	if (!settings)
	{
		TiXmlNode *node = m_pXmlDocument->FirstChildElement("FileZilla3")->InsertEndChild(TiXmlElement("Settings"));
		if (!node)
			return;
		settings = node->ToElement();
		if (!settings)
			return;
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
	
	element = TiXmlElement("Host");
	element.InsertEndChild(TiXmlText(ConvUTF8(server.GetHost())));
	node->InsertEndChild(element);

	element = TiXmlElement("Port");
	element.InsertEndChild(TiXmlText(ConvUTF8(wxString::Format(_T("%d"), server.GetPort()))));
	node->InsertEndChild(element);
	
	element = TiXmlElement("Protocol");
	element.InsertEndChild(TiXmlText(ConvUTF8(wxString::Format(_T("%d"), server.GetProtocol()))));
	node->InsertEndChild(element);
	
	element = TiXmlElement("Type");
	element.InsertEndChild(TiXmlText(ConvUTF8(wxString::Format(_T("%d"), server.GetType()))));
	node->InsertEndChild(element);
	
	element = TiXmlElement("Logontype");
	element.InsertEndChild(TiXmlText(ConvUTF8(wxString::Format(_T("%d"), server.GetLogonType()))));
	node->InsertEndChild(element);
	
	if (server.GetLogonType() != ANONYMOUS)
	{
		element = TiXmlElement("User");
		element.InsertEndChild(TiXmlText(ConvUTF8(server.GetUser())));
		node->InsertEndChild(element);

		element = TiXmlElement("Pass");
		element.InsertEndChild(TiXmlText(ConvUTF8(server.GetPass())));
		node->InsertEndChild(element);
	}
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
	
	text = handle.FirstChildElement("Protocol").FirstChild().Text();
	if (!text)
		return false;
	long protocol = 0;
	if (!ConvLocal(text->Value()).ToLong(&protocol))
		return false;
	
	text = handle.FirstChildElement("Type").FirstChild().Text();
	if (!text)
		return false;
	long type = 0;
	if (!ConvLocal(text->Value()).ToLong(&type))
		return false;
	
	text = handle.FirstChildElement("Logontype").FirstChild().Text();
	if (!text)
		return false;
	long logonType = 0;
	if (!ConvLocal(text->Value()).ToLong(&logonType))
		return false;
	
	if ((long)ANONYMOUS != logonType)
	{
		text = handle.FirstChildElement("User").FirstChild().Text();
		if (!text)
			return false;
		wxString user = ConvLocal(text->Value());
		if (!user)
			return false;
	
		text = handle.FirstChildElement("Pass").FirstChild().Text();
		wxString pass;
		if (text)
			pass = ConvLocal(text->Value());
		
		server = CServer((enum ServerProtocol)protocol, (enum ServerType)type, host, port, user, pass);
	}
	else
		server = CServer((enum ServerProtocol)protocol, (enum ServerType)type, host, port);
	
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
		TiXmlElement *newElement = element->FirstChildElement(ConvUTF8(sub));
		if (newElement)
			element = newElement;
		else
		{
			TiXmlNode *node = element->InsertEndChild(TiXmlElement(ConvUTF8(sub)));
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
		element = element->FirstChildElement(ConvUTF8(sub));
		if (!element)
			return false;
	}
	
	bool res = GetServer(element, server);
	
	FreeXml();
	
	return res;
}
