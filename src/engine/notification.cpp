#include "FileZilla.h"

const wxEventType fzEVT_NOTIFICATION = wxNewEventType();

wxFzEvent::wxFzEvent(int id /*=wxID_ANY*/) : wxEvent(id, fzEVT_NOTIFICATION)
{
}

wxEvent *wxFzEvent::Clone() const
{
	return new wxFzEvent(*this);
}

CNotification::CNotification()
{
}

CNotification::~CNotification()
{
}

CLogmsgNotification::CLogmsgNotification()
{
}

CLogmsgNotification::~CLogmsgNotification()
{
}

enum NotificationId CLogmsgNotification::GetID() const
{
	return nId_logmsg;
}

COperationNotification::COperationNotification()
{
}

COperationNotification::~COperationNotification()
{
}

enum NotificationId COperationNotification::GetID() const
{
	return nId_operation;
}

CDirectoryListingNotification::CDirectoryListingNotification(CDirectoryListing *pDirectoryListing)
{
	this->pDirectoryListing = pDirectoryListing;
}

CDirectoryListingNotification::~CDirectoryListingNotification()
{
	delete pDirectoryListing;
}

enum NotificationId CDirectoryListingNotification::GetID() const
{
	return nId_listing;
}

const CDirectoryListing *CDirectoryListingNotification::GetDirectoryListing() const
{
	return pDirectoryListing;
}
