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


using namespace std;
#include "wxdbusconnection.h"
#include <poll.h>
#include <list>


typedef list<DBusWatch *> WatchesList;

static dbus_bool_t add_watch(DBusWatch *watch, void *data);

static void remove_watch(DBusWatch *watch, void *data);

static void toggle_watch(DBusWatch *watch, void *data);

class DBusThread : public wxThread
{
public:
	DBusThread(wxThreadKind kind, wxDBusConnection * parent, int ID, DBusConnection * connection) ;
	virtual ~DBusThread();
	virtual ExitCode Entry();
	inline WatchesList * GetWatchesList() { return &bus_watches; }
	void rebuild_fd_array();
	inline void EnterCriticalSection() { m_mutex.Lock(); }
	inline void LeaveCriticalSection() { m_mutex.Unlock(); }
	inline void SetExit() { m_exit = true; }
	inline int GetID() { return m_ID; }
	
	void Wakeup();

	int m_wakeup_pipe[2];
	wxMutex m_mutex;
	wxCondition m_condition;
	int m_waiting;
private:
	int m_ID;
	DBusConnection * m_connection;
	wxDBusConnection * m_parent;
	bool m_exit;
	list<DBusWatch *> bus_watches;
	struct pollfd * fd_array;
};

void DBusThread::Wakeup()
{
	char tmp = 0;
	write(m_wakeup_pipe[1], &tmp, 1);
}

DBusThread::DBusThread(wxThreadKind kind, wxDBusConnection * parent, int ID, DBusConnection * connection): wxThread(kind),
	m_mutex(), m_condition(m_mutex)
{
	m_ID = ID;
	m_connection = connection;
	m_parent = parent;
	m_exit = false;
	fd_array = NULL;
	m_waiting = 0;

	pipe(m_wakeup_pipe);
}

DBusThread::~DBusThread()
{
	close(m_wakeup_pipe[0]);
	close(m_wakeup_pipe[1]);
	delete [] fd_array;
}

void DBusThread::rebuild_fd_array()
{
	delete [] fd_array;
	fd_array = new pollfd[bus_watches.size() + 1];
	list<DBusWatch *>::iterator it;
	int i = 0;
	for (it = bus_watches.begin(); it != bus_watches.end(); it++, i++) {
		fd_array[i].fd = dbus_watch_get_unix_fd(* it);
		fd_array[i].events = 0;
		if (dbus_watch_get_enabled(* it)) {
			int flags = dbus_watch_get_flags(* it);
			fd_array[i].events = POLLHUP | POLLERR;
			if (flags & DBUS_WATCH_READABLE)
				fd_array[i].events |= POLLIN;
			if (flags & DBUS_WATCH_WRITABLE)
				fd_array[i].events |= POLLOUT;
		}
		fd_array[i].revents = 0;
	}
	fd_array[i].fd = m_wakeup_pipe[0];
	fd_array[i].events = POLLIN;
}

wxThread::ExitCode DBusThread::Entry()
{
	dbus_connection_set_watch_functions(m_connection, add_watch, remove_watch, toggle_watch, (void *) this, NULL);
	while (!m_exit)	{
		EnterCriticalSection();
		int res;
		if ((res = poll(fd_array, bus_watches.size() + 1, -1)) > 0) 
		{
			list<DBusWatch *>::iterator it = bus_watches.begin();
			for (unsigned int i = 0; i < bus_watches.size(); i++, it++) {
				if (fd_array[i].revents) {
					int flags = 0;
					if (fd_array[i].revents & POLLIN)
						flags |= DBUS_WATCH_READABLE;
					if (fd_array[i].revents & POLLOUT)
						flags |= DBUS_WATCH_WRITABLE;
					if (fd_array[i].revents & POLLERR)
						flags |= DBUS_WATCH_ERROR;
					if (fd_array[i].revents & POLLHUP)
						flags |= DBUS_WATCH_HANGUP;
					dbus_watch_handle(* it, flags);
				}
			}
			if (fd_array[bus_watches.size()].revents & POLLIN)
			{
				char tmp;
				read(m_wakeup_pipe[0], &tmp, 1);
				if (m_waiting == 1)
				{
					m_waiting = 2;
					m_condition.Wait();
				}
			}
		}

		if (dbus_connection_get_dispatch_status(m_connection) == DBUS_DISPATCH_DATA_REMAINS) 
			dbus_connection_dispatch(m_connection);

		// Update flags
		list<DBusWatch *>::iterator it = bus_watches.begin();
		for (unsigned int i = 0; i < bus_watches.size(); i++, it++) {
			if (fd_array[i].revents) {
				int flags = dbus_watch_get_flags(* it);
				fd_array[i].events = POLLHUP | POLLERR;
				if (flags & DBUS_WATCH_READABLE)
					fd_array[i].events |= POLLIN;
				if (flags & DBUS_WATCH_WRITABLE)
					fd_array[i].events |= POLLOUT;
			}
		}
		LeaveCriticalSection();
	}

	return 0;
}

