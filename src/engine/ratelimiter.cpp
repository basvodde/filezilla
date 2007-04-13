#include "FileZilla.h"
#include "ratelimiter.h"

static const int tickDelay = 250;

CRateLimiter* CRateLimiter::m_pTheRateLimiter = 0;

BEGIN_EVENT_TABLE(CRateLimiter, wxEvtHandler)
EVT_TIMER(wxID_ANY, CRateLimiter::OnTimer)
END_EVENT_TABLE()

CRateLimiter::CRateLimiter(COptionsBase* pOptions) :
	m_usageCount(1),
	m_pOptions(pOptions)
{
	m_timer.SetOwner(this);
	m_tokenDebt[0] = 0;
	m_tokenDebt[1] = 0;
}

CRateLimiter::~CRateLimiter()
{
}

CRateLimiter* CRateLimiter::Create(COptionsBase* pOptions)
{
	if (!m_pTheRateLimiter)
		m_pTheRateLimiter = new CRateLimiter(pOptions);
	else
		m_pTheRateLimiter->m_usageCount++;
	
	return m_pTheRateLimiter;
}

CRateLimiter* CRateLimiter::Get()
{
	return m_pTheRateLimiter;
}

void CRateLimiter::Free()
{
	wxASSERT(m_pTheRateLimiter);
	if (!m_pTheRateLimiter)
		return;

	wxASSERT(this == m_pTheRateLimiter);
	if (m_usageCount <= 1)
	{
		delete this;
		m_pTheRateLimiter = 0;
	}
	else
		m_usageCount--;
}

void CRateLimiter::AddObject(CRateLimiterObject* pObject)
{
	m_objectList.push_back(pObject);

	for (int i = 0; i < 2; i++)
	{
		int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + i) * 1024;
		if (limit > 0)
		{
			int tokens = limit * tickDelay / 1000;

			tokens /= m_objectList.size();
			if (m_tokenDebt[i] > 0)
			{
				if (tokens >= m_tokenDebt[i])
				{
					tokens -= m_tokenDebt[i];
					m_tokenDebt[i] = 0;
				}
				else
				{
					tokens = 0;
					m_tokenDebt[i] -= tokens;
				}
			}

			pObject->m_bytesAvailable[i] = tokens;
		}
		else
			pObject->m_bytesAvailable[i] = -1;


		if (!m_timer.IsRunning())
			m_timer.Start(tickDelay, false);
	}
}

void CRateLimiter::RemoveObject(CRateLimiterObject* pObject)
{
	for (std::list<CRateLimiterObject*>::iterator iter = m_objectList.begin(); iter != m_objectList.end(); iter++)
	{
		if (*iter == pObject)
		{
			for (int i = 0; i < 2; i++)
			{
				// If an object already used up some of its assigned tokens, add them to m_tokenDebt,
				// so that newly created objects get less initial tokens.
				// That ensures that rapidly adding and removing objects does not exceed the rate
				int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + i) * 1024;
				int tokens = limit * tickDelay / 1000;
				tokens /= m_objectList.size();
				if ((*iter)->m_bytesAvailable[i] < tokens)
					m_tokenDebt[i] += tokens - (*iter)->m_bytesAvailable[i];

			}
			m_objectList.erase(iter);
			break;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		for (std::list<CRateLimiterObject*>::iterator iter = m_wakeupList[i].begin(); iter != m_wakeupList[i].end(); iter++)
		{
			if (*iter == pObject)
			{
				m_wakeupList[i].erase(iter);
				break;
			}
		}
	}
}

