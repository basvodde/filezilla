#ifndef __LOGGING_H__
#define __LOGGING_H__

enum MessageType
{
	Status,
	Error,
	Command,
	Response,
	Debug_Warning,
	Debug_Info,
	Debug_Verbose,
	Debug_Debug,

	RawList
};

#endif

