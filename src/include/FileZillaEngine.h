#ifndef __FILEZILLAENGINE_H__
#define __FILEZILLAENGINE_H__

enum EngineNotificationType
{
	engineCancel,
	engineHostresolve,
	engineTransferEnd
};

#include "engineprivate.h"

class CFileZillaEngine : public CFileZillaEnginePrivate
{
public:
	CFileZillaEngine();
	virtual ~CFileZillaEngine();

	// Initialize the engine. Pass over the event handler that should receive notification
	// events. (Defined in notification.h)
	int Init(wxEvtHandler *pEventHandler, COptionsBase *pOptions);

	// TODO: Init function with a function pointer for a callback function for 
	// notifications. Not all users of the engine use wxWidgets.

    // Execute the given command. See commands.h for a list of the available commands.
	int Command(const CCommand &command);

	bool IsBusy() const;
	bool IsConnected() const;
	
	// IsActive returns true only if data has been transferred (recv)
	// or sent (!recv) since the last time IsActive was called with
	// the same argument.
	bool IsActive(bool recv);

	// Returns the next pending notification.
	// Has to be called until it returns 0 each time you get the
	// pending notifications event, or you'll lose notifications.
	CNotification* GetNextNotification();

	const CCommand *GetCurrentCommand() const;
	enum Command GetCurrentCommandId() const;

	// Sets the reply to an async request, e.g. a file exists request.
	bool IsPendingAsyncRequestReply(const CAsyncRequestNotification *pNotification);
	bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);
	
	// Get a progress update about the current transfer. changed will be set
	// to true if the data has been updated compared to the last time
	// GetTransferStatus was called.
	bool GetTransferStatus(CTransferStatus &status, bool &changed);
};

#endif