dbus_bool_t add_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	thread->m_waiting = 1;
	char tmp = 0;
	write(thread->m_wakeup_pipe[1], &tmp, 0);
	thread->EnterCriticalSection();
	thread->GetWatchesList()->push_front(watch);
	thread->rebuild_fd_array();

	if (thread->m_waiting == 2)
		thread->m_condition.Signal();
	thread->m_waiting = 0;
	thread->LeaveCriticalSection();
	return true;
}

void remove_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	thread->m_waiting = 1;
	char tmp = 0;
	write(thread->m_wakeup_pipe[1], &tmp, 0);
	thread->EnterCriticalSection();

	thread->GetWatchesList()->remove(watch);
	thread->rebuild_fd_array();

	if (thread->m_waiting == 2)
		thread->m_condition.Signal();
	thread->m_waiting = 0;
	thread->LeaveCriticalSection();
}

void toggle_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	thread->m_waiting = 1;
	char tmp = 0;
	write(thread->m_wakeup_pipe[1], &tmp, 0);
	thread->EnterCriticalSection();

	thread->rebuild_fd_array();

	if (thread->m_waiting == 2)
		thread->m_condition.Signal();
	thread->m_waiting = 0;
	thread->LeaveCriticalSection();
}

DBusHandlerResult handle_message(DBusConnection *connection, DBusMessage *message, void *user_data);

void response_notify(DBusPendingCall *pending, void *user_data);

DBusHandlerResult handle_notification(DBusConnection *connection, DBusMessage *message, void *user_data);

wxDBusConnection::wxDBusConnection(int ID, wxEvtHandler * EvtHandler, bool System)
{
	m_error = new wxDBusError;
	m_connection = System ? dbus_bus_get(DBUS_BUS_SYSTEM, &(m_error->GetError())) : dbus_bus_get(DBUS_BUS_SESSION, &(m_error->GetError()));
	if (!m_connection) {
		printf("Failed to connect to D-BUS: %s", m_error->GetError().message);
		m_thread = NULL;
	} else
	{
		dbus_connection_add_filter(m_connection, handle_message, (void *) this, NULL);
		m_ID = ID;
		m_EvtHandler = EvtHandler;
		m_thread = new DBusThread(wxTHREAD_JOINABLE, this, ID, m_connection);
		m_thread->Create();
		m_thread->Run();
	}
}

wxDBusConnection::~wxDBusConnection()
{
	if (m_thread != NULL)
	{
		m_thread->SetExit();
		m_thread->Wait();
		delete m_thread;
	}
	if (m_connection != NULL)
		dbus_connection_unref(m_connection);
	delete m_error;
}

bool wxDBusConnection::IsConnected()
{
	return m_connection && (bool)dbus_connection_get_is_connected(m_connection);
}

bool wxDBusConnection::Send(DBusMessage *message, dbus_uint32_t *serial)
{
	bool res = dbus_connection_send(m_connection, message, serial);
	m_thread->Wakeup();
	return res;
}

bool wxDBusConnection::SendWithReply(DBusMessage *message, int timeout_milliseconds)
{
	DBusPendingCall * pcall = NULL;
	bool result = (bool) dbus_connection_send_with_reply(m_connection, message, &pcall, timeout_milliseconds);
	dbus_pending_call_set_notify(pcall, response_notify, (void *) this, NULL);
	m_thread->Wakeup();
	return result;
}

