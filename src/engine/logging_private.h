#ifndef __LOGGING_PRIVATE_H__
#define __LOGGING_PRIVATE_H__

class CLogging
{
public:
	CLogging(CFileZillaEnginePrivate *pEngine);

	void LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const;
	void LogMessageRaw(MessageType nMessageType, const wxChar *msg) const;
	void LogMessage(wxString sourceFile, int nSourceLine, void *pInstance, MessageType nMessageType, const wxChar *msgFormat, ...) const;

private:
	CFileZillaEnginePrivate *m_pEngine;
};

#endif
