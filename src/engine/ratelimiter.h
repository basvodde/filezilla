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

	void AddObject(CRateLimiterObject* pObject);
	void RemoveObject(CRateLimiterObject* pObject);

protected:
	int GetBucketSize() const;

	CRateLimiter(COptionsBase* pOptions);
	virtual ~CRateLimiter();

	std::list<CRateLimiterObject*> m_objectList;
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
	int GetAvailableBytes(enum CRateLimiter::rate_direction direction) const { return m_bytesAvailable[direction]; }

protected:
	void UpdateUsage(enum CRateLimiter::rate_direction direction, int usedBytes);
	void Wait(enum CRateLimiter::rate_direction direction);

	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction) { }

private:
	bool m_waiting[2];
	int m_bytesAvailable[2];
};

#endif //__RATELIMITER_H__
