// FileZillaEngine.cpp: Implementierung der Klasse CFileZillaEngine.
//
//////////////////////////////////////////////////////////////////////

#include "filezilla.h"
#include "FileZillaEngine.h"
#include "filezilla.h"
#include "ControlSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CFileZillaEngine::CFileZillaEngine()
{
	m_pEventHandler = 0;
	m_pControlSocket = 0;
}

CFileZillaEngine::~CFileZillaEngine()
{

}

int CFileZillaEngine::Init(wxEvtHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;

	return FZ_REPLY_OK;
}
