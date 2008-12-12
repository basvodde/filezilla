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

#ifndef WX_DBUSCONNECTION 
#define WX_DBUSCONNECTION

#include "wx/wx.h"
#include <dbus/dbus.h>

class DBusThread;

class wxDBusError : public wxObject
{
public:
	wxDBusError();
	virtual ~wxDBusError();
	DBusError &GetError();
	bool IsErrorName(const char * name);
	void Reset();
	const char * GetName();
	const char * GetMessage();
	bool IsSet();
private:
	DBusError m_error;
};

class wxDBusConnectionEvent : public wxNotifyEvent
{
public:
    wxDBusConnectionEvent(wxEventType commandType = wxEVT_NULL,
      int id = 0, DBusMessage * message = NULL): wxNotifyEvent(commandType, id)
		{m_message = message; };

	wxDBusConnectionEvent(const wxDBusConnectionEvent &event): wxNotifyEvent(event)
		{m_message = event.m_message;};

    virtual wxEvent *Clone() const
		{return new wxDBusConnectionEvent(*this);};

	DBusMessage * GetMessage()
		{ return m_message; };

DECLARE_DYNAMIC_CLASS(wxDBusConnectionEvent);
private:
	DBusMessage * m_message;
};

typedef void (wxEvtHandler::*wxDBusConnectionEventFunction)
                                        (wxDBusConnectionEvent&);
/*
  There are three events declared for the wxDBusConnection object. 
  wxEVT_DBUS_SIGNAL informs the application about any signal that appearrs on the bus.
  wxEVT_DBUS_NOTIFICATION informs the application about any messages that it 
  is has registered to recieve by calling RegisterObjectPath.
  These messages may be either signals or method calls.
  Note that all the messages that raise wxEVT_DBUS_NOTIFICATION also
  raise wxEVT_DBUS_SIGNAL.
  Your app may be uninterested in processing wxEVT_DBUS_SIGNAL, but it is
  a good practice to process it anyway. Each time the event is called a copy of a D-Bus 
  message is allocated for it.
  This copy should be freed in the event's handler. There are two ways to do it.
  the two following handlers do nothing else but release a copy of D-BUS message:

void MainFrame::OnSignal(wxDBusConnectionEvent& event)
{
	wxDBusMessage * msg = wxDBusMessage::ExtractFromEvent(&event);
	delete wxDBusMessage; // when we delete this object we free an underlying D-Bus message with it.
}

****

void MainFrame::OnSignal(wxDBusConnectionEvent& event)
{
	dbus_message_unref(event.GetMessage()); // here we call a low level D-Bus function to release the message. 
}

  The wxEVT_DBUS_ASYNC_RESPONSE event is raised when a response to an asynchronous
  method call arrives on the bus. Process this event only if you use asynchronous method calls.
*/

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_DBUS_NOTIFICATION, wxID_ANY)
	DECLARE_EVENT_TYPE(wxEVT_DBUS_SIGNAL,wxID_ANY)
	DECLARE_EVENT_TYPE(wxEVT_DBUS_ASYNC_RESPONSE, wxID_ANY)
END_DECLARE_EVENT_TYPES()

#define EVT_DBUS_NOTIFICATION(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_DBUS_NOTIFICATION, id, -1, (wxObjectEventFunction) (wxEventFunction)  (wxDBusConnectionEventFunction) & fn, (wxObject *) NULL ),

#define EVT_DBUS_SIGNAL(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_DBUS_SIGNAL, id, -1, (wxObjectEventFunction) (wxEventFunction) (wxDBusConnectionEventFunction) & fn, (wxObject *) NULL ),

#define EVT_DBUS_ASYNC_RESPONSE(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_DBUS_ASYNC_RESPONSE, id, -1, (wxObjectEventFunction) (wxEventFunction) (wxDBusConnectionEventFunction) & fn, (wxObject *) NULL ),

#define wxDBusConnectionEventHandler(func) (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxDBusConnectionEventFunction, &func)

class wxDBusConnection: public wxObject
{
public:
	wxDBusConnection(int ID, wxEvtHandler * EvtHandler, bool System);
	virtual ~wxDBusConnection();
	inline DBusConnection * GetConnection() {return m_connection;};
	inline int GetID() {return m_ID;};
	inline wxEvtHandler * GetEvtHandler() {return m_EvtHandler;};
	bool IsConnected();
	wxDBusError &GetLastError();
	bool Send(DBusMessage *message, dbus_uint32_t *serial);
	bool SendWithReply(DBusMessage *message, int timeout_milliseconds);
	DBusMessage * SendWithReplyAndBlock(DBusMessage *message, int timeout_milliseconds);
	void Flush();
	DBusMessage * BorrowMessage();
	void ReturnMessage(DBusMessage * message);
	DBusMessage * PopMessage();
	bool RegisterObjectPath(const char *path);
	bool UnregisterObjectPath(const char *path);
private:
	int m_ID;
	wxEvtHandler * m_EvtHandler;
	DBusConnection * m_connection;
	DBusThread * m_thread;
	wxDBusError * m_error;
};

#endif
