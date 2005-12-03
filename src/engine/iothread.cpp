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
	: wxThread(wxTHREAD_JOINABLE), m_evtHandler(0), m_pFile(0), m_condition(m_mutex)
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
}

CIOThread::~CIOThread()
{
	if (m_pFile)
		delete m_pFile;

	for (unsigned int i = 0; i < BUFFERCOUNT; i++)
		delete [] m_buffers[i];
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
	wxThread::Create();
	wxThread::Run();

	return true;
}

wxThread::ExitCode CIOThread::Entry()
{
	if (m_read)
	{
		while (m_running)
		{
			int len = m_pFile->Read(m_buffers[m_curThreadBuf], BUFFERSIZE);

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
		if (!m_running)
		{
			m_mutex.Unlock();
			return 0;
		}
		if (m_curAppBuf == -1)
		{
			m_threadWaiting = true;
			m_condition.Wait();
		}

		if (!m_running)
		{
			m_mutex.Unlock();
			return 0;
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

			int written = m_pFile->Write(m_buffers[m_curThreadBuf], m_bufferLens[m_curThreadBuf]);

			wxMutexLocker locker(m_mutex);

			if (written != (int)m_bufferLens[m_curThreadBuf])
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

	int written = m_pFile->Write(m_buffers[m_curAppBuf], len);
	if (written != len)
		return false;

	return true;
}

int CIOThread::GetNextReadBuffer(char** pBuffer)
{
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

/* Convert ascii net to local ascii
#ifndef __WXMSW__
			if (!m_binaryMode)
			{
				// On systems other than Windows, we have to convert the line endings from
				// CRLF into LFs only. We do this by skipping every CR.
				int i = 0;
				while (i < numread && m_pTransferBuffer[i] != '\r')
					i++;
				int j = i;
				while (i < numread)
				{
					if (m_pTransferBuffer[i] != '\r')
						m_pTransferBuffer[j++] = m_pTransferBuffer[i];
					i++;
				}
				m_pTransferBuffer += j;
				m_transferBufferLen -= j;
			}
			else
#endif
*/

/* convert local ascii to net ascii
if (m_transferBufferPos * 2 < BUFFERSIZE)
		{
			if (pData->pFile)
			{
				wxFileOffset numread;
#ifndef __WXMSW__
				// Again, on  non-Windows systems perform ascii file conversion. This
				// time we add CRs
				if (!m_binaryMode)
				{
					int numToRead = (BUFFERSIZE - m_transferBufferPos) / 2;
					char* tmp = new char[numToRead];
					numread = pData->pFile->Read(tmp, numToRead);
					if (numread < numToRead)
					{
						delete pData->pFile;
						pData->pFile = 0;
					}

					char *p = m_pTransferBuffer + m_transferBufferPos;
					for (int i = 0; i < numread; i++)
					{
						char c = tmp[i];
						if (c == '\n')
							*p++ = '\r';
						*p++ = c;
					}
					delete [] tmp;
					numread = p - (m_pTransferBuffer + m_transferBufferPos);
				}
				else
#endif
				{
					numread = pData->pFile->Read(m_pTransferBuffer + m_transferBufferPos, BUFFERSIZE - m_transferBufferPos);
					if (numread < BUFFERSIZE - m_transferBufferPos)
					{
						delete pData->pFile;
						pData->pFile = 0;
					}
				}

				if (numread > 0)
					m_transferBufferPos += numread;
				
				if (numread < 0)
				{
					m_pControlSocket->LogMessage(::Error, _("Can't read from file"));
					TransferEnd(1);
					return;
				}
				else if (!numread && !m_transferBufferPos)
				{
					TransferEnd(0);
					return;
				}
			}
			else if (!m_transferBufferPos)
			{
				TransferEnd(0);
				return;
			}
		}
*/

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
