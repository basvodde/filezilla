
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "filezilla.h"
#include "iothread.h"

wxThreadError wxThread::Create(unsigned int stackSize)
{
	mock().actualCall("wxThread::Create");
}

TEST_GROUP(IOThread)
{
};

TEST(IOThread, CreateAThread)
{
	mock().expectOneCall("wxThread::Create");
	wxFile *file = new wxFile;
	CIOThread iothread;
	iothread.Create(file, false, false);
}
