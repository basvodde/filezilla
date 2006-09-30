#ifndef __OPTIONSBASE_H__
#define __OPTIONSBASE_H__

// The engine of FileZilla 3 can be configured using a few settings.
// In order to read and set the settings, the engine has to be passed
// a pointer to a COptionsBase object.
// Since COptionsBase is virtual, the user of the engine has to create a
// derived class which handles settings-reading and writing.

enum engineOptions
{
	OPTION_USEPASV,			// Use passive mode unless overridden by 
							// server settings
	OPTION_LIMITPORTS,
	OPTION_LIMITPORTS_LOW,
	OPTION_LIMITPORTS_HIGH,
	OPTION_EXTERNALIPMODE,		/* External IP Address mode for use in active mode
								   Values: 0: ask operating system
								           1: use provided IP
										   2: use provided resolver
							    */
	OPTION_EXTERNALIP,
	OPTION_EXTERNALIPRESOLVER,
	OPTION_NOEXTERNALONLOCAL,	// Don't use external IP address if connection is
								// coming from a local unroutable address
	OPTION_PASVREPLYFALLBACKMODE,
	OPTION_TIMEOUT,
	OPTION_LOGGING_DEBUGLEVEL,
	OPTION_LOGGING_RAWLISTING,

	OPTION_FZSFTP_EXECUTABLE,	// full path to fzsftp executable

	OPTIONS_ENGINE_NUM
};

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
