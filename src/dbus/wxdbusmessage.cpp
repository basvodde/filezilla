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


#include "wxdbusmessage.h"
#include <string.h>

wxDBusMessage::wxDBusMessage(DBusMessage * message)
{
	m_message = message;
	m_get_iter_initialized = false;	
	m_add_iter_initialized = false;
}

wxDBusMessage::~wxDBusMessage()
{
	dbus_message_unref(m_message);
}

wxDBusMessage * wxDBusMessage::ExtractFromEvent(wxDBusConnectionEvent * event)
{
	return new wxDBusMessage(event->GetMessage());
}

unsigned int wxDBusMessage::GetSerial()
{
	return dbus_message_get_serial(m_message);
}

unsigned int wxDBusMessage::GetReplySerial()
{
	return dbus_message_get_reply_serial(m_message);
}

void wxDBusMessage::SetReplySerial(unsigned int serial)
{
	dbus_message_set_reply_serial(m_message, serial);
}

const char * wxDBusMessage::GetPath()
{
	return dbus_message_get_path(m_message);
}

const char * wxDBusMessage::GetMember()
{
	 return dbus_message_get_member(m_message);
}

int wxDBusMessage::GetType()
{
	 return dbus_message_get_type(m_message);
}

const char * wxDBusMessage::GetInterface()
{
	return dbus_message_get_interface(m_message);
}

DBusMessage * wxDBusMessage::GetMessage()
{
	return m_message;
}

bool wxDBusMessage::MoveToNextArg()
{
	if (!m_get_iter_initialized){
		if (!dbus_message_iter_init(m_message, &m_iter))
			return false;
		m_get_iter_initialized = true;
		m_add_iter_initialized = false;
		return true;
	}
	return (bool) dbus_message_iter_next(&m_iter);
}
	
int wxDBusMessage::GetAgrType()
{
	if (!m_get_iter_initialized)
		MoveToNextArg();
	return dbus_message_iter_get_arg_type(&m_iter);
}

void wxDBusMessage::GetArgValue(void * value)
{
	if (!m_get_iter_initialized)
		MoveToNextArg();
	dbus_message_iter_get_basic(&m_iter, value);
}

bool wxDBusMessage::GetUInt(wxUint32& u)
{
	return dbus_message_get_args(m_message, 0, DBUS_TYPE_UINT32, &u, DBUS_TYPE_INVALID);
}

const char * wxDBusMessage::GetString()
{
	const char * result;
	if (!dbus_message_get_args(m_message, 0, DBUS_TYPE_STRING, &result, DBUS_TYPE_INVALID))
		return 0;

	return result;
}

const char * wxDBusMessage::GetObjectPath()
{
	const char * result;
	if (!dbus_message_get_args(m_message, 0, DBUS_TYPE_OBJECT_PATH, &result, DBUS_TYPE_INVALID))
		return 0;

	return result;
}

int wxDBusMessage::GetArrayElementType()
{
	return dbus_message_iter_get_element_type(&m_iter);
}
	
/*
int wxDBusMessage::GetArrayLength()
{
	dbus_message_iter_get_array_len(&m_iter);
}
*/

void wxDBusMessage::GetFixedArray(void * value, int * n_elements)
{
	DBusMessageIter sub;
	dbus_message_iter_recurse(&m_iter, &sub);
	dbus_message_iter_get_fixed_array(&sub, value, n_elements);
}

void wxDBusMessage::init_add_iter()
{
	if (!m_add_iter_initialized){
		dbus_message_iter_init_append(m_message, &m_iter);
		m_add_iter_initialized = true;
		m_get_iter_initialized = false;
	}
}

bool wxDBusMessage::AddArg(int type, void * value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, type, value);
}

