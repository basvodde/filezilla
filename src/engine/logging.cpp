#include "FileZilla.h"
#include "logging_private.h"

CLogging::CLogging(CFileZillaEnginePrivate *pEngine)
{
	m_pEngine = pEngine;
}

void CLogging::LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const
{
	va_list ap;
    
    va_start(ap, msgFormat);
	wxString text = wxString::FormatV(msgFormat, ap);
	va_end(ap);

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = text;
	m_pEngine->AddNotification(notification);
}

void CLogging::LogMessage(wxString SourceFile, int nSourceLine, void *pInstance, MessageType nMessageType, const wxChar *msgFormat, ...) const
{
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

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = msg;
	m_pEngine->AddNotification(notification);
}
