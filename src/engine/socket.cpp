#include <wx/defs.h>
#ifdef __WXMSW__
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif
#include "FileZilla.h"
#include "socket.h"
#include "threadex.h"

#define WAIT_CONNECT 0x01
#define WAIT_READ	 0x02
#define WAIT_WRITE	 0x04
#define WAIT_EVENTCOUNT 3

const wxEventType fzEVT_SOCKET = wxNewEventType();

#ifdef __WXMSW__
static int ConvertMSWErrorCode(int error)
{
	switch (error)
	{
	case WSAEINVAL:
		return EAI_BADFLAGS;
	case WSANO_RECOVERY:
		return EAI_FAIL;
	case WSAEAFNOSUPPORT:
		return EAI_FAMILY;
	case WSA_NOT_ENOUGH_MEMORY:
		return EAI_MEMORY;
	case WSANO_DATA:
		return EAI_NODATA;
	case WSAHOST_NOT_FOUND:
		return EAI_NONAME;
	case WSATYPE_NOT_FOUND:
		return EAI_SERVICE;
	case WSAESOCKTNOSUPPORT:
		return EAI_SOCKTYPE;
	case WSAEWOULDBLOCK:
		return EGAIN;
	case WSAEMFILE:
		return EMFILE;
	case WSAEINTR:
		return EINTR;
	case WSAEFAULT:
		return EFAULT;
	case WSAEACCES:
		return EACCES;
	default:
		return error;
	}
}
#endif

class X
{
public:
	X()
	{
		int errors[] = {
			0
		};
		for (int i = 0; errors[i]; i++)
		{
			int code = ConvertMSWErrorCode(errors[i]);
			if (CSocket::GetErrorDescription(code).Len() < 15)
				wxMessageBox(CSocket::GetErrorDescription(code));
		}

	}
};

CSocketEvent::CSocketEvent(int id, enum EventType type, wxString data)
	: wxEvent(id, fzEVT_SOCKET), m_type(type), m_data(data), m_error(0)
{
}

CSocketEvent::CSocketEvent(int id, enum EventType type, int error /*=0*/)
	: wxEvent(id, fzEVT_SOCKET), m_type(type), m_error(error)
{
}

wxEvent* CSocketEvent::Clone() const
{
	CSocketEvent* pClone = new CSocketEvent(GetId(), GetType(), m_error);
	pClone->m_data = m_data;
	return pClone;
}

class CSocketThread : protected wxThreadEx
{
	friend class CSocket;
public:
	CSocketThread()
		: m_condition(m_sync)
	{
		m_pSocket = 0;
		m_pHost = 0;
		m_pPort = 0;
#ifdef __WXMSW__
		m_sync_event = WSA_INVALID_EVENT;
#endif
		m_quit = false;

		m_waiting = 0;
		m_triggered = 0;
		m_threadwait = false;

		for (int i = 0; i < WAIT_EVENTCOUNT; i++)
		{
			m_triggered_errors[i] = 0;
		}
	}

	void SetSocket(CSocket* pSocket)
	{
		m_sync.Lock();
		m_pSocket = pSocket;
		if (m_pHost)
		{
			delete [] m_pHost;
			m_pHost = 0;
		}
		if (m_pPort)
		{
			delete [] m_pPort;
			m_pPort = 0;
		}
		m_waiting = 0;
		m_sync.Unlock();
	}

	int Connect()
	{
		wxASSERT(m_pSocket);
		if (m_pHost)
			delete [] m_pHost;
		if (m_pPort)
			delete [] m_pPort;

		wxWX2MBbuf buf = m_pSocket->m_host.mb_str();
		m_pHost = new char[strlen(buf) + 1];
		strcpy(m_pHost, buf);

		// Connect method of CSocket ensures port is in range
		m_pPort = new char[6];
		sprintf(m_pPort, "%d", m_pSocket->m_port);
		m_pPort[5] = 0;

		Start();

		return 0;
	}

	int Start()
	{
#ifdef __WXMSW__
		if (m_sync_event == WSA_INVALID_EVENT)
			m_sync_event = WSACreateEvent();
		if (m_sync_event == WSA_INVALID_EVENT)
			return 1;
#endif

		int res = Create();
		if (res != wxTHREAD_NO_ERROR)
			return 1;

		Run();

		return 0;
	}

