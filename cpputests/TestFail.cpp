
#include "CppUTest/TestHarness.h"

TEST_GROUP(fail)
{
};

TEST(fail, fail)
{
	FAIL("FAIL");
}
