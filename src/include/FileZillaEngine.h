#pragma once

class CControlSocket;
class CFileZillaEngine  
{
public:
	CFileZillaEngine();
	virtual ~CFileZillaEngine();

	int Init(wxEvtHandler *pEventHandler);

	int Command(const CCommand &command);

protected:
	wxEvtHandler *m_pEventHandler;
	CControlSocket *m_pControlSocket;
};