#include "FileZilla.h"
#include "Options.h"

COptions::COptions()
{
}

COptions::~COptions()
{
}

int COptions::GetOptionVal(int nID)
{
	return 0;
}

wxString COptions::GetOption(int nID)
{
	return _T("");
}

bool COptions::SetOption(int nID, int value)
{
	return true;
}

bool COptions::SetOption(int nID, wxString value)
{
	return true;
}
