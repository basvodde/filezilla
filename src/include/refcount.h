#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

// Trivial template class to refcount objects with COW semantics.

template<class T> class CRefcountObject
{
public:
	CRefcountObject();
	CRefcountObject(const CRefcountObject<T>& v);
	virtual ~CRefcountObject();

	void clear();

	T& Get();

	const T& operator*() const;
	const T* operator->() const;

	bool operator==(const CRefcountObject<T>& cmp) const;
	inline bool operator!=(const CRefcountObject<T>& cmp) const { return !(*this == cmp); }
	bool operator<(const CRefcountObject<T>& cmp) const;

	CRefcountObject<T>& operator=(const CRefcountObject<T>& v);
private:
	int* m_refcount;
	T* m_ptr;
};

template<class T> bool CRefcountObject<T>::operator==(const CRefcountObject<T>& cmp) const
{
	if (m_ptr == cmp.m_ptr)
		return true;

	return *m_ptr == *cmp.m_ptr;
}

template<class T> CRefcountObject<T>::CRefcountObject()
{
	m_refcount = new int(1);
	
	m_ptr = new T;
}

template<class T> CRefcountObject<T>::CRefcountObject(const CRefcountObject<T>& v)
{
	m_refcount = v.m_refcount;
	(*m_refcount)++;
	m_ptr = v.m_ptr;
}

template<class T> CRefcountObject<T>::~CRefcountObject()
{
	if ((*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}
}

template<class T> T& CRefcountObject<T>::Get()
{
	if (*m_refcount != 1)
	{
		(*m_refcount)--;
		m_refcount = new int(1);
		T* ptr = new T(*m_ptr);
		m_ptr = ptr;
	}

	return *m_ptr;
}

template<class T> CRefcountObject<T>& CRefcountObject<T>::operator=(const CRefcountObject<T>& v)
{
	if (m_ptr == v.m_ptr)
		return *this;
	if ((*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}

	m_refcount = v.m_refcount;
	(*m_refcount)++;
	m_ptr = v.m_ptr;
	return *this;
}

template<class T> bool CRefcountObject<T>::operator<(const CRefcountObject<T>& cmp) const
{
	if (m_ptr == cmp.m_ptr)
		return false;

	return *m_ptr < *cmp.m_ptr;
}

template<class T> void CRefcountObject<T>::clear()
{
	if (*m_refcount != 1)
	{
		(*m_refcount)--;
		m_refcount = new int(1);
	}
	else
		delete m_ptr;
	m_ptr = new T;
}

template<class T> const T& CRefcountObject<T>::operator*() const
{
	return *m_ptr;
}

template<class T> const T* CRefcountObject<T>::operator->() const
{
	return m_ptr;
}


#endif //__REFCOUNT_H__