	// Cancels select or idle wait
	void WakeupThread(bool already_locked)
	{
		if (!already_locked)
			m_sync.Lock();

		if (m_threadwait)
		{
			m_threadwait = false;
			m_condition.Signal();
			if (!already_locked)
				m_sync.Unlock();
			return;
		}
		
#ifdef __WXMSW__
		WSASetEvent(m_sync_event);
#else
#endif
		if (!already_locked)
			m_sync.Unlock();
	}

protected:

	bool SendEvent(CSocketEvent& evt)
	{
		if (!m_pSocket || !m_pSocket->m_pEvtHandler)
			return false;

		m_pSocket->m_pEvtHandler->AddPendingEvent(evt);

		return true;
	}

	// Only call while locked
	bool DoConnect()
	{
		char* pHost;
		char* pPort;

		if (!m_pHost || !m_pPort)
			return false;

		pHost = m_pHost;
		m_pHost = 0;
		pPort = m_pPort;
		m_pPort = 0;

		m_sync.Unlock();

		struct addrinfo *addressList = 0;
		struct addrinfo hints = {0};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		int res = getaddrinfo(pHost, pPort, &hints, &addressList);

		delete [] pHost;
		delete [] pPort;

		if (!Lock())
		{
			if (!res && addressList)
				freeaddrinfo(addressList);
			return false;
		}

		if (res)
		{
#ifdef __WXMSW__
			res = ConvertMSWErrorCode(res);
#endif

			CSocketEvent evt(m_pSocket->m_id, CSocketEvent::connection, res);
			SendEvent(evt);
			m_pSocket->m_state = CSocket::closed;
		}
		
		for (struct addrinfo *addr = addressList; addr; addr = addr->ai_next)
		{

			CSocketEvent evt(m_pSocket->m_id, CSocketEvent::hostaddress, CSocket::AddressToString(addr->ai_addr, addr->ai_addrlen));
			SendEvent(evt);

			int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

			if (fd == -1)
			{
#ifdef __WXMSW__
				int res = ConvertMSWErrorCode(WSAGetLastError());
#else
				int res = errno;
#endif
				CSocketEvent evt(m_pSocket->m_id, addr->ai_next ? CSocketEvent::connection_next : CSocketEvent::connection, res);
				SendEvent(evt);

				if (!addr->ai_next)
					m_pSocket->m_state = CSocket::closed;

				continue;
			}

			// Set socket to non-blocking.
			unsigned long nonblock = 1;
			ioctlsocket(fd, FIONBIO, &nonblock);

			int res = connect(fd, addr->ai_addr, addr->ai_addrlen);
			if (res == -1)
			{
#ifdef __WXMSW__
				// Map to POSIX error codes
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK)
					res = EINPROGRESS;
				else
					res = ConvertMSWErrorCode(WSAGetLastError());
#else
				res = errno;
#endif
			}

			if (res == EINPROGRESS)
			{
				
				m_pSocket->m_fd = fd;

				bool wait_successful;
				do
				{
					wait_successful = DoWait(WAIT_CONNECT);
					if ((m_triggered & WAIT_CONNECT))
						break;
				} while (wait_successful);

				if (!wait_successful)
				{
					close(fd);
					freeaddrinfo(addressList);
					return false;
				}

				res = m_triggered_errors[0];
			}

			if (res)
			{
				CSocketEvent evt(m_pSocket->m_id, addr->ai_next ? CSocketEvent::connection_next : CSocketEvent::connection, res);
				SendEvent(evt);

				m_pSocket->m_fd = -1;
				close(fd);

				if (!addr->ai_next)
					m_pSocket->m_state = CSocket::closed;
			}
			else
			{
				m_pSocket->m_fd = fd;
				m_pSocket->m_state = CSocket::connected;

				CSocketEvent evt(m_pSocket->m_id, CSocketEvent::connection, 0);
				SendEvent(evt);

				// We're now interested in all the other nice events
				m_waiting |= WAIT_READ | WAIT_WRITE;

				return true;
			}
		}
		freeaddrinfo(addressList);

