#include "filezilla.h"

#include "logging_private.h"

#include <wx/log.h>

#include <errno.h>

bool CLogging::m_logfile_initialized = false;
#ifdef __WXMSW__
HANDLE CLogging::m_log_fd = INVALID_HANDLE_VALUE;
#else
int CLogging::m_log_fd = -1;
#endif
wxString CLogging::m_prefixes[MessageTypeCount];
unsigned int CLogging::m_pid;
int CLogging::m_max_size;
wxString CLogging::m_file;

int CLogging::m_refcount = 0;

CLogging::CLogging(CFileZillaEnginePrivate *pEngine)
{
	m_pEngine = pEngine;
	m_refcount++;
}

CLogging::~CLogging()
{
	m_refcount--;

	if (!m_refcount)
	{
#ifdef __WXMSW__
		if (m_log_fd != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_log_fd);
			m_log_fd = INVALID_HANDLE_VALUE;
		}
#else
		if (m_log_fd != -1)
		{
			close(m_log_fd);
			m_log_fd = -1;
		}
#endif
		m_logfile_initialized = false;
	}
}

void CLogging::LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const
{
	InitLogFile();

	const int debugLevel = m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL);
	switch (nMessageType)
	{
	case Debug_Warning:
		if (!debugLevel)
			return;
		break;
	case Debug_Info:
		if (debugLevel < 2)
			return;
		break;
	case Debug_Verbose:
		if (debugLevel < 3)
			return;
		break;
	case Debug_Debug:
		if (debugLevel != 4)
			return;
		break;
	case RawList:
		if (!m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_RAWLISTING))
			return;
		break;
	default:
		break;
	}

	va_list ap;
    
    va_start(ap, msgFormat);
	wxString text = wxString::FormatV(msgFormat, ap);
	va_end(ap);

	LogToFile(nMessageType, text);

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = text;
	m_pEngine->AddNotification(notification);
}

void CLogging::LogMessageRaw(MessageType nMessageType, const wxChar *msg) const
{
	InitLogFile();

	const int debugLevel = m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL);
	switch (nMessageType)
	{
	case Debug_Warning:
		if (!debugLevel)
			return;
		break;
	case Debug_Info:
		if (debugLevel < 2)
			return;
		break;
	case Debug_Verbose:
		if (debugLevel < 3)
			return;
		break;
	case Debug_Debug:
		if (debugLevel != 4)
			return;
		break;
	case RawList:
		if (!m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_RAWLISTING))
			return;
		break;
	default:
		break;
	}

	LogToFile(nMessageType, msg);

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = msg;
	m_pEngine->AddNotification(notification);
}

void CLogging::LogMessage(wxString SourceFile, int nSourceLine, void *pInstance, MessageType nMessageType, const wxChar *msgFormat, ...) const
{
	InitLogFile();

	const int debugLevel = m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL);
	switch (nMessageType)
	{
	case Debug_Warning:
		if (!debugLevel)
			return;
		break;
	case Debug_Info:
		if (debugLevel < 2)
			return;
		break;
	case Debug_Verbose:
		if (debugLevel < 3)
			return;
		break;
	case Debug_Debug:
		if (debugLevel != 4)
			return;
		break;
	default:
		break;
	}

	int pos = SourceFile.Find('\\', true);
	if (pos != -1)
		SourceFile = SourceFile.Mid(pos+1);

	pos = SourceFile.Find('/', true);
	if (pos != -1)
		SourceFile = SourceFile.Mid(pos+1);

	va_list ap;
    
	va_start(ap, msgFormat);
	wxString text = wxString::FormatV(msgFormat, ap);
	va_end(ap);

	wxString msg = wxString::Format(_T("%s(%d): %s   caller=%p"), SourceFile.c_str(), nSourceLine, text.c_str(), this);

	LogToFile(nMessageType, msg);

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = msg;
	m_pEngine->AddNotification(notification);
}

