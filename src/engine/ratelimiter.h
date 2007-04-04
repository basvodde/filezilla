#ifndef __RATELIMITER_H__
#define __RATELIMITER_H__

class COptionsBase;

class CRateLimiterObject;

// This class implements a simple rate limiter based on the Token Bucket algorithm.
class CRateLimiter : protected wxEvtHandler
{
public:
	static CRateLimiter* Create(COptionsBase *pOptions);
	void Free();

	enum rate_direction
	{
		inbound,
		outbound
	};

	void AddObject(CRateLimiterObject* pObject, enum rate_direction direction);
	void RemoveObject(CRateLimiterObject* pObject);
	void RemoveObject(CRateLimiterObject* pObject, enum rate_direction direction);

protected:
	CRateLimiter(COptionsBase* pOptions);
	virtual ~CRateLimiter();

	std::list<CRateLimiterObject*> m_objectLists[2];
	std::list<CRateLimiterObject*> m_wakeupList[2];

	wxTimer m_timer;

	static CRateLimiter *m_pTheRateLimiter;
	unsigned int m_usageCount;

	int m_tokenDebt[2];

	COptionsBase* m_pOptions;

	void WakeupWaitingObjects();

	DECLARE_EVENT_TABLE();
	void OnTimer(wxTimerEvent& event);
};

class CRateLimiterObject
{
	friend class CRateLimiter;

public:
	CRateLimiterObject();
	int GetAvailableBytes() const { return m_bytesAvailable; }

protected:
	void UpdateUsage(int usedBytes);
	void Wait();

	virtual void OnRateAvailable() { }

private:
	bool m_waiting;
	int m_bytesAvailable;
};

#endif //__RATELIMITER_H__
