#pragma once

class COptions : public COptionsBase
{
public:
	COptions();
	virtual ~COptions();

	virtual int GetOptionVal(int nID);
	virtual wxString GetOption(int nID);

	virtual bool SetOption(int nID, int value);
	virtual bool SetOption(int nID, wxString value);
};