		return false;
	}

	// Obtains lock in all cases.
	// Returns false if thread should quit
	bool Lock()
	{
		m_sync.Lock();
		if (m_quit || !m_pSocket)
			return false;

		return true;
	}

	// Call only while locked
	bool DoWait(int wait)
	{
		m_waiting |= wait;

		while (true)
		{
#ifdef __WXMSW__
			int wait_events = FD_CLOSE;
			if (m_waiting & WAIT_CONNECT)
				wait_events |= FD_CONNECT;
			if (m_waiting & WAIT_READ)
				wait_events |= FD_READ;
			if (m_waiting & WAIT_WRITE)
				wait_events |= FD_WRITE;
			WSAEventSelect(m_pSocket->m_fd, m_sync_event, wait_events);
			m_sync.Unlock();
			WSAWaitForMultipleEvents(1, &m_sync_event, false, WSA_INFINITE, false);

			m_sync.Lock();
			if (m_quit || !m_pSocket)
			{
				return false;
			}

			WSANETWORKEVENTS events;
			int res = WSAEnumNetworkEvents(m_pSocket->m_fd, m_sync_event, &events);
			if (res)
			{
				res = ConvertMSWErrorCode(WSAGetLastError());
				return false;
			}

			bool matched = false;
			if (m_waiting & WAIT_CONNECT)
			{
				if (events.lNetworkEvents & FD_CONNECT)
				{
					m_triggered |= WAIT_CONNECT;
					m_triggered_errors[0] = ConvertMSWErrorCode(events.iErrorCode[FD_CONNECT_BIT]);
					m_waiting &= ~WAIT_CONNECT;
					matched = true;
				}
			}
			if (m_waiting & WAIT_READ)
			{
				if (events.lNetworkEvents & FD_READ)
				{
					m_triggered |= WAIT_READ;
					m_triggered_errors[1] = ConvertMSWErrorCode(events.iErrorCode[FD_READ_BIT]);
					m_waiting &= ~WAIT_READ;
					matched = true;
				}
			}
			if (m_waiting & WAIT_WRITE)
			{
				if (events.lNetworkEvents & FD_WRITE)
				{
					m_triggered |= WAIT_WRITE;
					m_triggered_errors[2] = ConvertMSWErrorCode(events.iErrorCode[FD_WRITE_BIT]);
					m_waiting &= ~WAIT_WRITE;
					matched = true;
				}
			}

			if (matched || !m_waiting)
			{
				return true;
			}
		}
		
#endif
	}

	void SendEvents()
	{
		if (m_triggered & WAIT_READ)
		{
			CSocketEvent evt(m_pSocket->m_id, CSocketEvent::read, m_triggered_errors[1]);
			m_pSocket->m_pEvtHandler->AddPendingEvent(evt);
			m_triggered &= ~WAIT_READ;
		}
		if (m_triggered & WAIT_WRITE)
		{
			CSocketEvent evt(m_pSocket->m_id, CSocketEvent::write, m_triggered_errors[2]);
			m_pSocket->m_pEvtHandler->AddPendingEvent(evt);
			m_triggered &= ~WAIT_WRITE;
		}
	}

	// Call only while locked
	bool IdleLoop()
	{
		if (m_quit)
			return false;
		while (!m_pSocket || (!m_waiting && !m_pHost))
		{
			m_threadwait = true;
			m_condition.Wait();

			if (m_quit)
				return false;
		}
		
		return true;
	}

	virtual ExitCode Entry()
	{
		m_sync.Lock();
		while (true)
		{
			if (!IdleLoop())
			{
				m_sync.Unlock();
				return 0;
			}

			if (!DoConnect())
				continue;

			while (IdleLoop())
			{
				if (!DoWait(0))
					break;
				SendEvents();
			}
		}

		return 0;
	}

	CSocket* m_pSocket;

	char* m_pHost;
	char* m_pPort;

#ifdef __WXMSW__
	// We wait on this using WSAWaitForMultipleEvents
	WSAEVENT m_sync_event;
#endif

	wxMutex m_sync;
	wxCondition m_condition;

	bool m_quit;

	// The socket events we are waiting for
	int m_waiting;

	// The triggered socket events
	int m_triggered;
	int m_triggered_errors[WAIT_EVENTCOUNT];

	// Thread waits for instructions
	bool m_threadwait;
};

CSocket::CSocket(wxEvtHandler* pEvtHandler, int id)
	: m_pEvtHandler(pEvtHandler), m_id(id)
{
	X x;
	m_fd = -1;
	m_state = none;
	m_pSocketThread = 0;

	m_port = 0;
}

