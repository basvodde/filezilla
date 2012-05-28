
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "FileZillaEngine.h"

const wxEventTable CFileZillaEnginePrivate::sm_eventTable = wxEventTable();
wxEventHashTable CFileZillaEnginePrivate::sm_eventHashTable (CFileZillaEnginePrivate::sm_eventTable);

const wxEventTable* CFileZillaEnginePrivate::GetEventTable() const
{
	FAIL("CFileZillaEnginePrivate::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& CFileZillaEnginePrivate::GetEventHashTable() const
{
	FAIL("CFileZillaEnginePrivate::GetEventHashTable");
	return sm_eventHashTable;
}

CFileZillaEnginePrivate::CFileZillaEnginePrivate()
{
	FAIL("CFileZillaEnginePrivate::CFileZillaEnginePrivate");
}

CFileZillaEnginePrivate::~CFileZillaEnginePrivate()
{
	FAIL("CFileZillaEnginePrivate::~CFileZillaEnginePrivate");
}



CFileZillaEngine::CFileZillaEngine()
{
	FAIL("CFileZillaEngine::CFileZillaEngine");
}

CFileZillaEngine::~CFileZillaEngine()
{
	FAIL("CFileZillaEngine::~CFileZillaEngine");
}

int CFileZillaEngine::CacheLookup(const CServerPath& path, CDirectoryListing& listing)
{
	FAIL("CFileZillaEngine::CacheLookup");
	return 0;
}

int CFileZillaEngine::Command(const CCommand &command)
{
	FAIL("CFileZillaEngine::Command");
	return 0;
}

CNotification* CFileZillaEngine::GetNextNotification()
{
	FAIL("CFileZillaEngine::GetNextNotification");
	return NULL;
}

bool CFileZillaEngine::GetTransferStatus(CTransferStatus &status, bool &changed)
{
	FAIL("CFileZillaEngine::GetTransferStatus");
	return true;
}

int CFileZillaEngine::Init(wxEvtHandler *pEventHandler, COptionsBase *pOptions)
{
	FAIL("CFileZillaEngine::Init");
	return 0;
}

bool CFileZillaEngine::IsActive(enum _direction direction)
{
	FAIL("CFileZillaEngine::IsActive");
	return true;
}

bool CFileZillaEngine::IsBusy() const
{
	FAIL("CFileZillaEngine::IsBusy");
	return true;
}

bool CFileZillaEngine::IsConnected() const
{
	FAIL("CFileZillaEngine::IsConnected");
	return true;
}

bool CFileZillaEngine::IsPendingAsyncRequestReply(const CAsyncRequestNotification *pNotification)
{
	FAIL("CFileZillaEngine::IsPendingAsyncRequestReply");
	return true;
}

bool CFileZillaEngine::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	FAIL("CFileZillaEngine::SetAsyncRequestReply");
	return true;
}

