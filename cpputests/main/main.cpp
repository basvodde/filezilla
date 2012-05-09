
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTestExt/MockSupportPlugin.h"

int main(int argc, char** argv)
{
	MockSupportPlugin mockPlugin;
	TestRegistry::getCurrentRegistry()->installPlugin(&mockPlugin);
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
