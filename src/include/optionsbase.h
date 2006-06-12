#ifndef __OPTIONSBASE_H__
#define __OPTIONSBASE_H__

// The engine of FileZilla 3 can be configured using a few settings.
// In order to read and set the settings, the engine has to be passed
// a pointer to a COptionsBase object.
// Since COptionsBase is virtual, the user of the engine has to create a
// derived class which handles settings-reading and writing.

#define OPTIONS_ENGINE_NUM 10

#define	OPTION_USEPASV 0			// Use passive mode unless overridden by 
									// server settings
#define OPTION_LIMITPORTS 1
#define OPTION_LIMITPORTS_LOW 2
#define OPTION_LIMITPORTS_HIGH 3
#define OPTION_EXTERNALIPMODE 4		/* External IP Address mode for use in active mode
									   Values: 0: ask operating system
									           1: use provided IP
											   2: use provided resolver
								    */
#define OPTION_EXTERNALIP 5
#define OPTION_EXTERNALIPRESOLVER 6
#define OPTION_NOEXTERNALONLOCAL 7	// Don't use external IP address if connection is
									// coming from a local unroutable address
#define OPTION_PASVREPLYFALLBACKMODE 8
#define OPTION_TIMEOUT 9

class COptionsBase
{
public:
	inline virtual ~COptionsBase() {};
	virtual int GetOptionVal(unsigned int nID) = 0;
	virtual wxString GetOption(unsigned int nID) = 0;

	virtual bool SetOption(unsigned int nID, int value) = 0;
	virtual bool SetOption(unsigned int nID, wxString value) = 0;
};

#endif