bool wxDBusMessage::AddArrayOfString(const char **value, int n_elements)
{
	init_add_iter();
	DBusMessageIter sub;
	dbus_message_iter_open_container(&m_iter, DBUS_TYPE_ARRAY, "s", &sub);
	for (int i = 0; i < n_elements; i++)
	{
		if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(value[i])))
			return false;
	}
	dbus_message_iter_close_container(&m_iter, &sub);

	return true;
}

bool wxDBusMessage::AddArrayOfByte(int element_type, const void *value, int n_elements)
{
	init_add_iter();
	DBusMessageIter sub;
	dbus_message_iter_open_container(&m_iter, DBUS_TYPE_ARRAY, "y", &sub);
	if (!dbus_message_iter_append_fixed_array(&sub, element_type, value, n_elements))
		return false;
	dbus_message_iter_close_container(&m_iter, &sub);

	return true;
}

bool wxDBusMessage::AddDict(const char **value, int n_elements)
{
	init_add_iter();
	DBusMessageIter sub;
	dbus_message_iter_open_container(&m_iter, DBUS_TYPE_ARRAY, 
		DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
		&sub);
	for (int i = 0; i < n_elements; i += 2)
	{
		DBusMessageIter dict_iter;
		dbus_message_iter_open_container(&sub, DBUS_TYPE_DICT_ENTRY, 0, &dict_iter);
		if (!dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &(value[i])))
			return false;
		DBusMessageIter variant;
		dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &variant);
		if (!dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &(value[i + 1])))
			return false;
		dbus_message_iter_close_container(&dict_iter, &variant);
		dbus_message_iter_close_container(&sub, &dict_iter);
	}
	dbus_message_iter_close_container(&m_iter, &sub);

	return true;
}

bool wxDBusMessage::AddString(const char * value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, DBUS_TYPE_STRING, &value);
}

bool wxDBusMessage::AddInt(int value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, DBUS_TYPE_INT32, &value);
}

bool wxDBusMessage::AddUnsignedInt(unsigned int value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, DBUS_TYPE_UINT32, &value);
}

bool wxDBusMessage::AddBool(bool value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, DBUS_TYPE_BOOLEAN, &value);
}

bool wxDBusMessage::AddObjectPath(const char* value)
{
	init_add_iter();
	return (bool) dbus_message_iter_append_basic(&m_iter, DBUS_TYPE_OBJECT_PATH, &value);
}


bool wxDBusMessage::Send(wxDBusConnection * connection, unsigned int * serial)
{
	return (bool) connection->Send(m_message, serial);
}


wxDBusMethodCall::wxDBusMethodCall(const char *destination, const char *path, const char *interface, const char *method):
	wxDBusMessage(dbus_message_new_method_call(destination, path, interface, method))
{
}

wxDBusMessage * wxDBusMethodCall::Call(wxDBusConnection * connection, int timeout)
{
	DBusMessage * result = connection->SendWithReplyAndBlock(GetMessage(), timeout);
	return result != NULL ? new wxDBusMessage(result) : NULL;
}

bool wxDBusMethodCall::CallAsync(wxDBusConnection * connection, int timeout)
{
	return connection->SendWithReply(GetMessage(), timeout);
}

wxDBusSignal::wxDBusSignal(const char *path, const char *interface, const char *name) : 
	wxDBusMessage(dbus_message_new_signal(path, interface, name))
{
}

wxDBusMethodReturn::wxDBusMethodReturn(wxDBusMessage * method_call) :
	wxDBusMessage(dbus_message_new_method_return(method_call->GetMessage()))
{
}

bool wxDBusMethodReturn::Return(wxDBusConnection * connection, unsigned int * serial)
{
	dbus_message_set_no_reply(GetMessage(), TRUE);
	return (bool) connection->Send(GetMessage(), serial);
}

wxDBusErrorMessage::wxDBusErrorMessage(wxDBusMessage * reply_to, const char * error_name, const char * error_message) :
	wxDBusMessage(dbus_message_new_error(reply_to->GetMessage(), error_name, error_message))
{
}
