#include <libfilezilla.h>
#include <iostream>
#include <string>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <locale.h>
#include <wx/init.h>
	
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	if (!wxInitialize())
	{
		std::cout << "Failed to initialize wxWidgets" << std::endl;
		return 1;
	}

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest(registry.makeTest());
	bool wasSuccessful = runner.run("", false);

	wxUninitialize();
	return wasSuccessful ? 0 : 1;
}