CSocket::~CSocket()
{
	if (m_state != none)
		Close();

	if (m_pSocketThread)
		m_pSocketThread->SetSocket(0);
}

int CSocket::Connect(wxString host, unsigned int port)
{
	if (m_state != none)
		return EISCONN;

	if (port < 1 || port > 65535)
		return EINVAL;

	m_state = connecting;

	m_pSocketThread = new CSocketThread();
	m_pSocketThread->SetSocket(this);

	m_host = host;
	m_port = port;
	m_pSocketThread->Connect();

	return EINPROGRESS;
}

void CSocket::SetEventHandler(wxEvtHandler* pEvtHandler, int id)
{
	wxASSERT(m_state == none);
	m_pEvtHandler = pEvtHandler;
	m_id = id;
}

#define ERRORDECL(c, desc) { c, _T(#c), wxTRANSLATE(desc) },

struct Error_table
{
	int code;
	const wxChar* const name;
	const wxChar* const description;
};

static struct Error_table error_table[] =
{
	ERRORDECL(EACCES, "Permission denied")
	ERRORDECL(EADDRINUSE, "Local address in use")
	ERRORDECL(EAFNOSUPPORT, "The specified address family is not supported")
	ERRORDECL(EINPROGRESS, "Operation in progress")
	ERRORDECL(EINVAL, "Invalid argument passed")
	ERRORDECL(EMFILE, "Process file table overflow")
	ERRORDECL(ENFILE, "System limit of open files exceeded")
	ERRORDECL(ENOBUFS, "Out of memory")
	ERRORDECL(ENOMEM, "Out of memory")
	ERRORDECL(EPERM, "Permission denied")
	ERRORDECL(EPROTONOSUPPORT, "Protocol not supported")
	ERRORDECL(EAGAIN, "Resource temporarily unavailable")
	ERRORDECL(EALREADY, "Operation already in progress")
	ERRORDECL(EBADF, "Bad file descriptor")
	ERRORDECL(ECONNREFUSED, "Connection refused by server")
	ERRORDECL(EFAULT, "Socket address outside address space")
	ERRORDECL(EINTR, "Interrupted by signal")
	ERRORDECL(EISCONN, "Socket already connected")
	ERRORDECL(ENETUNREACH, "Network unreachable")
	ERRORDECL(ENOTSOCK, "File descriptior not a socket")
	ERRORDECL(ETIMEDOUT, "Connection attempt timed out")
	ERRORDECL(EHOSTUNREACH, "No route to host")


	// Getaddrinfo related
#ifndef __WXMSW__
	ERRORDECL(EAI_ADDRFAMILY, "Network host does not have any network addresses in the requested address family")
#endif
	ERRORDECL(EAI_AGAIN, "Temporary failure in name resolution")
	ERRORDECL(EAI_BADFLAGS, "Invalid value for ai_flags")
	ERRORDECL(EAI_FAIL, "Nonrecoverable failure in name resolution")
	ERRORDECL(EAI_FAMILY, "The ai_family member is not supported")
	ERRORDECL(EAI_MEMORY, "Memory allocation failure")
	ERRORDECL(EAI_NODATA, "No address associated with nodename")
	ERRORDECL(EAI_NONAME, "Neither nodename nor servname provided, or not known")
	ERRORDECL(EAI_SERVICE, "The servname parameter is not supported for ai_socktype")
	ERRORDECL(EAI_SOCKTYPE, "The ai_socktype member is not supported")
#ifndef __WXMSW__
	ERRORDECL(EAI_SYSTEM, "Other system error")
#endif
	
	// Codes that have no POSIX equivalence
#ifdef __WXMSW__
	ERRORDECL(WSANOTINITIALISED, "Not initialized, need to call WSAStartup")
	ERRORDECL(WSAENETDOWN, "System's network subsystem has failed")
	ERRORDECL(WSAEPROTOTYPE, "Protocol not supported on given socket type")
	ERRORDECL(WSAESOCKTNOSUPPORT, "Socket type not supported for address family")
	ERRORDECL(WSAEADDRNOTAVAIL, "Cannot assign requested address")
#endif
	{ 0, 0, 0 }
};

