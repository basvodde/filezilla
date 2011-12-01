#include <filezilla.h>
#include "xmlfunctions.h"
#include "Options.h"
#include <wx/ffile.h>
#include <wx/log.h>

#include <local_filesys.h>

CXmlFile::CXmlFile(const wxString& fileName)
{
	m_pPrinter = 0;
	SetFileName(fileName);
	m_pDocument = 0;
}

CXmlFile::CXmlFile(const wxFileName& fileName /*=wxFileName()*/)
{
	m_pPrinter = 0;
	SetFileName(fileName);
	m_pDocument = 0;
}

void CXmlFile::SetFileName(const wxString& name)
{
	m_fileName = wxFileName(COptions::Get()->GetOption(OPTION_DEFAULT_SETTINGSDIR), name + _T(".xml"));
	m_modificationTime = wxDateTime();
}

void CXmlFile::SetFileName(const wxFileName& fileName)
{
	m_fileName = fileName;
	m_modificationTime = wxDateTime();
}

CXmlFile::~CXmlFile()
{
	delete m_pPrinter;
	delete m_pDocument;
}

TiXmlElement* CXmlFile::Load(const wxString& name, bool create)
{
	wxFileName fileName(COptions::Get()->GetOption(OPTION_DEFAULT_SETTINGSDIR), name + _T(".xml"));
	return Load(fileName, create);
}

TiXmlElement* CXmlFile::Load(const wxFileName& fileName, bool create)
{
	if (fileName.IsOk())
		SetFileName(fileName);

	wxCHECK(m_fileName.IsOk(), 0);

	delete m_pDocument;
	m_pDocument = 0;

	wxString error;
	TiXmlElement* pElement = GetXmlFile(m_fileName, create, &error);
	if (!pElement)
	{
		if (!error.empty())
		{
			m_error.Printf(_("The file '%s' could not be loaded."), m_fileName.GetFullPath().c_str());
			if (!error.empty())
				m_error += _T("\n") + error;
			else
				m_error += wxString(_T("\n")) + _("Make sure the file can be accessed and is a well-formed XML document.");
			m_modificationTime = wxDateTime();
		}
		return 0;
	}

	{
		wxLogNull log;
		m_modificationTime = m_fileName.GetModificationTime();
	}

	m_pDocument = pElement->GetDocument();
	return pElement;
}

TiXmlElement* CXmlFile::GetElement()
{
	if (!m_pDocument)
		return 0;

	TiXmlElement* pElement = m_pDocument->FirstChildElement("FileZilla3");
	if (!pElement)
	{
		delete m_pDocument;
		m_pDocument = 0;
		return 0;
	}
	else
		return pElement;
}

const TiXmlElement* CXmlFile::GetElement() const
{
	if (!m_pDocument)
		return 0;

	const TiXmlElement* pElement = m_pDocument->FirstChildElement("FileZilla3");
	return pElement;
}

bool CXmlFile::Modified()
{
	wxCHECK(m_fileName.IsOk(), false);

	if (!m_modificationTime.IsValid())
		return false;

	wxLogNull log;
	wxDateTime modificationTime;
	if (!m_fileName.FileExists())
		return true;

	modificationTime = m_fileName.GetModificationTime();
	if (modificationTime.IsValid() && modificationTime == m_modificationTime)
		return false;

	return true;
}

void CXmlFile::Close()
{
	delete m_pDocument;
	m_pDocument = 0;
}

bool CXmlFile::Save(wxString* error)
{
	wxCHECK(m_fileName.IsOk(), false);

	wxCHECK(m_pDocument, false);

	bool res = SaveXmlFile(m_fileName, GetElement(), error);
	wxLogNull log;
	m_modificationTime = m_fileName.GetModificationTime();
	return res;
}

TiXmlElement* CXmlFile::CreateEmpty()
{
	delete m_pDocument;

	m_pDocument = new TiXmlDocument();
	m_pDocument->SetCondenseWhiteSpace(false);
	m_pDocument->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", "yes"));

	return m_pDocument->LinkEndChild(new TiXmlElement("FileZilla3"))->ToElement();
}

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

	char* utf8 = ConvUTF8(value);
	if (!utf8)
		return;

	TiXmlElement *element = new TiXmlElement(name);

	element->LinkEndChild(new TiXmlText(utf8));
	delete [] utf8;

	node->LinkEndChild(element);
}

void AddTextElement(TiXmlElement* node, const char* name, int value)
{
	char buffer[sizeof(int) * 8]; // Always big enough
	sprintf(buffer, "%d", value);
	AddTextElementRaw(node, name, buffer);
}

