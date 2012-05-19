
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/timer.h"

wxTimerBase::~wxTimerBase()
{
	FAIL("wxTimerBase::~wxTimerBase");
}

void wxTimerBase::Notify()
{
	FAIL("wxTimerBase::Notify");
}

bool wxTimerBase::Start(int milliseconds, bool oneShot)
{
	FAIL("wxTimerBase::Start");
	return true;
}

wxTimer::~wxTimer()
{
	FAIL("wxTimer::~wxTimer");
}

void wxTimer::Stop()
{
	FAIL("wxTimer::Stop");
}

wxClassInfo *wxTimer::GetClassInfo() const
{
	FAIL("wxTimer::GetClassInfo");
	return NULL;
}

bool wxTimer::Start(int milliseconds,bool one_shot)
{
	FAIL("wxTimer::Start");
	return true;
}

bool wxTimer::IsRunning() const
{
	FAIL("wxTimer::IsRunning");
	return true;
}
