#include "FileZilla.h"
#include "iothread.h"

const wxEventType fzEVT_IOTHREAD = wxNewEventType();

CIOThreadEvent::CIOThreadEvent(int id /*=wxID_ANY*/)
	: wxEvent(id, fzEVT_IOTHREAD)
{
}

wxEvent* CIOThreadEvent::Clone() const
{
	return new CIOThreadEvent(GetId());
}

CIOThread::CIOThread()
	: wxThreadEx(wxTHREAD_JOINABLE), m_evtHandler(0), m_pFile(0), m_condition(m_mutex)
{
	for (unsigned int i = 0; i < BUFFERCOUNT; i++)
	{
		m_buffers[i] = new char[BUFFERSIZE];
		m_bufferLens[i] = 0;
	}

	m_error = false;
	m_running = false;
	m_appWaiting = false;
	m_threadWaiting = false;
	m_destroyed = false;
	m_wasCarriageReturn = false;
	m_error_description = 0;
}

CIOThread::~CIOThread()
{
	if (m_pFile)
		delete m_pFile;

	for (unsigned int i = 0; i < BUFFERCOUNT; i++)
		delete [] m_buffers[i];

	delete [] m_error_description;
}

bool CIOThread::Create(wxFile* pFile, bool read, bool binary)
{
	wxASSERT(pFile);
	if (m_pFile)
		delete m_pFile;
	m_pFile = pFile;
	m_read = read;
	m_binary = binary;

	if (read)
	{
		m_curAppBuf = BUFFERCOUNT - 1;
		m_curThreadBuf = 0;
	}
	else
	{
		m_curAppBuf = -1;
		m_curThreadBuf = 0;
	}

	m_running = true;
	wxThreadEx::Create();
	wxThreadEx::Run();

	return true;
}

wxThreadEx::ExitCode CIOThread::Entry()
{
	if (m_read)
	{
		while (m_running)
		{
			int len = ReadFromFile(m_buffers[m_curThreadBuf], BUFFERSIZE);

			wxMutexLocker locker(m_mutex);

			if (m_appWaiting)
			{
				if (!m_evtHandler)
				{
					m_running = false;
					break;
				}
				m_appWaiting = false;
				CIOThreadEvent evt;
				wxPostEvent(m_evtHandler, evt);
			}

			if (len == wxInvalidOffset)
			{
				m_error = true;
				m_running = false;
				break;
			}

			m_bufferLens[m_curThreadBuf] = len;

			if (!len)
			{
				m_running = false;
				break;
			}

			++m_curThreadBuf %= BUFFERCOUNT;
			if (m_curThreadBuf == m_curAppBuf)
			{
				if (!m_running)
					break;

				m_threadWaiting = true;
				if (m_running)
					m_condition.Wait();
			}
		}
	}
	else
	{
		m_mutex.Lock();
		if (m_curAppBuf == -1)
		{
			if (!m_running)
			{
				m_mutex.Unlock();
				return 0;
			}
			else
			{
				m_threadWaiting = true;
				m_condition.Wait();
			}
		}
		m_mutex.Unlock();

		while (true)
		{
			m_mutex.Lock();
			while (m_curThreadBuf == m_curAppBuf)
			{
				if (!m_running)
				{
					m_mutex.Unlock();
					return 0;
				}
				m_threadWaiting = true;
				m_condition.Wait();
			}
			m_mutex.Unlock();

			bool writeSuccessful = WriteToFile(m_buffers[m_curThreadBuf], m_bufferLens[m_curThreadBuf]);

			wxMutexLocker locker(m_mutex);

			if (!writeSuccessful)
			{
				m_error = true;
				m_running = false;
			}

			if (m_appWaiting)
			{
				if (!m_evtHandler)
				{
					m_running = false;
					break;
				}
				m_appWaiting = false;
				CIOThreadEvent evt;
				wxPostEvent(m_evtHandler, evt);
			}

			if (m_error)
				break;

			++m_curThreadBuf %= BUFFERCOUNT;
		}
	}

	return 0;
}

int CIOThread::GetNextWriteBuffer(char** pBuffer, int len /*=BUFFERSIZE*/)
{
	wxASSERT(!m_destroyed);

	wxMutexLocker locker(m_mutex);

	if (m_error)
		return IO_Error;

	if (m_curAppBuf == -1)
	{
		m_curAppBuf = 0;
		*pBuffer = m_buffers[0];
		return IO_Success;
	}

	m_bufferLens[m_curAppBuf] = len;

	int newBuf = (m_curAppBuf + 1) % BUFFERCOUNT;
	if (newBuf == m_curThreadBuf)
	{
		m_appWaiting = true;
		return IO_Again;
	}

	if (m_threadWaiting)
	{
		m_condition.Signal();
		m_threadWaiting = false;
	}

	m_curAppBuf = newBuf;
	*pBuffer = m_buffers[newBuf];

	return IO_Success;
}

