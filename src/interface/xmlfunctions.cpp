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
