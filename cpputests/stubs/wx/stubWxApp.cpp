
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/app.h"
#include "wx/window.h"
#include "wx/vidmode.h"

int wxAppBase::OnExit()
{
	FAIL("wxAppBase::OnExit");
	return 0;
}

int wxAppBase::MainLoop()
{
	FAIL("wxAppBase::MainLoop");
	return 0;
}

wxVideoMode wxAppBase::GetDisplayMode() const
{
	FAIL("wxAppBase::GetDisplayMode");
	return wxVideoMode();
}

bool wxAppBase::OnCmdLineParsed(wxCmdLineParser& parser)
{
	FAIL("wxAppBase::OnCmdLineParsed");
	return true;
}

wxAppTraits *wxAppBase::CreateTraits()
{
	FAIL("wxAppBase::CreateTraits");
	return NULL;
}

bool wxAppBase::ProcessIdle()
{
	FAIL("wxAppBase::ProcessIdle");
	return true;
}

bool wxAppBase::Dispatch()
{
	FAIL("wxAppBase::Dispatch");
	return true;
}

bool wxAppBase::Pending()
{
	FAIL("wxAppBase::Pending");
	return true;
}

void wxAppBase::SetActive(bool isActive, wxWindow *lastFocus)
{
	FAIL("wxAppBase::SetActive");
}

wxLayoutDirection wxAppBase::GetLayoutDirection() const
{
	FAIL("wxAppBase::GetLayoutDirection");
	return wxLayoutDirection();
}

bool wxAppBase::SendIdleEvents(wxWindow* win, wxIdleEvent& event)
{
	FAIL("wxAppBase::SendIdleEvents");
	return true;
}

bool wxAppBase::OnExceptionInMainLoop()
{
	FAIL("wxAppBase::OnExceptionInMainLoop");
	return true;
}

void wxAppBase::OnInitCmdLine(wxCmdLineParser& parser)
{
	FAIL("wxAppBase::OnInitCmdLine");
}

void wxAppBase::ExitMainLoop()
{
	FAIL("wxAppBase::ExitMainLoop");
}

void wxAppBase::Exit()
{
	FAIL("wxAppBase::Exit");
}

wxAppBase::~wxAppBase()
{
	FAIL("wxAppBase::~wxAppBase");
}

wxWindow *wxAppBase::GetTopWindow() const
{
	FAIL("wxAppBase::GetTopWindow");
	return NULL;
}

int wxAppBase::OnRun()
{
	FAIL("wxAppBase::OnRun");
	return 0;
}

bool wxAppBase::OnInitGui()
{
	FAIL("wxAppBase::OnInitGui");
	return true;
}

void wxAppBase::CleanUp()
{
	FAIL("wxAppBase::CleanUp");
}

bool wxAppBase::Initialize(int& argc, wxChar **argv)
{
	FAIL("wxAppBase::Initialize");
	return true;
}


const wxEventTable wxApp::sm_eventTable = wxEventTable();
wxEventHashTable wxApp::sm_eventHashTable (wxApp::sm_eventTable);

wxClassInfo *wxApp::GetClassInfo() const
{
	FAIL("wxApp::GetClassInfo");
	return NULL;
}

short wxApp::MacHandleAEODoc(const WXAPPLEEVENTREF event , WXAPPLEEVENTREF reply)
{
	FAIL("wxApp::MacHandleAEODoc");
	return 0;
}

const wxEventTable* wxApp::GetEventTable() const
{
	FAIL("wxApp::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxApp::GetEventHashTable() const
{
	FAIL("wxApp::GetEventHashTable");
	return sm_eventHashTable;
}


bool wxApp::Initialize(int& argc, wxChar **argv)
{
	FAIL("wxWindowBase::Initialize");
	return true;
}

bool wxApp::Yield(bool onlyIfNeeded)
{
	FAIL("wxWindowBase::Initialize");
	return true;
}

short wxApp::MacHandleAEOApp(const WXAPPLEEVENTREF event , WXAPPLEEVENTREF reply)
{
	FAIL("wxWindowBase::MacHandleAEOApp");
	return 0;
}

void wxApp::MacNewFile()
{
	FAIL("wxWindowBase::MacNewFile");
}

void wxApp::MacReopenApp()
{
	FAIL("wxWindowBase::MacReopenApp");
}

short wxApp::MacHandleAEPDoc(const WXAPPLEEVENTREF event , WXAPPLEEVENTREF reply)
{
	FAIL("wxWindowBase::MacHandleAEPDoc");
	return 0;
}

void wxApp::WakeUpIdle()
{
	FAIL("wxWindowBase::WakeUpIdle");
}

void wxApp::MacPrintFile(const wxString &fileName)
{
	FAIL("wxWindowBase::MacPrintFile");
}

void wxApp::CleanUp()
{
	FAIL("wxWindowBase::CleanUp");
}

short wxApp::MacHandleAERApp(const WXAPPLEEVENTREF event , WXAPPLEEVENTREF reply)
{
	FAIL("wxWindowBase::MacHandleAERApp");
	return 0;
}

short wxApp::MacHandleAEQuit(const WXAPPLEEVENTREF event , WXAPPLEEVENTREF reply)
{
	FAIL("wxWindowBase::MacHandleAEQuit");
	return 0;
}

void wxApp::MacHandleUnhandledEvent( WXEVENTREF ev )
{
	FAIL("wxWindowBase::MacHandleUnhandledEvent");
}

bool wxApp::OnInitGui()
{
	FAIL("wxWindowBase::OnInitGui");
	return false;
}

void wxApp::MacOpenFile(const wxString &fileName)
{
	FAIL("wxWindowBase::MacOpenFile");
}
