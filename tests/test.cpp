#include <iostream>
#include <wx/wx.h>
#include <wx/app.h>

int main(int argc, char* argv[])
{
	if (!wxInitialize())
	{
		std::cout << "Failed to initialize wxWidgets" << std::endl;
		return 1;
	}

	wxUninitialize();
	return 0;
}
