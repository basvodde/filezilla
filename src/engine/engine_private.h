#ifndef __ENGINE_PRIVATE_H__
#define __ENGINE_PRIVATE_H__

#include "logging_private.h"
#include "ControlSocket.h"
#include "asynchostresolver.h"
#include "FileZillaEngine.h"

class wxFzEngineEvent : public wxEvent
{
public:
	wxFzEngineEvent(int id, enum EngineNotificationType eventType);
	virtual wxEvent *Clone() const;

	enum EngineNotificationType m_eventType;
};

extern const wxEventType fzEVT_ENGINE_NOTIFICATION;
typedef void (wxEvtHandler::*fzEngineEventFunction)(wxFzEngineEvent&);

#define EVT_FZ_ENGINE_NOTIFICATION(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_ENGINE_NOTIFICATION, id, -1, \
        (wxObjectEventFunction)(fzEngineEventFunction) wxStaticCastEvent( fzEngineEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#endif
