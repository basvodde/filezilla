#ifndef __OPTIONSBASE_H__
#define __OPTIONSBASE_H__

// The engine of FileZilla 3 can be configured using a few settings.
// In order to read and set the settings, the engine has to be passed
// a pointer to a COptionsBase object.
// Since COptionsBase is virtual, the user of the engine has to create a
// derived class which handles settings-reading and writing.

#define OPTIONS_ENGINE_NUM 5

#define	OPTION_USEPASV 0			// Use passive mode unless overridden by 
									// server settings
#define OPTION_LIMITPORTS 1
#define OPTION_LIMITPORTS_LOW 2
#define OPTION_LIMITPORTS_HIGH 3
#define OPTION_EXTERNALIP 4			// External IP Address for use in active mode

class COptionsBase
{
public:
	virtual int GetOptionVal(unsigned int nID) = 0;
	virtual wxString GetOption(unsigned int nID) = 0;

	virtual bool SetOption(unsigned int nID, int value) = 0;
	virtual bool SetOption(unsigned int nID, wxString value) = 0;
};

#endif
