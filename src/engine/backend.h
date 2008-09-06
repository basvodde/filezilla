#ifndef __BACKEND_H__
#define __BACKEND_H__

#include "ratelimiter.h"
#include "socket.h"

class CBackend : public CRateLimiterObject
{
public:
	CBackend(CSocketEventHandler* pEvtHandler);
	virtual ~CBackend() {}

	virtual int Read(void *buffer, unsigned int size, int& error) = 0;
	virtual int Peek(void *buffer, unsigned int size, int& error) = 0;
	virtual int Write(const void *buffer, unsigned int size, int& error) = 0;

	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction) = 0;

protected:
	CSocketEventHandler* const m_pEvtHandler;
};

class CSocket;
class CSocketBackend : public CBackend, public CSocketEventSource
{
public:
	CSocketBackend(CSocketEventHandler* pEvtHandler, CSocket* pSocket);
	virtual ~CSocketBackend();
	// Backend definitions
	virtual int Read(void *buffer, unsigned int size, int& error);
	virtual int Peek(void *buffer, unsigned int size, int& error);
	virtual int Write(const void *buffer, unsigned int size, int& error);

protected:
	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction);

	CSocket* m_pSocket;
};

#endif //__BACKEND_H__