void CLogging::InitLogFile() const
{
	if (m_logfile_initialized)
		return;

	m_logfile_initialized = true;

	m_file = m_pEngine->GetOptions()->GetOption(OPTION_LOGGING_FILE);
	if (m_file == _T(""))
		return;

#ifdef __WXMSW__
	m_log_fd = CreateFile(m_file, FILE_APPEND_DATA, FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (m_log_fd == INVALID_HANDLE_VALUE)
#else
	m_log_fd = open(m_file.fn_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (m_log_fd == -1)
#endif
	{
		LogMessage(Error, _("Could not open log file: %s"), wxSysErrorMsg());
		return;
	}

	m_prefixes[Status] = _("Status:");
	m_prefixes[Error] = _("Error:");
	m_prefixes[Command] = _("Command:");
	m_prefixes[Response] = _("Response:");
	m_prefixes[Debug_Warning] = _("Trace:");
	m_prefixes[Debug_Info] = m_prefixes[Debug_Warning];
	m_prefixes[Debug_Verbose] = m_prefixes[Debug_Warning];
	m_prefixes[Debug_Debug] = m_prefixes[Debug_Warning];
	m_prefixes[RawList] = _("Listing:");

	m_pid = wxGetProcessId();

	m_max_size = m_pEngine->GetOptions()->GetOptionVal(OPTION_LOGGING_FILE_SIZELIMIT);
	if (m_max_size < 0)
		m_max_size = 0;
	else if (m_max_size > 2000)
		m_max_size = 2000;
	m_max_size *= 1024 * 1024;
}

void CLogging::LogToFile(MessageType nMessageType, const wxString& msg) const
{
#ifdef __WXMSW__
	if (m_log_fd == INVALID_HANDLE_VALUE)
		return;
#else
	if (m_log_fd == -1)
		return;
#endif

	wxDateTime now = wxDateTime::Now();
	wxString out(wxString::Format(_T("%s %u %d %s %s")
#ifdef __WXMSW__
		_T("\r\n"),
#else
		_T("\n"),
#endif
		now.Format(_T("%Y-%m-%d %H:%M:%S")).c_str(), m_pid, m_pEngine->GetEngineId(), m_prefixes[nMessageType].c_str(), msg.c_str()));

	const wxWX2MBbuf utf8 = out.mb_str(wxConvUTF8);
	if (utf8)
	{
#ifdef __WXMSW__
		if (m_max_size)
		{
			LARGE_INTEGER size;
			if (!GetFileSizeEx(m_log_fd, &size) || size.QuadPart > m_max_size)
			{
				CloseHandle(m_log_fd);

				// m_log_fd might no longer be the original file.
				// Recheck on a new handle. Proteced with a mutex against other processes
				HANDLE hMutex = ::CreateMutex(0, true, _T("FileZilla 3 Logrotate Mutex"));

				HANDLE hFile = CreateFile(m_file, FILE_APPEND_DATA, FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					wxString error = wxSysErrorMsg();

					// Oh dear..
					ReleaseMutex(hMutex);
					CloseHandle(hMutex);

					m_log_fd = INVALID_HANDLE_VALUE;
					LogMessage(Error, _("Could not open log file: %s"), error.c_str());
					return;
				}

				wxString error;
				if (GetFileSizeEx(hFile, &size) && size.QuadPart > m_max_size)
				{
					CloseHandle(hFile);
					
					// MoveFileEx can fail if trying to access a deleted file for which another process still has
					// a handle. Move it far away first.
					// Todo: Handle the case in which logdir and tmpdir are on different volumes.
					// (Why is everthing so needlessly complex on MSW?)
					wxString tmp = wxFileName::CreateTempFileName(_T("fz3"));
					MoveFileEx(m_file + _T(".1"), tmp, MOVEFILE_REPLACE_EXISTING);
					DeleteFile(tmp);
					MoveFileEx(m_file, m_file + _T(".1"), MOVEFILE_REPLACE_EXISTING);
					m_log_fd = CreateFile(m_file, FILE_APPEND_DATA, FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					if (m_log_fd == INVALID_HANDLE_VALUE)
					{
						// If this function would return bool, I'd return FILE_NOT_FOUND here.
						error = wxSysErrorMsg();
					}
				}
				else
					m_log_fd = hFile;

				ReleaseMutex(hMutex);
				CloseHandle(hMutex);

				if (error != _T(""))
					LogMessage(Error, _("Could not open log file: %s"), error.c_str());
			}
		}
		DWORD len = (DWORD)strlen((const char*)utf8);
		DWORD written;
		BOOL res = WriteFile(m_log_fd, (const char*)utf8, len, &written, 0);
		if (!res || written != len)
		{
			LogMessage(Error, _("Could not write to log file: %s"), wxSysErrorMsg());
			CloseHandle(m_log_fd);
			m_log_fd = INVALID_HANDLE_VALUE;
		}
#else
		if (m_max_size)
		{
			struct stat buf;
			int rc = fstat(m_log_fd, &buf);
			while (!rc && buf.st_size > 10000)//m_max_size)
			{
				struct flock lock = {0};
				lock.l_type = F_WRLCK;
				lock.l_whence = SEEK_SET;
				lock.l_start = 0;
				lock.l_len = 1;

				int rc;

				// Retry through signals
				while ((rc = fcntl(m_log_fd, F_SETLKW, &lock)) == -1 && errno == EINTR);

				// Ignore any other failures
				int fd = open(m_file.fn_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
				if (fd == -1)
				{
					wxString error = wxSysErrorMsg();

					close(m_log_fd);
					m_log_fd = -1;

					LogMessage(Error, error);
					return;
				}
				struct stat buf2;
				rc = fstat(fd, &buf2);

				// Different files
				if (!rc && buf.st_ino != buf2.st_ino)
				{
					close(m_log_fd); // Releases the lock
					m_log_fd = fd;
					buf = buf2;
					continue;
				}

				// The file is indeed the log file and we are holding a lock on it.

				// Rename it
				rc = rename(m_file.fn_str(), (m_file + _T(".1")).fn_str());
				close(m_log_fd);
				close(fd);

				// Get the new file
				m_log_fd = open(m_file.fn_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
				if (m_log_fd == -1)
				{
					LogMessage(Error, wxSysErrorMsg());
					return;
				}

				if (!rc) // Rename didn't fail
					rc = fstat(m_log_fd, &buf);
			}
		}
		size_t len = strlen((const char*)utf8);
		size_t written = write(m_log_fd, (const char*)utf8, len);
		if (written != len)
		{
			close(m_log_fd);
			m_log_fd = -1;
			LogMessage(Error, _("Could not write to log file: %s"), wxSysErrorMsg());
		}
#endif
	}
}
