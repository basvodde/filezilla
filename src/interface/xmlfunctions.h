/*
 * xmlfunctions.h declares some useful xml helper functions, especially to
 * improve usability together with wxWidgets.
 */

#ifndef __XMLFUNCTIONS_H__
#define __XMLFUNCTIONS_H__

#include "../tinyxml/tinyxml.h"

// Convert the given string into an UTF-8 string. Returned string has to be deleted manually.
char* ConvUTF8(const wxString& value);

// Convert the given utf-8 string into wxString
wxString ConvLocal(const char *value);

// Add a new element with the specified name and value to the xml document
void AddTextElement(TiXmlElement* node, const char* name, const wxString& value);

// Get string from named element
wxString GetTextElement(TiXmlElement* node, const char* name);

// Get (64-bit) integer from named element
int GetTextElementInt(TiXmlElement* node, const char* name, int defValue = 0);
wxLongLong GetTextElementLongLong(TiXmlElement* node, const char* name, int defValue = 0);

#endif //__XMLFUNCTIONS_H__
