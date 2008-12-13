#ifndef __LOGGING_PRIVATE_H__
#define __LOGGING_PRIVATE_H__

class CLogging
{
public:
	CLogging(CFileZillaEnginePrivate *pEngine);
	virtual ~CLogging();

	void LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const;
	void LogMessageRaw(MessageType nMessageType, const wxChar *msg) const;
	void LogMessage(wxString sourceFile, int nSourceLine, void *pInstance, MessageType nMessageType, const wxChar *msgFormat, ...) const;

private:
	CFileZillaEnginePrivate *m_pEngine;

	void InitLogFile() const;
	void LogToFile(MessageType nMessageType, const wxString& msg) const;

	static bool m_logfile_initialized;
#ifdef __WXMSW__
	static HANDLE m_log_fd;
#else
	static int m_log_fd;
#endif
	static wxString m_prefixes[MessageTypeCount];
	static unsigned int m_pid;
	static int m_max_size;
	static wxString m_file;

	static int m_refcount;
};

#endif
