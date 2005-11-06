#include "FileZilla.h"
#include "xmlfunctions.h"

char* ConvUTF8(const wxString& value)
{
	// First convert the string into unicode if neccessary.
	const wxWCharBuffer buffer = wxConvCurrent->cWX2WC(value);

	// Calculate utf-8 string length
	wxMBConvUTF8 conv;
	int len = conv.WC2MB(0, buffer, 0);

	// Not convert the string
	char *utf8 = new char[len + 1];
	conv.WC2MB(utf8, buffer, len + 1);

	return utf8;
}

wxString ConvLocal(const char *value)
{
	return wxString(wxConvUTF8.cMB2WC(value), *wxConvCurrent);
}

void AddTextElement(TiXmlElement* node, const char* name, const wxString& value)
{
	wxASSERT(node);

	TiXmlElement element(name);

	char* utf8 = ConvUTF8(value);
	if (!utf8)
		return;
	
    element.InsertEndChild(TiXmlText(utf8));
	delete [] utf8;

	node->InsertEndChild(element);
}

wxString GetTextElement(TiXmlElement* node, const char* name)
{
	wxASSERT(node);

	TiXmlElement* element = node->FirstChildElement(name);
	if (!element)
		return _T("");

	TiXmlNode* textNode = element->FirstChild();
	if (!textNode || !textNode->ToText())
		return _T("");

	return ConvLocal(textNode->Value());
}

int GetTextElementInt(TiXmlElement* node, const char* name, int defValue /*=0*/)
{
	wxASSERT(node);

	TiXmlElement* element = node->FirstChildElement(name);
	if (!element)
		return defValue;

	TiXmlNode* textNode = element->FirstChild();
	if (!textNode || !textNode->ToText())
		return defValue;

	const char* str = textNode->Value();
	const char* p = str;

	int value = 0;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	while (*p)
	{
		if (*p < '0' || *p > '9')
			return defValue;

		value *= 10;
		value += *p - '0';

		p++;
	}

	return negative ? -value : value;
}

wxLongLong GetTextElementLongLong(TiXmlElement* node, const char* name, int defValue /*=0*/)
{
	wxASSERT(node);

	TiXmlElement* element = node->FirstChildElement(name);
	if (!element)
		return defValue;

	TiXmlNode* textNode = element->FirstChild();
	if (!textNode || !textNode->ToText())
		return defValue;

	const char* str = textNode->Value();
	const char* p = str;

	wxLongLong value = 0;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	while (*p)
	{
		if (*p < '0' || *p > '9')
			return defValue;

		value *= 10;
		value += *p - '0';

		p++;
	}

	return negative ? -value : value;
}

// Opens the specified XML file if it exists or creates a new one otherwise.
// Returns 0 on error.
TiXmlElement* GetXmlFile(wxFileName file)
{
	if (wxFileExists(file.GetFullPath()))
	{
		// File does exist, open it

		TiXmlDocument* pXmlDocument = new TiXmlDocument();
		if (!pXmlDocument->LoadFile(file.GetFullPath().mb_str()))
		{
			delete pXmlDocument;
			return 0;
		}
		if (!pXmlDocument->FirstChildElement("FileZilla3"))
		{
			delete pXmlDocument;
			return 0;
		}

		TiXmlElement* pElement = pXmlDocument->FirstChildElement("FileZilla3");
		if (!pElement)
		{
			delete pXmlDocument;
			return 0;
		}
		else
			return pElement;
	}
	else
	{
		// File does not exist, create new XML document

		TiXmlDocument* pXmlDocument = new TiXmlDocument();
		pXmlDocument->InsertEndChild(TiXmlDeclaration("1.0", "UTF-8", "yes"));
	
		TiXmlElement element("FileZilla3");
		pXmlDocument->InsertEndChild(TiXmlElement("FileZilla3"));

		pXmlDocument->SaveFile(file.GetFullPath().mb_str());
		
		return pXmlDocument->FirstChildElement("FileZilla3");
	}
}

bool SaveXmlFile(const wxFileName& file, TiXmlNode* node, wxString* error /*=0*/)
{
	if (!node)
		return true;

	TiXmlDocument* pDocument = node->GetDocument();

	bool exists = false;
	if (wxFileExists(file.GetFullPath()))
	{
		exists = true;
		if (!wxCopyFile(file.GetFullPath(), file.GetFullPath() + _T("~")))
		{
			const wxString msg = _("Failed to create backup copy of xml file");
			if (error)
				*error = msg;
			else
				wxMessageBox(msg);
			return false;
		}
	}

	if (!pDocument->SaveFile(file.GetFullPath().mb_str()))
	{
		const wxString msg = _("Failed to write xml file");
		if (error)
			*error = msg;
		else
			wxMessageBox(msg);
		return false;
	}

	if (exists)
		wxRemoveFile(file.GetFullPath() + _T("~"));

	return true;
}

