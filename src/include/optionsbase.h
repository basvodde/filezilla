#pragma once

#define OPTIONS_ENGINE_NUM = 1

#define	OPTION_USEPASV 1

class COptionsBase
{
public:
	virtual int GetOptionVal(int nID) = 0;
	virtual wxString GetOption(int nID) = 0;

	virtual bool SetOption(int nID, int value) = 0;
	virtual bool SetOption(int nID, wxString value) = 0;
};
