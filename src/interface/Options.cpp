#include "FileZilla.h"
#include "Options.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"

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
	"Limit local ports", number, _T("0"),
	"Limit ports low", number, _T("6000"),
	"Limit ports high", number, _T("7000"),
	"External IP", string, _T(""),
	"Number of Transfers", number, _T("2"),
	"Transfer Retry Count", number, _T("5")
};

COptions::COptions()
{
	m_acquired = false;
	
	for (unsigned int i = 0; i < OPTIONS_NUM; i++)
		m_optionsCache[i].cached = false;

	wxFileName file = wxFileName(wxGetApp().GetSettingsDir(), _T("filezilla.xml"));
	m_pXmlDocument = GetXmlFile(file)->GetDocument();

	if (!m_pXmlDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", make sure the file is valid.\nFor this session, default settings will be used and any changes to the settings, including changes done in the Site Manager, are not persistent."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);
	}
	else
		CreateSettingsXmlElement();
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

	if (m_pXmlDocument)
	{
		SetXmlValue(nID, wxString::Format(_T("%d"), value));
		wxFileName file = wxFileName(wxGetApp().GetSettingsDir(), _T("filezilla.xml"));
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

	if (m_pXmlDocument)
	{
		SetXmlValue(nID, value);
		wxFileName file = wxFileName(wxGetApp().GetSettingsDir(), _T("filezilla.xml"));
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
	}

	return true;
}

void COptions::CreateSettingsXmlElement()
{
	if (!m_pXmlDocument)
		return;

	if (m_pXmlDocument->FirstChildElement("FileZilla3")->FirstChildElement("Settings"))
		return;	

	TiXmlElement *element = m_pXmlDocument->FirstChildElement("FileZilla3");
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

	wxFileName file = wxFileName(wxGetApp().GetSettingsDir(), _T("filezilla.xml"));
	m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
}

void COptions::SetXmlValue(unsigned int nID, wxString value)
{
	wxASSERT(m_pXmlDocument);

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
	wxASSERT(m_pXmlDocument);

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

int COptions::Validate(unsigned int nID, int value)
{
	return value;
}

wxString COptions::Validate(unsigned int nID, wxString value)
{
	return value;
}

TiXmlElement *COptions::GetXml()
{
	if (m_acquired)
		return 0;

	if (!m_pXmlDocument)
		return 0;
	
	return m_pXmlDocument->FirstChildElement("FileZilla3");
}

void COptions::FreeXml(bool save)
{
	if (!m_pXmlDocument)
		return;

	m_acquired = false;
	
	if (save)
	{
		wxFileName file = wxFileName(wxGetApp().GetSettingsDir(), _T("filezilla.xml"));
		m_pXmlDocument->SaveFile(file.GetFullPath().mb_str());
	}
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
	
	::SetServer(element, server);
	
	FreeXml(true);
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
	
	bool res = ::GetServer(element, server);
	
	FreeXml(false);
	
	return res;
}
