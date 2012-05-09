
#include "CppUTest/TestHarness.h"
#include "filezilla.h"
#include "socket.h"

TEST_GROUP(socket)
{
};

TEST(socket, disconnectWillCallTheStandardCClose)
{
	CSocket sock(NULL);
}
