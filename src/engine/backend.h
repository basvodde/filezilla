#ifndef __BACKEND_H__
#define __BACKEND_H__

#include "ratelimiter.h"

class CBackend : public CRateLimiterObject
{
public:
	CBackend(wxEvtHandler* pEvtHandler);
	virtual ~CBackend() {}
	virtual void Read(void *data, unsigned int len) = 0;
	virtual void Write(const void *data, unsigned int len) = 0;
	virtual bool Error() const = 0;
	virtual int LastError() const = 0;
	virtual unsigned int LastCount() const = 0;
	virtual void Peek(void *buffer, unsigned int len) = 0;

	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction) = 0;

	int GetId() const { return m_Id; }

protected:
	wxEvtHandler* const m_pEvtHandler;

private:
	int m_Id;

	// Initialized with 0, incremented each time
	// a new instance is created
	// Mainly needed for CHttpControlSockets if server sends a redirects.
	// Otherwise, lingering events from the previous connection will cause
	// problems
	static int m_nextId;
};

class CSocketBackend : public CBackend
{
public:
	CSocketBackend(wxEvtHandler* pEvtHandler, wxSocketBase* pSocket);
	virtual ~CSocketBackend();
	// Backend definitions
	virtual void Read(void *buffer, unsigned int len);
	virtual void Write(const void *buffer, unsigned int len);
	virtual bool Error() const { return m_error; }
	virtual unsigned int LastCount() const { return m_lastCount; }
	virtual int LastError() const { return m_lastError; }
	virtual void Peek(void *buffer, unsigned int len);

protected:
	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction);

	void UpdateResults();
	wxSocketBase* m_pSocket;

	bool m_error;
	int m_lastCount;
	int m_lastError;
};

#endif //__BACKEND_H__
