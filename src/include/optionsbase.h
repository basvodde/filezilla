#ifndef __OPTIONSBASE_H__
#define __OPTIONSBASE_H__

#define OPTIONS_ENGINE_NUM 5

#define	OPTION_USEPASV 0
#define OPTION_LIMITPORTS 1
#define OPTION_LIMITPORTS_LOW 2
#define OPTION_LIMITPORTS_HIGH 3
#define OPTION_EXTERNALIP 4

class COptionsBase
{
public:
	virtual int GetOptionVal(unsigned int nID) = 0;
	virtual wxString GetOption(unsigned int nID) = 0;

	virtual bool SetOption(unsigned int nID, int value) = 0;
	virtual bool SetOption(unsigned int nID, wxString value) = 0;
};

#endif