void AddTextElementRaw(TiXmlElement* node, const char* name, const char* value)
{
	wxASSERT(node);
	wxASSERT(value && *value);

	TiXmlElement *element = new TiXmlElement(name);

	element->LinkEndChild(new TiXmlText(value));

	node->LinkEndChild(element);
}

void AddTextElement(TiXmlElement* node, const wxString& value)
{
	wxASSERT(node);

	char* utf8 = ConvUTF8(value);
	if (!utf8)
		return;

	for (TiXmlNode* pChild = node->FirstChild(); pChild; pChild = pChild->NextSibling())
	{
		if (!pChild->ToText())
			continue;

		node->RemoveChild(pChild);
		break;
	}

	node->LinkEndChild(new TiXmlText(utf8));
	delete [] utf8;
}

void AddTextElement(TiXmlElement* node, int value)
{
	char buffer[sizeof(int)]; // Always big enough
	sprintf(buffer, "%d", value);
	AddTextElementRaw(node, buffer);
}

void AddTextElementRaw(TiXmlElement* node, const char* value)
{
	wxASSERT(node);
	wxASSERT(value && *value);

	for (TiXmlNode* pChild = node->FirstChild(); pChild; pChild = pChild->NextSibling())
	{
		if (!pChild->ToText())
			continue;

		node->RemoveChild(pChild);
		break;
	}

    node->LinkEndChild(new TiXmlText(value));
}

