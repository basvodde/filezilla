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

#include "wxdbusconnection.h"
#include "config.h"
#include <poll.h>
#include <list>

// Define WITH_LIBDBUS to 1 (e.g. from configure) if you are using
// libdbus < 1.2 that does not have dbus_watch_get_unix_fd yet.
#ifndef WITH_LIBDBUS
#define WITH_LIBDBUS 2
#endif

typedef std::list<DBusWatch *> WatchesList;

class DBusThread : public wxThread
{
public:
	DBusThread(wxThreadKind kind, wxDBusConnection * parent, int ID, DBusConnection * connection) ;
	virtual ~DBusThread();
	virtual ExitCode Entry();
	inline void SetExit() { m_exit = true; }
	inline int GetID() { return m_ID; }
	
	void Wakeup();

	inline void EnterCriticalSection() { m_critical_section.Enter(); }
	inline void LeaveCriticalSection() { m_critical_section.Leave(); }

private:
	inline WatchesList * GetWatchesList() { return &bus_watches; }
	bool CalledFromThread() const { return !pthread_equal(m_parent_id, pthread_self()); }

	int m_wakeup_pipe[2];
	wxCriticalSection m_critical_section;
	bool m_thread_holds_lock;
	
	int m_ID;
	DBusConnection * m_connection;
	wxDBusConnection * m_parent;
	bool m_exit;
	std::list<DBusWatch *> bus_watches;

	pthread_t m_parent_id;

	static dbus_bool_t add_watch(DBusWatch *watch, void *data);
	static void remove_watch(DBusWatch *watch, void *data);
	static void toggle_watch(DBusWatch *watch, void *data);
	dbus_bool_t add_watch(DBusWatch *watch);
	void remove_watch(DBusWatch *watch);
	void toggle_watch(DBusWatch *watch);
};

void DBusThread::Wakeup()
{
	char tmp = 0;
	write(m_wakeup_pipe[1], &tmp, 1);
}

DBusThread::DBusThread(wxThreadKind kind, wxDBusConnection * parent, int ID, DBusConnection * connection): wxThread(kind)
{
	m_ID = ID;
	m_connection = connection;
	m_parent = parent;
	m_exit = false;
	m_thread_holds_lock = false;

	pipe(m_wakeup_pipe);
	fcntl(m_wakeup_pipe[0], F_SETFL, O_NONBLOCK);

	m_parent_id = pthread_self();

	dbus_connection_set_watch_functions(m_connection, add_watch, remove_watch, toggle_watch, (void *)this, NULL);
}

DBusThread::~DBusThread()
{
	close(m_wakeup_pipe[0]);
	close(m_wakeup_pipe[1]);
}

wxThread::ExitCode DBusThread::Entry()
{
	while (!m_exit)	{

		EnterCriticalSection();

		while (dbus_connection_get_dispatch_status(m_connection) == DBUS_DISPATCH_DATA_REMAINS) 
			dbus_connection_dispatch(m_connection);

		// Prepare list of file descriptors to pull
		struct pollfd *fd_array = new struct pollfd[bus_watches.size() + 1];
		
		fd_array[0].fd = m_wakeup_pipe[0];
		fd_array[0].events = POLLIN;

		unsigned int nfds = 1;
		for (std::list<DBusWatch *>::iterator it = bus_watches.begin(); it != bus_watches.end(); it++) {
			if (dbus_watch_get_enabled(*it))
			{
#if WITH_LIBDBUS >= 2
				fd_array[nfds].fd = dbus_watch_get_unix_fd(*it);
#else
				fd_array[nfds].fd = dbus_watch_get_fd(*it);
#endif
				int flags = dbus_watch_get_flags(*it);
				fd_array[nfds].events = POLLHUP | POLLERR;
				if (flags & DBUS_WATCH_READABLE)
					fd_array[nfds].events |= POLLIN;
				if (flags & DBUS_WATCH_WRITABLE)
					fd_array[nfds].events |= POLLOUT;
				nfds++;				
			}
		}
		LeaveCriticalSection();

		int res;
		if ((res = poll(fd_array, nfds, -1)) > 0) {

			EnterCriticalSection();
			m_thread_holds_lock = true;

			char tmp;
			if (read(m_wakeup_pipe[0], &tmp, 1) == 1) {
				m_thread_holds_lock = false;
				LeaveCriticalSection();
				delete [] fd_array;
				continue;
			}

			for (std::list<DBusWatch *>::iterator it = bus_watches.begin(); it != bus_watches.end(); it++) {
				if (!dbus_watch_get_enabled(*it))
					continue;
#if WITH_LIBDBUS >= 2
				int fd = dbus_watch_get_unix_fd(*it);
#else
				int fd = dbus_watch_get_fd(*it);
#endif
				unsigned int i;
				for (i = 1; i < nfds; i++) {
					if (fd_array[i].fd == fd && fd_array[i].revents) {
						int flags = 0;
						if (fd_array[i].revents & POLLIN)
							flags |= DBUS_WATCH_READABLE;
						if (fd_array[i].revents & POLLOUT)
							flags |= DBUS_WATCH_WRITABLE;
						if (fd_array[i].revents & POLLERR)
							flags |= DBUS_WATCH_ERROR;
						if (fd_array[i].revents & POLLHUP)
							flags |= DBUS_WATCH_HANGUP;
						dbus_watch_handle(*it, flags);
						break;
					}
				}

				// Only handle a single watch in each poll iteration, bus_watches could change inside dbus_watch_handle
				if (i != nfds)
					break;
			}
			m_thread_holds_lock = false;
			LeaveCriticalSection();
		}

		delete [] fd_array;
	}

	return 0;
}