void CRateLimiter::OnTimer(wxTimerEvent& event)
{
	std::list<CRateLimiterObject*> objectsToUnwait;
	for (int i = 0; i < 2; i++)
	{
		m_tokenDebt[i] = 0;

		if (!m_objectList.size())
			continue;

		int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + i) * 1024;
		if (!limit)
		{
			for (std::list<CRateLimiterObject*>::iterator iter = m_objectList.begin(); iter != m_objectList.end(); iter++)
			{
				(*iter)->m_bytesAvailable[i] = -1;
				if ((*iter)->m_waiting[i])
					m_wakeupList[i].push_back(*iter);
			}
			continue;
		}

		int tokens = limit * tickDelay / 1000;
		if (!tokens)
			tokens = 1;
		int maxTokens = tokens * GetBucketSize();

		// Get amount of tokens for each object
		int tokensPerObject = tokens / m_objectList.size();
		
		if (!tokensPerObject)
			tokensPerObject = 1;
		tokens = 0;

		// This list will hold all objects which didn't reach maxTokens
		std::list<CRateLimiterObject*> unsaturatedObjects;

		for (std::list<CRateLimiterObject*>::iterator iter = m_objectList.begin(); iter != m_objectList.end(); iter++)
		{
			if ((*iter)->m_bytesAvailable[i] == -1)
			{
				wxASSERT(!(*iter)->m_waiting[i]);
				(*iter)->m_bytesAvailable[i] = tokensPerObject;
				unsaturatedObjects.push_back(*iter);
			}
			else
			{
				(*iter)->m_bytesAvailable[i] += tokensPerObject;
				if ((*iter)->m_bytesAvailable[i] > maxTokens)
				{
					tokens += (*iter)->m_bytesAvailable[i] - maxTokens;
					(*iter)->m_bytesAvailable[i] = maxTokens;
				}
				else
					unsaturatedObjects.push_back(*iter);

				if ((*iter)->m_waiting[i])
					m_wakeupList[i].push_back(*iter);
			}
		}

		// If there are any left-over tokens (in case of objects with a rate below the limit)
		// assign to the unsaturated sources
		while (tokens && !unsaturatedObjects.empty())
		{
			tokensPerObject = tokens / unsaturatedObjects.size();
			if (!tokensPerObject)
				break;
			tokens = 0;

			std::list<CRateLimiterObject*> objects;
			objects.swap(unsaturatedObjects);

			for (std::list<CRateLimiterObject*>::iterator iter = objects.begin(); iter != objects.end(); iter++)
			{
				(*iter)->m_bytesAvailable[i] += tokensPerObject;
				if ((*iter)->m_bytesAvailable[i] > maxTokens)
				{
					tokens += (*iter)->m_bytesAvailable[i] - maxTokens;
					(*iter)->m_bytesAvailable[i] = maxTokens;
				}
				else
					unsaturatedObjects.push_back(*iter);
			}
		}
	}
	WakeupWaitingObjects();

	if (m_objectList.empty())
		m_timer.Stop();
}

void CRateLimiter::WakeupWaitingObjects()
{
	for (int i = 0; i < 2; i++)
	{
		while (!m_wakeupList[i].empty())
		{
			CRateLimiterObject* pObject = m_wakeupList[i].front();
			m_wakeupList[i].pop_front();
			if (!pObject->m_waiting[i])
				continue;
			
			wxASSERT(pObject->m_bytesAvailable != 0);
			pObject->m_waiting[i] = false;

			pObject->OnRateAvailable((rate_direction)i);
		}
	}
}

int CRateLimiter::GetBucketSize() const
{
	const int burst_tolerance = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_BURSTTOLERANCE);

	int bucket_size = 1000 / tickDelay;
	switch (burst_tolerance)
	{
	case 1:
		bucket_size *= 2;
		break;
	case 2:
		bucket_size *= 5;
		break;
	default:
		break;
	}

	return bucket_size;
}

CRateLimiterObject::CRateLimiterObject()
{
	for (int i = 0; i < 2; i++)
	{
		m_waiting[i] = false;
		m_bytesAvailable[i] = -1;
	}
}

void CRateLimiterObject::UpdateUsage(enum CRateLimiter::rate_direction direction, int usedBytes)
{
	wxASSERT(usedBytes <= m_bytesAvailable[direction]);
	if (usedBytes > m_bytesAvailable[direction])
		m_bytesAvailable[direction] = 0;
	else
		m_bytesAvailable[direction] -= usedBytes;
}

void CRateLimiterObject::Wait(enum CRateLimiter::rate_direction direction)
{
	wxASSERT(m_bytesAvailable[direction] == 0);
	m_waiting[direction] = true;
}
