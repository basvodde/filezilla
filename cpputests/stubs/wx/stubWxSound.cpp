
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/sound.h"

wxSound::wxSound(const wxString& fileName, bool isResource)
{
	FAIL("wxSound::wxSound");
}

wxSound::~wxSound()
{
	FAIL("wxSound::~wxSound");
}

bool wxSound::DoPlay(unsigned flags) const
{
	FAIL("wxSound::DoPlay");
	return true;
}
