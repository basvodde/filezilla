#include "FileZilla.h"


const wxEventType fzEVT_NOTIFICATION = wxNewEventType();

wxFzEvent::wxFzEvent(int id /*=wxID_ANY*/) : wxEvent(id, fzEVT_NOTIFICATION)
{
}

wxEvent *wxFzEvent::Clone() const
{
	return new wxFzEvent(*this);
}

enum NotificationId CLogmsgNotification::GetID() const
{
	return nId_logmsg;
}

enum NotificationId COperationNotification::GetID() const
{
	return nId_operation;
}