dbus_bool_t DBusThread::add_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	if (!thread->CalledFromThread()) {
		char tmp = 0;
		write(thread->m_wakeup_pipe[1], &tmp, 0);
		thread->EnterCriticalSection();
	}
	else if (!thread->m_thread_holds_lock)
		thread->EnterCriticalSection();

	WatchesList &watches = *thread->GetWatchesList();
	watches.push_back(watch);

	if (!thread->CalledFromThread())
		thread->LeaveCriticalSection();
	else if (!thread->m_thread_holds_lock)
		thread->LeaveCriticalSection();

	return true;
}

void DBusThread::remove_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	if (!thread->CalledFromThread()) {
		char tmp = 0;
		write(thread->m_wakeup_pipe[1], &tmp, 0);
		thread->EnterCriticalSection();
	}
	else if (!thread->m_thread_holds_lock)
		thread->EnterCriticalSection();

	WatchesList *watches = thread->GetWatchesList();
	watches->remove(watch);

	if (!thread->CalledFromThread())
		thread->LeaveCriticalSection();
	else if (!thread->m_thread_holds_lock)
		thread->LeaveCriticalSection();
}

void DBusThread::toggle_watch(DBusWatch *watch, void *data)
{
	DBusThread * thread = (DBusThread *) data;

	thread->Wakeup();
}

DBusHandlerResult handle_message(DBusConnection *connection, DBusMessage *message, void *user_data);

void response_notify(DBusPendingCall *pending, void *user_data);

DBusHandlerResult handle_notification(DBusConnection *connection, DBusMessage *message, void *user_data);

wxDBusConnection::wxDBusConnection(int ID, wxEvtHandler * EvtHandler, bool System)
{
	// Make sure libdbus locks its data structures, otherwise
	// there'll be crashes if it gets used from multiple threads
	// at the same time
	dbus_threads_init_default();

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
		m_thread->Wakeup();
		m_thread->Wait();
		delete m_thread;
	}
	if (m_connection != NULL) {
		dbus_connection_unref(m_connection);
	}
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
	m_thread->EnterCriticalSection();
	bool result = (bool) dbus_connection_send_with_reply(m_connection, message, &pcall, timeout_milliseconds);
	dbus_pending_call_set_notify(pcall, response_notify, (void *) this, NULL);
	m_thread->LeaveCriticalSection();
	m_thread->Wakeup();
	return result;
}

DBusMessage * wxDBusConnection::SendWithReplyAndBlock(DBusMessage *message, int timeout_milliseconds)
{
	m_error->Reset();
	m_thread->EnterCriticalSection();
	// We need to block the mainloop or bad things will happen
	DBusMessage * result = dbus_connection_send_with_reply_and_block(m_connection, message, timeout_milliseconds, &(m_error->GetError()));
	m_thread->LeaveCriticalSection();
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
