
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "filezilla.h"
#include "socket.h"

class CSocketForTesting : public CSocket
{
public:
	CSocketForTesting() : CSocket(NULL) {}

	void setSocketDescriptor(int fd)
	{
		m_fd = fd;
	}
};

TEST_GROUP(socket)
{
};

int stubbed_close(int fildes)
{
	mock().actualCall("close").withParameter("fildes", fildes);
}

TEST(socket, disconnectWillCallTheStandardCClose)
{
	UT_PTR_SET(stdc_close, stubbed_close);

	int fileDescriptor = 10;
	mock().expectOneCall("close").withParameter("fildes", fileDescriptor);
	CSocketForTesting sock;
	sock.setSocketDescriptor(fileDescriptor);
	sock.Close();
}
