#ifndef __BACKEND_H__
#define __BACKEND_H__

#include "ratelimiter.h"

class CSocketEventHandler;
class CBackend : public CRateLimiterObject
{
public:
	CBackend(CSocketEventHandler* pEvtHandler);
	virtual ~CBackend() {}

	virtual int Read(void *buffer, unsigned int size, int& error) = 0;
	virtual int Peek(void *buffer, unsigned int size, int& error) = 0;
	virtual int Write(const void *buffer, unsigned int size, int& error) = 0;

	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction) = 0;

	int GetId() const { return m_Id; }

	static int GetNextId();

protected:
	CSocketEventHandler* const m_pEvtHandler;

private:
	int m_Id;

	// Initialized with 0, incremented each time
	// a new instance is created
	// Mainly needed for CHttpControlSockets if server sends a redirects.
	// Otherwise, lingering events from the previous connection will cause
	// problems
	static int m_nextId;
};

class CSocket;
class CSocketBackend : public CBackend
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
