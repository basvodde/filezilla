#include "FileZilla.h"
#include "logging_private.h"

CLogging::CLogging(CFileZillaEnginePrivate *pEngine)
{
	m_pEngine = pEngine;
}

void CLogging::LogMessage(MessageType nMessageType, const wxChar *msgFormat, ...) const
{
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
	}

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

	CLogmsgNotification *notification = new CLogmsgNotification;
	notification->msgType = nMessageType;
	notification->msg = msg;
	m_pEngine->AddNotification(notification);
}
