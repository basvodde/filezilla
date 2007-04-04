#include "FileZilla.h"
#include "ratelimiter.h"

static const int tickDelay = 250;
static int bucketSize = 1000 / tickDelay;

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

void CRateLimiter::AddObject(CRateLimiterObject* pObject, enum rate_direction direction)
{
	m_objectLists[direction].push_back(pObject);

	int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + direction);
	if (limit > 0)
	{
		int tokens = limit * tickDelay / 1000;

		tokens /= m_objectLists[direction].size();
		if (m_tokenDebt[direction] > 0)
		{
			if (tokens >= m_tokenDebt[direction])
			{
				tokens -= m_tokenDebt[direction];
				m_tokenDebt[direction] = 0;
			}
			else
			{
				tokens = 0;
				m_tokenDebt[direction] -= tokens;
			}
		}

		pObject->m_bytesAvailable = tokens;
	}
	else
		pObject->m_bytesAvailable = -1;


	if (!m_timer.IsRunning())
		m_timer.Start(tickDelay, false);
}

void CRateLimiter::RemoveObject(CRateLimiterObject* pObject)
{
	RemoveObject(pObject, inbound);
	RemoveObject(pObject, outbound);
}

void CRateLimiter::RemoveObject(CRateLimiterObject* pObject, enum rate_direction direction)
{
	for (std::list<CRateLimiterObject*>::iterator iter = m_objectLists[direction].begin(); iter != m_objectLists[direction].end(); iter++)
	{
		if (*iter == pObject)
		{
			// If an object already used up some of its assigned tokens, add them to m_tokenDebt,
			// so that newly created objects get less initial tokens.
			// That ensures that rapidly adding and removing objects does not exceed the rate
			int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + direction);
			int tokens = limit * tickDelay / 1000;
			tokens /= m_objectLists[direction].size();
			if ((*iter)->m_bytesAvailable < tokens)
				m_tokenDebt[direction] += tokens - (*iter)->m_bytesAvailable;

			m_objectLists[direction].erase(iter);
			break;
		}
	}

	for (std::list<CRateLimiterObject*>::iterator iter = m_wakeupList[direction].begin(); iter != m_wakeupList[direction].end(); iter++)
	{
		if (*iter == pObject)
		{
			m_objectLists[direction].erase(iter);
			break;
		}
	}
}

void CRateLimiter::OnTimer(wxTimerEvent& event)
{
	std::list<CRateLimiterObject*> objectsToUnwait;
	for (int i = 0; i < 2; i++)
	{
		m_tokenDebt[i] = 0;

		int limit = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND + i);
		if (!limit)
		{
			for (std::list<CRateLimiterObject*>::iterator iter = m_objectLists[i].begin(); iter != m_objectLists[i].end(); iter++)
			{
				(*iter)->m_bytesAvailable = -1;
				if ((*iter)->m_waiting)
					m_wakeupList[i].push_back(*iter);
			}
			m_tokenDebt[i] = 0;
			continue;
		}

		int tokens = limit * tickDelay / 1000;
		if (!tokens)
			tokens = 1;
		int maxTokens = tokens * bucketSize;

		// Get amount of tokens for each object
		int tokensPerObject = tokens / m_objectLists[i].size();
		
		if (!tokensPerObject)
			tokensPerObject = 1;
		tokens = 0;

		// This list will hold all objects which didn't reach maxTokens
		std::list<CRateLimiterObject*> unsaturatedObjects;

		for (std::list<CRateLimiterObject*>::iterator iter = m_objectLists[i].begin(); iter != m_objectLists[i].end(); iter++)
		{
			if ((*iter)->m_bytesAvailable == -1)
			{
				wxASSERT(!(*iter)->m_waiting);
				(*iter)->m_bytesAvailable = tokensPerObject;
				unsaturatedObjects.push_back(*iter);
			}
			else
			{
				(*iter)->m_bytesAvailable += tokensPerObject;
				if ((*iter)->m_bytesAvailable > maxTokens)
				{
					tokens += (*iter)->m_bytesAvailable - maxTokens;
					(*iter)->m_bytesAvailable = maxTokens;
				}
				else
					unsaturatedObjects.push_back(*iter);

				if ((*iter)->m_waiting)
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
				(*iter)->m_bytesAvailable += tokensPerObject;
				if ((*iter)->m_bytesAvailable > maxTokens)
				{
					tokens += (*iter)->m_bytesAvailable - maxTokens;
					(*iter)->m_bytesAvailable = maxTokens;
				}
				else
					unsaturatedObjects.push_back(*iter);
			}
		}
	}
	WakeupWaitingObjects();

			
	if (m_objectLists[0].empty() && m_objectLists[1].empty())
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
			if (!pObject->m_waiting)
				continue;
			
			wxASSERT(pObject->m_bytesAvailable != 0);
			pObject->m_waiting = false;

			pObject->OnRateAvailable();
		}
	}
}

CRateLimiterObject::CRateLimiterObject() :
	m_waiting(false),
	m_bytesAvailable(-1)
{
}

void CRateLimiterObject::UpdateUsage(int usedBytes)
{
	wxASSERT(usedBytes <= m_bytesAvailable);
	if (usedBytes > m_bytesAvailable)
		m_bytesAvailable = 0;
	else
		m_bytesAvailable -= usedBytes;
}

void CRateLimiterObject::Wait()
{
	wxASSERT(m_bytesAvailable == 0);
	m_waiting = true;
}
