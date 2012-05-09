
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "filezilla.h"
#include "iothread.h"

class MockThreadExImpl : public wxThreadExImpl
{
public:
	MockThreadExImpl(wxThreadEx* parent) : wxThreadExImpl(parent) {}

	wxThreadError Create(unsigned int stackSize)
	{
		mock().actualCall("wxThreadEx::Create");
		return wxTHREAD_NO_ERROR;
	}
};

TEST_GROUP(IOThread)
{
};

TEST(IOThread, CreateAThread)
{
	CIOThread iothread;
	MockThreadExImpl* mockImpl = new MockThreadExImpl(&iothread);
	iothread.setThreadImp(mockImpl);

	mock().expectOneCall("wxThreadEx::Create");
	wxFile *file = new wxFile;
	iothread.Create(file, false, false);
}