bool CIOThread::Finalize(int len)
{
	wxASSERT(m_pFile);

	if (m_destroyed)
		return true;

	Destroy();

	if (m_curAppBuf == -1)
		return true;

	if (m_error)
		return false;

	if (!len)
		return true;

	if (!WriteToFile(m_buffers[m_curAppBuf], len))
		return false;

#ifndef __WXMSW__
	if (!m_binary && m_wasCarriageReturn)
	{
		const char CR = '\r';
		if (m_pFile->Write(&CR, 1) != 1)
			return false;
	}
#endif
	return true;
}

int CIOThread::GetNextReadBuffer(char** pBuffer)
{
	wxASSERT(!m_destroyed);
	wxASSERT(m_read);

	int newBuf = (m_curAppBuf + 1) % BUFFERCOUNT;

	wxMutexLocker locker(m_mutex);

	if (newBuf == m_curThreadBuf)
	{
		if (m_error)
			return IO_Error;
		else if (!m_running)
			return IO_Success;
		else
		{
			m_appWaiting = true;
			return IO_Again;
		}
	}

	if (m_threadWaiting)
	{
		m_condition.Signal();
		m_threadWaiting = false;
	}

	*pBuffer = m_buffers[newBuf];
	m_curAppBuf = newBuf;

	return m_bufferLens[newBuf];
}

void CIOThread::Destroy()
{
	if (m_destroyed)
		return;
	m_destroyed = true;

	m_mutex.Lock();

	m_running = false;
	if (m_threadWaiting)
	{
		m_threadWaiting = false;
		m_condition.Signal();
	}
	m_mutex.Unlock();

	Wait();
}

int CIOThread::ReadFromFile(char* pBuffer, int maxLen)
{
	// In binary mode, no conversion has to be done.
	// Also, under Windows the native newline format is already identical
	// to the newline format of the FTP protocol
#ifndef __WXMSW__
	if (m_binary)
#endif
		return m_pFile->Read(pBuffer, maxLen);

#ifndef __WXMSW__

	// In the worst case, length will doubled: If reading
	// only LFs from the file
	const int readLen = maxLen / 2;

	char* r = pBuffer + readLen;
	int len = m_pFile->Read(r, readLen);
	if (!len || len == wxInvalidOffset)
		return len;

	const char* const end = r + len;
	char* w = pBuffer;

	// Convert all stand-alone LFs into CRLF pairs.
	while (r != end)
	{
		char c = *r++;
		if (c == '\n')
		{
			if (!m_wasCarriageReturn)
				*w++ = '\r';
			m_wasCarriageReturn = false;
		}
		else if (c == '\r')
			m_wasCarriageReturn = true;
		else
			m_wasCarriageReturn = false;

		*w++ = c;
	}

	return w - pBuffer;
#endif
}

bool CIOThread::WriteToFile(char* pBuffer, int len)
{
	// In binary mode, no conversion has to be done.
	// Also, under Windows the native newline format is already identical
	// to the newline format of the FTP protocol
#ifndef __WXMSW__
	if (m_binary)
	{
#endif
		return DoWrite(pBuffer, len);
#ifndef __WXMSW__
	}
	else
	{
		// On all CRLF pairs, omit the CR. Don't harm stand-alone CRs
		// I assume disk access is buffered, otherwise the 1 byte writes are
		// going to hurt performance.
		const char CR = '\r';
		const char* const end = pBuffer + len;
		for (char* r = pBuffer; r != end; r++)
		{
			char c = *r;
			if (c == '\r')
				m_wasCarriageReturn = true;
			else if (c == '\n')
			{
				m_wasCarriageReturn = false;
				if (!DoWrite(&c, 1))
					return false;
			}
			else
			{
				if (m_wasCarriageReturn)
				{
					m_wasCarriageReturn = false;
					if (!DoWrite(&CR, 1))
						return false;
				}

				if (!DoWrite(&c, 1))
					return false;
			}
		}
		return true;
	}
#endif
}

bool CIOThread::DoWrite(const char* pBuffer, int len)
{
	int fd = m_pFile->fd();
	if (wxWrite(fd, pBuffer, len) == len)
		return true;

	int code = wxSysErrorCode();

	const wxString error = wxSysErrorMsg(code);

	wxMutexLocker locker(m_mutex);
	delete [] m_error_description;
	m_error_description = new wxChar[error.Len() + 1];
	wxStrcpy(m_error_description, error);

	return false;
}

wxString CIOThread::GetError()
{
	wxMutexLocker locker(m_mutex);
	if (!m_error_description)
		return _T("");

	return wxString(m_error_description);
}