wxString GetTextElement_Trimmed(TiXmlElement* node, const char* name)
{
	wxString t = GetTextElement(node, name);
	t.Trim(true);
	t.Trim(false);

	return t;
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

wxString GetTextElement_Trimmed(TiXmlElement* node)
{
	wxString t = GetTextElement(node);
	t.Trim(true);
	t.Trim(false);

	return t;
}

wxString GetTextElement(TiXmlElement* node)
{
	wxASSERT(node);

	for (TiXmlNode* pChild = node->FirstChild(); pChild; pChild = pChild->NextSibling())
	{
		if (!pChild->ToText())
			continue;

		return ConvLocal(pChild->Value());
	}

	return _T("");
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

bool GetTextElementBool(TiXmlElement* node, const char* name, bool defValue /*=false*/)
{
	wxASSERT(node);

	TiXmlElement* element = node->FirstChildElement(name);
	if (!element)
		return defValue;

	TiXmlNode* textNode = element->FirstChild();
	if (!textNode || !textNode->ToText())
		return defValue;

	const char* str = textNode->Value();
	if (!str)
		return defValue;
	
	switch (str[0])
	{
	case '0':
		return false;
	case '1':
		return true;
	default:
		return defValue;
	}
}

bool LoadXmlDocument(TiXmlDocument* pXmlDocument, const wxString& file, wxString* error /*=0*/)
{
	wxFFile f(file, _T("rb"));
	if (!f.IsOpened())
	{
		if (error)
		{
			const wxChar* s = wxSysErrorMsg();
			if (s && *s)
				*error = s;
			else
				*error = _("Unknown error opening the file. Make sure the file can be accessed and is a well-formed XML document.");
		}
		return false;
	}

	if (!pXmlDocument->LoadFile(f.fp()))
	{
		if (pXmlDocument->ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY)
		{
			if (error)
			{
				const char* s = pXmlDocument->ErrorDesc();
				error->Printf(_("The XML document is not well-formed: %s"), wxString(s, wxConvLibc).c_str());
			}
			return false;
		}
	}
	return true;
}

// Opens the specified XML file if it exists or creates a new one otherwise.
// Returns 0 on error.
TiXmlElement* GetXmlFile(wxFileName file, bool create /*=true*/, wxString* error /*=0*/)
{
	if (wxFileExists(file.GetFullPath()) && file.GetSize() > 0)
	{
		// File does exist, open it

		TiXmlDocument* pXmlDocument = new TiXmlDocument;
		pXmlDocument->SetCondenseWhiteSpace(false);
		if (!LoadXmlDocument(pXmlDocument, file.GetFullPath(), error))
		{
			delete pXmlDocument;
			return 0;
		}

		TiXmlElement* pElement = pXmlDocument->FirstChildElement("FileZilla3");
		if (!pElement)
		{
			if (pXmlDocument->FirstChildElement())
			{
				// Not created by FileZilla3
				delete pXmlDocument;

				if (error)
					*error = _("Unknown root element, the file does not appear to be generated by FileZilla.");
				return 0;
			}
			pElement = pXmlDocument->LinkEndChild(new TiXmlElement("FileZilla3"))->ToElement();
		}

		return pElement;
	}
	else
	{
		// File does not exist
		if (!create)
			return 0;
		
		// create new XML document
		TiXmlDocument* pXmlDocument = new TiXmlDocument();
		pXmlDocument->SetCondenseWhiteSpace(false);
		pXmlDocument->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", "yes"));

		pXmlDocument->LinkEndChild(new TiXmlElement("FileZilla3"));

		if (!SaveXmlFile(file, pXmlDocument, 0))
		{
			delete pXmlDocument;
			return 0;
		}

		return pXmlDocument->FirstChildElement("FileZilla3");
	}
}

bool SaveXmlFile(const wxFileName& file, TiXmlNode* node, wxString* error /*=0*/, bool move /*=false*/)
{
	if (!node)
		return true;

	const wxString& fullPath = file.GetFullPath();

	TiXmlDocument* pDocument = node->GetDocument();

	bool exists = false;
	
	bool isLink = false;
	int flags = 0;
	if (CLocalFileSystem::GetFileInfo( fullPath, isLink, 0, 0, &flags ) == CLocalFileSystem::file)
	{
#ifdef __WXMSW__
		if (flags & FILE_ATTRIBUTE_HIDDEN)
			SetFileAttributes(fullPath, flags & ~FILE_ATTRIBUTE_HIDDEN);
#endif

		exists = true;
		bool res;
		if (!move)
		{
			wxLogNull null;
			res = wxCopyFile(fullPath, fullPath + _T("~"));
		}
		else
		{
			wxLogNull null;
			res = wxRenameFile(fullPath, fullPath + _T("~"));
		}
		if (!res)
		{
			const wxString msg = _("Failed to create backup copy of xml file");
			if (error)
				*error = msg;
			else
				wxMessageBox(msg);
			return false;
		}
	}

	wxFFile f(fullPath, _T("w"));
	if (!f.IsOpened() || !pDocument->SaveFile(f.fp()))
	{
		wxRemoveFile(fullPath);
		if (exists)
		{
			wxLogNull null;
			wxRenameFile(fullPath + _T("~"), fullPath);
		}
		const wxString msg = _("Failed to write xml file");
		if (error)
			*error = msg;
		else
			wxMessageBox(msg);
		return false;
	}

	if (exists)
		wxRemoveFile(fullPath + _T("~"));

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
	if (type < 0 || type >= SERVERTYPE_MAX)
		return false;

	server.SetType((enum ServerType)type);

	int logonType = GetTextElementInt(node, "Logontype");
	if (logonType < 0)
		return false;

	server.SetLogonType((enum LogonType)logonType);

	if (server.GetLogonType() != ANONYMOUS)
	{
		wxString user = GetTextElement(node, "User");

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

	if (protocol == FTP || protocol == FTPS || protocol == FTPES)
	{
		std::vector<wxString> postLoginCommands;
		TiXmlElement* pElement = node->FirstChildElement("PostLoginCommands");
		if (pElement)
		{
			TiXmlElement* pCommandElement = pElement->FirstChildElement("Command");
			while (pCommandElement)
			{
				TiXmlNode* textNode = pCommandElement->FirstChild();
				if (textNode && textNode->ToText())
				{
					wxString command = ConvLocal(textNode->Value());
					if (command != _T(""))
						postLoginCommands.push_back(command);
				}

				pCommandElement = pCommandElement->NextSiblingElement("Command");
			}
		}
		if (!server.SetPostLoginCommands(postLoginCommands))
			return false;
	}

	server.SetBypassProxy(GetTextElementInt(node, "BypassProxy", false) == 1);
	server.SetName(GetTextElement_Trimmed(node, "Name"));

	if (server.GetName().empty())
		server.SetName(GetTextElement_Trimmed(node));

	return true;
}

void SetServer(TiXmlElement *node, const CServer& server)
{
	if (!node)
		return;

	bool kiosk_mode = COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) != 0;
	
	node->Clear();

	AddTextElement(node, "Host", server.GetHost());
	AddTextElement(node, "Port", server.GetPort());
	AddTextElement(node, "Protocol", server.GetProtocol());
	AddTextElement(node, "Type", server.GetType());

	enum LogonType logonType = server.GetLogonType();

	if (server.GetLogonType() != ANONYMOUS)
	{
		AddTextElement(node, "User", server.GetUser());

		if (server.GetLogonType() == NORMAL || server.GetLogonType() == ACCOUNT)
		{
			if (kiosk_mode)
				logonType = ASK;
			else
			{
				AddTextElement(node, "Pass", server.GetPass());

				if (server.GetLogonType() == ACCOUNT)
					AddTextElement(node, "Account", server.GetAccount());
			}
		}
	}
	AddTextElement(node, "Logontype", logonType);

	AddTextElement(node, "TimezoneOffset", server.GetTimezoneOffset());
	switch (server.GetPasvMode())
	{
	case MODE_PASSIVE:
		AddTextElementRaw(node, "PasvMode", "MODE_PASSIVE");
		break;
	case MODE_ACTIVE:
		AddTextElementRaw(node, "PasvMode", "MODE_ACTIVE");
		break;
	default:
		AddTextElementRaw(node, "PasvMode", "MODE_DEFAULT");
		break;
	}
	AddTextElement(node, "MaximumMultipleConnections", server.MaximumMultipleConnections());

	switch (server.GetEncodingType())
	{
	case ENCODING_AUTO:
		AddTextElementRaw(node, "EncodingType", "Auto");
		break;
	case ENCODING_UTF8:
		AddTextElementRaw(node, "EncodingType", "UTF-8");
		break;
	case ENCODING_CUSTOM:
		AddTextElementRaw(node, "EncodingType", "Custom");
		AddTextElement(node, "CustomEncoding", server.GetCustomEncoding());
		break;
	}

	const enum ServerProtocol protocol = server.GetProtocol();
	if (protocol == FTP || protocol == FTPS || protocol == FTPES)
	{
		const std::vector<wxString>& postLoginCommands = server.GetPostLoginCommands();
		if (!postLoginCommands.empty())
		{
			TiXmlElement* pElement = node->LinkEndChild(new TiXmlElement("PostLoginCommands"))->ToElement();
			for (std::vector<wxString>::const_iterator iter = postLoginCommands.begin(); iter != postLoginCommands.end(); iter++)
				AddTextElement(pElement, "Command", *iter);
		}
	}

	AddTextElementRaw(node, "BypassProxy", server.GetBypassProxy() ? "1" : "0");
	const wxString& name = server.GetName();
	if (name != _T(""))
		AddTextElement(node, "Name", name);
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

	const char* value = node->Attribute(name);
	if (!value)
		return _T("");

	return ConvLocal(value);
}

TiXmlElement* FindElementWithAttribute(TiXmlElement* node, const char* element, const char* attribute, const char* value)
{
	TiXmlElement* child;
	if (element)
		child = node->FirstChildElement(element);
	else
		child = node->FirstChildElement();

	while (child)
	{
		const char* nodeVal = child->Attribute(attribute);
		if (nodeVal && !strcmp(value, nodeVal))
			return child;

		if (element)
			child = child->NextSiblingElement(element);
		else
			child = child->NextSiblingElement();
	}

	return 0;
}

TiXmlElement* FindElementWithAttribute(TiXmlElement* node, const char* element, const char* attribute, int value)
{
	TiXmlElement* child;
	if (element)
		child = node->FirstChildElement(element);
	else
		child = node->FirstChildElement();

	while (child)
	{
		int nodeValue;
		const char* nodeVal = child->Attribute(attribute, &nodeValue);
		if (nodeVal && nodeValue == value)
			return child;

		if (element)
			child = child->NextSiblingElement(element);
		else
			child = child->NextSiblingElement();
	}

	return 0;
}

int GetAttributeInt(TiXmlElement* node, const char* name)
{
	int value;
	if (!node->Attribute(name, &value))
		return 0;

	return value;
}

void SetAttributeInt(TiXmlElement* node, const char* name, int value)
{
	node->SetAttribute(name, value);
}

int CXmlFile::GetRawDataLength()
{
	if (!m_pDocument)
		return 0;

	delete m_pPrinter;
	m_pPrinter = new TiXmlPrinter;
	m_pPrinter->SetStreamPrinting();

	m_pDocument->Accept(m_pPrinter);
	return m_pPrinter->Size();
}

void CXmlFile::GetRawDataHere(char* p) // p has to big enough to hold at least GetRawDataLength() bytes
{
	if (!m_pPrinter)
	{
		wxFAIL;
		return;
	}

	memcpy(p, m_pPrinter->CStr(), m_pPrinter->Size());
}

bool CXmlFile::ParseData(char* data)
{
	delete m_pDocument;
	m_pDocument = new TiXmlDocument;
	m_pDocument->SetCondenseWhiteSpace(false);
	m_pDocument->Parse(data);

	if (!m_pDocument->FirstChildElement("FileZilla3"))
	{
		delete m_pDocument;
		m_pDocument = 0;
		return false;
	}

	return true;
}