DBusMessage * wxDBusConnection::SendWithReplyAndBlock(DBusMessage *message, int timeout_milliseconds)
{
	m_error->Reset();
	DBusMessage * result = dbus_connection_send_with_reply_and_block(m_connection, message, timeout_milliseconds, &(m_error->GetError()));
	m_thread->Wakeup();
	return result;
}

void wxDBusConnection::Flush()
{
	dbus_connection_flush(m_connection);
	m_thread->Wakeup();
}

DBusMessage * wxDBusConnection::BorrowMessage()
{
	return dbus_connection_borrow_message(m_connection);
}

void wxDBusConnection::ReturnMessage(DBusMessage * message)
{
	dbus_connection_return_message(m_connection, message);
}
	
DBusMessage * wxDBusConnection::PopMessage()
{
	return dbus_connection_pop_message(m_connection);
}

bool wxDBusConnection::RegisterObjectPath(const char *path)
{
	DBusObjectPathVTable vtable;
	vtable.unregister_function = NULL;
	vtable.message_function = handle_notification;
	bool result = dbus_connection_register_object_path(m_connection, path, &vtable, this);
	m_thread->Wakeup();
	return result;
}

bool wxDBusConnection::UnregisterObjectPath(const char *path)
{
	bool result = dbus_connection_unregister_object_path(m_connection, path);
	m_thread->Wakeup();
	return result;
}

wxDBusError &wxDBusConnection::GetLastError()
{
	return * m_error;
}

DBusHandlerResult handle_notification(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	wxDBusConnection * _connection = (wxDBusConnection *) user_data;
	DBusMessage * msg = dbus_message_copy(message);
	wxDBusConnectionEvent event(wxEVT_DBUS_NOTIFICATION, _connection->GetID(), msg);
	_connection->GetEvtHandler()->AddPendingEvent(event);
	return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult handle_message(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	wxDBusConnection * _connection = (wxDBusConnection *) user_data;
	DBusMessage * msg = dbus_message_copy(message);
	wxDBusConnectionEvent event(wxEVT_DBUS_SIGNAL, _connection->GetID(), msg);
	_connection->GetEvtHandler()->AddPendingEvent(event);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void response_notify(DBusPendingCall *pending, void *user_data)
{
	DBusMessage * reply;
	reply = dbus_pending_call_steal_reply(pending);
	DBusMessage * msg = dbus_message_copy(reply);
	wxDBusConnection * connection = (wxDBusConnection *) user_data;
	wxDBusConnectionEvent event(wxEVT_DBUS_ASYNC_RESPONSE, connection->GetID(), msg);
	connection->GetEvtHandler()->AddPendingEvent(event);
	dbus_message_unref(reply);
	dbus_pending_call_unref(pending);
}

wxDBusError::wxDBusError()
{
	dbus_error_init(&m_error);
}

wxDBusError::~wxDBusError()
{
	dbus_error_free(&m_error);
}

bool wxDBusError::IsErrorName(const char * name)
{
	return (bool) dbus_error_has_name(&m_error, name);
}

DBusError &wxDBusError::GetError()
{
	return m_error;
}
	
void wxDBusError::Reset()
{
	dbus_error_free(&m_error);
	dbus_error_init(&m_error);
}
	
const char * wxDBusError::GetName()
{
	return m_error.name;
}

const char * wxDBusError::GetMessage()
{
	return m_error.message;
}
	
bool wxDBusError::IsSet()
{
	return (bool) dbus_error_is_set(&m_error);
}

DEFINE_EVENT_TYPE(wxEVT_DBUS_NOTIFICATION)
DEFINE_EVENT_TYPE(wxEVT_DBUS_SIGNAL)
DEFINE_EVENT_TYPE(wxEVT_DBUS_ASYNC_RESPONSE)
IMPLEMENT_DYNAMIC_CLASS(wxDBusConnectionEvent, wxNotifyEvent)