wxString CSocket::GetErrorString(int error)
{
	for (int i = 0; error_table[i].code; i++)
	{
		if (error != error_table[i].code)
			continue;

		return error_table[i].name;
	}

	return wxString::Format(_T("%d"), error);
}

wxString CSocket::GetErrorDescription(int error)
{
	for (int i = 0; error_table[i].code; i++)
	{
		if (error != error_table[i].code)
			continue;

		return wxString(error_table[i].name) + _T(" - ") + wxGetTranslation(error_table[i].description);
	}

	return wxString::Format(_T("%d"), error);
}

int CSocket::Close()
{
	if (m_pSocketThread)
		m_pSocketThread->SetSocket(0);

	if (m_fd != -1)
	{
		close(m_fd);
		m_fd = -1;
	}
	m_state = none;

	return 0;
}

enum CSocket::SocketState CSocket::GetState()
{
	enum SocketState state;
	if (m_pSocketThread)
		m_pSocketThread->m_sync.Unlock();
	state = m_state;
	if (m_pSocketThread)
		m_pSocketThread->m_sync.Unlock();

	return state;
}

CSocket::Cleanup(bool force)
{
}

int CSocket::Read(void* buffer, unsigned int size, int& error)
{
	int res = recv(m_fd, (char*)buffer, size, 0);

	if (res == -1)
	{
#ifdef __WXMSW__
		error = ConvertMSWErrorCode(WSAGetLastError());
#else
		error = errno;
#endif
		if (error == EGAIN)
		{
			if (m_pSocketThread)
			{
				m_pSocketThread->m_sync.Lock();
				if (!(m_pSocketThread->m_waiting & WAIT_READ))
				{
					m_pSocketThread->m_waiting |= WAIT_READ;
					m_pSocketThread->WakeupThread(true);
				}
				m_pSocketThread->m_sync.Unlock();
			}
		}
	}
	else
		error = 0;

	return res;
}

int CSocket::Write(const void* buffer, unsigned int size, int& error)
{
	int res = send(m_fd, (const char*)buffer, size, 0);

	if (res == -1)
	{
#ifdef __WXMSW__
		error = ConvertMSWErrorCode(WSAGetLastError());
#else
		error = errno;
#endif
		if (error == EGAIN)
		{
			if (m_pSocketThread)
			{
				m_pSocketThread->m_sync.Lock();
				if (!(m_pSocketThread->m_waiting & WAIT_WRITE))
				{
					m_pSocketThread->m_waiting |= WAIT_WRITE;
					m_pSocketThread->WakeupThread(true);
				}
				m_pSocketThread->m_sync.Unlock();
			}
		}
	}
	else
		error = 0;

	return res;
}

wxString CSocket::AddressToString(const struct sockaddr* addr, int addr_len, bool with_port /*=true*/)
{
	int bufflen = NI_MAXHOST;
	char hostbuf[NI_MAXHOST];
	char portbuf[NI_MAXSERV];

	int res = getnameinfo(addr, addr_len, hostbuf, NI_MAXHOST, portbuf, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
	if (res) // Should never fail
		return _T("");

	wxString host = wxString(hostbuf, wxConvLibc);
	wxString port = wxString(portbuf, wxConvLibc);

	// IPv6 uses colons as separator, need to enclose address
	// to avoid ambiguity if also showing port
	if (with_port && addr->sa_family == AF_INET6)
		host = _T("[") + host + _T("]");

	return host + _T(":") + port;
}

wxString CSocket::GetLocalIP() const
{
	struct sockaddr addr;
	socklen_t addr_len = sizeof(addr);
	int res = getsockname(m_fd, &addr, &addr_len);
	if (res)
		return _T("");

	return AddressToString(&addr, addr_len, false);
}

wxString CSocket::GetPeerIP() const
{
	struct sockaddr addr;
	socklen_t addr_len = sizeof(addr);
	int res = getpeername(m_fd, &addr, &addr_len);
	if (res)
		return _T("");

	return AddressToString(&addr, addr_len, false);
}

int CSocket::GetAddressFamily() const
{
	struct sockaddr addr;
	socklen_t addr_len = sizeof(addr);
	int res = getsockname(m_fd, &addr, &addr_len);
	if (res)
		return AF_UNSPEC;

	return addr.sa_family;
}
