
#include "CppUTest/TestHarness.h"
#include "wx/thread.h"

wxThread::~wxThread()
{
}

bool wxThread::TestDestroy()
{
	FAIL("wxThread::TestDestroy called");
	return false;
}

wxCriticalSection::~wxCriticalSection()
{
}
