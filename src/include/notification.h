#ifndef __NOTIFICATION_H__
#define __NOTIFICATION_H__

extern const wxEventType fzEVT_NOTIFICATION;
#define EVT_FZ_NOTIFICATION(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_NOTIFICATION, id, -1, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

class wxFzEvent : public wxEvent
{
public:
	wxFzEvent(int id = wxID_ANY);
	virtual wxEvent *Clone() const;
};

enum NotificationId
{
	nId_logmsg, // notification about new messages for the message log
	nId_operation, // operation reply codes
	nId_connection, // connection information: connects, disconnects, timeouts etc..
	nId_transfers // transfer information: bytes transferes, transfer speed and such
};

class CNotification
{
public:
	virtual enum NotificationId GetID() const = 0;
};

class CLogmsgNotification : public CNotification
{
public:
	virtual enum NotificationId GetID() const;

	wxString msg;
	enum MessageType msgType;
};

class COperationNotification : public CNotification
{
public:
	virtual enum NotificationId GetID() const;
	
	int nReplyCode;
};

#endif
