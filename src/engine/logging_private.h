#ifndef __LOGGING_PRIVATE_H__
#define __LOGGING_PRIVATE_H__

class CLogging
{
public:
	CLogging(CFileZillaEngine *pEngine);

	void LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const;
	void LogMessage(wxString sourceFile, int nSourceLine, void *pInstance, MessageType nMessageType, const wxChar *msgFormat, ...) const;

private:
	CFileZillaEngine *m_pEngine;
};

#endif