bool GetServer(TiXmlElement *node, CServer& server)
{
	wxASSERT(node);
	
	wxString host = GetTextElement(node, "Host");
	if (host == _T(""))
		return false;

	int port = GetTextElementInt(node, "Port");
	if (port < 1 || port > 65535)
		return false;

	if (!server.SetHost(host, port))
		return false;
	
	int protocol = GetTextElementInt(node, "Protocol");
	if (protocol < 0)
		return false;

	server.SetProtocol((enum ServerProtocol)protocol);
	
	int type = GetTextElementInt(node, "Type");
	if (type < 0)
		return false;

	server.SetType((enum ServerType)type);

	int logonType = GetTextElementInt(node, "Logontype");
	if (logonType < 0)
		return false;

	server.SetLogonType((enum LogonType)logonType);
	
	if (server.GetLogonType() != ANONYMOUS)
	{
		wxString user = GetTextElement(node, "User");
		if (user == _T(""))
			return false;
	
		wxString pass;
		if ((long)NORMAL == logonType || (long)ACCOUNT == logonType)
			pass = GetTextElement(node, "Pass");

		if (!server.SetUser(user, pass))
			return false;
		
		if ((long)ACCOUNT == logonType)
		{
			wxString account = GetTextElement(node, "Account");
			if (account == _T(""))
				return false;
			if (!server.SetAccount(account))
				return false;
		}
	}

	int timezoneOffset = GetTextElementInt(node, "TimezoneOffset");
	if (!server.SetTimezoneOffset(timezoneOffset))
		return false;
	
	wxString pasvMode = GetTextElement(node, "PasvMode");
	if (pasvMode == _T("MODE_PASSIVE"))
		server.SetPasvMode(MODE_PASSIVE);
	else if (pasvMode == _T("MODE_ACTIVE"))
		server.SetPasvMode(MODE_ACTIVE);
	else
		server.SetPasvMode(MODE_DEFAULT);
	
	int maximumMultipleConnections = GetTextElementInt(node, "MaximumMultipleConnections");
	server.MaximumMultipleConnections(maximumMultipleConnections);

	wxString encodingType = GetTextElement(node, "EncodingType");
	if (encodingType == _T("Auto"))
		server.SetEncodingType(ENCODING_AUTO);
	else if (encodingType == _T("UTF-8"))
		server.SetEncodingType(ENCODING_UTF8);
	else if (encodingType == _T("Custom"))
	{
		wxString customEncoding = GetTextElement(node, "CustomEncoding");
		if (customEncoding == _T(""))
			return false;
		if (!server.SetEncodingType(ENCODING_CUSTOM, customEncoding))
			return false;
	}
	else
		server.SetEncodingType(ENCODING_AUTO);
	
	return true;
}

void SetServer(TiXmlElement *node, const CServer& server)
{
	if (!node)
		return;
	
	node->Clear();
	
	AddTextElement(node, "Host", server.GetHost());
	AddTextElement(node, "Port", wxString::Format(_T("%d"), server.GetPort()));
	AddTextElement(node, "Protocol", wxString::Format(_T("%d"), server.GetProtocol()));
	AddTextElement(node, "Type", wxString::Format(_T("%d"), server.GetType()));
	AddTextElement(node, "Logontype", wxString::Format(_T("%d"), server.GetLogonType()));
	
	if (server.GetLogonType() != ANONYMOUS)
	{
		AddTextElement(node, "User", server.GetUser());

		if (server.GetLogonType() == NORMAL || server.GetLogonType() == ACCOUNT)
			AddTextElement(node, "Pass", server.GetPass());
		
		if (server.GetLogonType() == ACCOUNT)
			AddTextElement(node, "Account", server.GetAccount());
	}

	AddTextElement(node, "TimezoneOffset", wxString::Format(_T("%d"), server.GetTimezoneOffset()));
	switch (server.GetPasvMode())
	{
	case MODE_PASSIVE:
		AddTextElement(node, "PasvMode", _T("MODE_PASSIVE"));
		break;
	case MODE_ACTIVE:
		AddTextElement(node, "PasvMode", _T("MODE_ACTIVE"));
		break;
	default:
		AddTextElement(node, "PasvMode", _T("MODE_DEFAULT"));
		break;
	}
	AddTextElement(node, "MaximumMultipleConnections", wxString::Format(_T("%d"), server.MaximumMultipleConnections()));

	switch (server.GetEncodingType())
	{
	case ENCODING_AUTO:
		AddTextElement(node, "EncodingType", _T("Auto"));
		break;
	case ENCODING_UTF8:
		AddTextElement(node, "EncodingType", _T("UTF-8"));
		break;
	case ENCODING_CUSTOM:
		AddTextElement(node, "EncodingType", _T("Custom"));
		AddTextElement(node, "CustomEncoding", server.GetCustomEncoding());
		break;
	}
}

void SetTextAttribute(TiXmlElement* node, const char* name, const wxString& value)
{
	wxASSERT(node);

	char* utf8 = ConvUTF8(value);
	if (!utf8)
		return;
	
    node->SetAttribute(name, utf8);
	delete [] utf8;
}

wxString GetTextAttribute(TiXmlElement* node, const char* name)
{
	wxASSERT(node);

	TiXmlElement* element = node->FirstChildElement(name);
	if (!element)
		return _T("");

	const char* value = node->Attribute(name);
	if (!value)
		return _T("");
	
	return ConvLocal(value);
}
