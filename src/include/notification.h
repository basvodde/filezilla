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
	nId_transfers, // transfer information: bytes transferes, transfer speed and such
	nId_listing, // directory listings
	nId_asyncrequest // asynchronous request
};

class CNotification
{
public:
	CNotification();
	virtual ~CNotification();
	virtual enum NotificationId GetID() const = 0;
};

class CLogmsgNotification : public CNotification
{
public:
	CLogmsgNotification();
	virtual ~CLogmsgNotification();
	virtual enum NotificationId GetID() const;

	wxString msg;
	enum MessageType msgType;
};

class COperationNotification : public CNotification
{
public:
	COperationNotification();
	virtual ~COperationNotification();
	virtual enum NotificationId GetID() const;
	
	int nReplyCode;
	enum Command commandId;
};

class CDirectoryListing;
class CDirectoryListingNotification : public CNotification
{
public:
	CDirectoryListingNotification(CDirectoryListing *pDirectoryListing);
	virtual ~CDirectoryListingNotification();
	virtual enum NotificationId GetID() const;
	const CDirectoryListing *GetDirectoryListing() const;

protected:
	CDirectoryListing *pDirectoryListing;
};

enum RequestId
{
	reqId_fileexists
};

class CAsyncRequestNotification : public CNotification
{
public:
	CAsyncRequestNotification();
	virtual ~CAsyncRequestNotification();
	virtual enum NotificationId GetID() const;

	virtual enum RequestId GetRequestID() const = 0;

	unsigned int requestNumber;
};

class CFileExistsNotification : public CAsyncRequestNotification
{
public:
	CFileExistsNotification();
	virtual ~CFileExistsNotification();
	virtual enum RequestId GetRequestID() const;

	bool download;

	wxString localFile;
	wxLongLong localSize;
	wxDateTime localTime;

	wxString remoteFile;
	CServerPath remotePath;
	wxLongLong remoteSize;
	wxDateTime remoteTime;
};

#endif
