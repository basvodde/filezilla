/****************************************************************************

Copyright (c) 2007 Andrei Borovsky <anb@symmetrica.net>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#ifndef WX_DBUS_MESSAGE
#define WX_DBUS_MESSAGE

#include "wxdbusconnection.h"

class wxDBusMessage : public wxObject
{
public:
	wxDBusMessage(DBusMessage * message);
	virtual ~wxDBusMessage();
	static wxDBusMessage * ExtractFromEvent(wxDBusConnectionEvent * event);
	unsigned int GetSerial();
	unsigned int GetReplySerial();
	void SetReplySerial(unsigned int serial);
	const char * GetPath();
	const char * GetInterface();
	const char * GetMember();
	int GetType();
	DBusMessage * GetMessage();
	bool MoveToNextArg();
	int GetAgrType();
	void GetArgValue(void * value);
	const char * GetString();
	const char * GetObjectPath();
	bool GetUInt(wxUint32& u);
	int GetArrayElementType();
	//int GetArrayLength(); uses deprecated function
	void GetFixedArray(void * value, int * n_elements);
	bool AddArg(int type, void * value);
	bool AddArrayOfString(const char **value, int n_elements);
	bool AddArrayOfByte(int element_type, const void *value, int n_elements);
	bool AddString(const char * value);
	bool AddInt(int value);
	bool AddUnsignedInt(unsigned int value);
	bool AddBool(bool value);
	bool AddObjectPath(const char* value);
	bool AddDict(const char** value, int n_elements); // Call with null to add empty dictionary. n_elements needs to be divisible by 2
	bool Send(wxDBusConnection * connection, unsigned int * serial);
	
private:
	DBusMessage * m_message;
	DBusMessageIter m_iter;
	bool m_get_iter_initialized;
	bool m_add_iter_initialized;
	void init_add_iter();
};

class wxDBusMethodCall : public wxDBusMessage
{
public:
	wxDBusMethodCall(const char *destination, const char *path, const char *interface, const char *method);
	wxDBusMessage * Call(wxDBusConnection * connection, int timeout);
	bool CallAsync(wxDBusConnection * connection, int timeout);
};

class wxDBusSignal : public wxDBusMessage
{
public:
	wxDBusSignal(const char *path, const char *interface, const char *name);
};

class wxDBusMethodReturn : public wxDBusMessage
{
	wxDBusMethodReturn(wxDBusMessage * method_call);
	bool Return(wxDBusConnection * connection, unsigned int * serial);
};

class wxDBusErrorMessage : public wxDBusMessage
{
	wxDBusErrorMessage(wxDBusMessage * reply_to, const char * error_name, const char * error_message);
};

#endif
