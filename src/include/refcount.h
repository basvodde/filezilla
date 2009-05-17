#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

// Simple shared pointer. Takes ownership of passed regular pointers
template<class T> class CSharedPointer
{
public:
	CSharedPointer();
	CSharedPointer(const CSharedPointer<T>& v);
	CSharedPointer(T *v);
	~CSharedPointer();

	void clear();

	T& operator*() const;
	T* operator->() const;

	T* Value() const {return m_ptr; } // Promise to NEVER EVER call delete on the returned value

	bool operator==(const CSharedPointer<T>& cmp) const { return m_ptr == cmp.m_ptr; }
	inline bool operator!=(const CSharedPointer<T>& cmp) const { m_ptr != cmp.m_ptr; }
	bool operator<(const CSharedPointer<T>& cmp) const { return m_ptr < cmp.m_ptr; }

	// Magic for CSharedPointer foo(bar); if (foo) ...
	// aka safe bool idiom
	typedef void (CSharedPointer::*bool_type)() const;
    void uncomparable() const {}
	operator bool_type() const { return m_ptr ? &CSharedPointer::uncomparable : 0; }

	CSharedPointer<T>& operator=(const CSharedPointer<T>& v);
	CSharedPointer<T>& operator=(T *v);
protected:
	int* m_refcount;
	T* m_ptr;
};

// Trivial template class to refcount objects with COW semantics.
template<class T> class CRefcountObject
{
public:
	CRefcountObject();
	CRefcountObject(const CRefcountObject<T>& v);
	CRefcountObject(const T& v);
	virtual ~CRefcountObject();

	void clear();

	T& Get();

	const T& operator*() const;
	const T* operator->() const;

	bool operator==(const CRefcountObject<T>& cmp) const;
	inline bool operator!=(const CRefcountObject<T>& cmp) const { return !(*this == cmp); }
	bool operator<(const CRefcountObject<T>& cmp) const;

	CRefcountObject<T>& operator=(const CRefcountObject<T>& v);
protected:
	int* m_refcount;
	T* m_ptr;
};

template<class T> class CRefcountObject_Uninitialized
{
	/* Almost same as CRefcountObject but does not allocate
	   an object initially.
	   You need to ensure to assign some data prior to calling
	   operator* or ->, otherwise you'll dereference the null-pointer.
	 */
public:
	CRefcountObject_Uninitialized();
	CRefcountObject_Uninitialized(const CRefcountObject_Uninitialized<T>& v);
	CRefcountObject_Uninitialized(const T& v);
	virtual ~CRefcountObject_Uninitialized();

	void clear();

	T& Get();

	const T& operator*() const;
	const T* operator->() const;

	bool operator==(const CRefcountObject_Uninitialized<T>& cmp) const;
	inline bool operator!=(const CRefcountObject_Uninitialized<T>& cmp) const { return !(*this == cmp); }
	bool operator<(const CRefcountObject_Uninitialized<T>& cmp) const;

	CRefcountObject_Uninitialized<T>& operator=(const CRefcountObject_Uninitialized<T>& v);

	bool operator!() const { return m_ptr == 0; }
protected:
	int* m_refcount;
	T* m_ptr;
};

template<class T> CSharedPointer<T>::CSharedPointer()
{
	m_ptr = 0;
	m_refcount = 0;
}

template<class T> CSharedPointer<T>::CSharedPointer(const CSharedPointer<T>& v)
{
	m_ptr = v.m_ptr;
	m_refcount = v.m_refcount;
	if (m_refcount)
		(*m_refcount)++;
}

template<class T> CSharedPointer<T>::CSharedPointer(T* v)
{
	if (v)
	{
		m_ptr = v;
		m_refcount = new int(1);
	}
	else
	{
		m_ptr = 0;
		m_refcount = 0;
	}
}

template<class T> CSharedPointer<T>::~CSharedPointer()
{
	if (m_refcount && (*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}
}

template<class T> void CSharedPointer<T>::clear()
{
	if (m_refcount && (*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}
	m_refcount = 0;
	m_ptr = 0;
}

template<class T> T& CSharedPointer<T>::operator*() const
{
	return *m_ptr;
}

template<class T> T* CSharedPointer<T>::operator->() const
{
	return m_ptr;
}

template<class T> CSharedPointer<T>& CSharedPointer<T>::operator=(const CSharedPointer<T>& v)
{
	if (this == &v)
		return *this;

	if (m_refcount && (*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}
	m_ptr = v.m_ptr;
	m_refcount = v.m_refcount;
	if (m_refcount)
		(*m_refcount)++;

	return *this;
}

template<class T> CSharedPointer<T>& CSharedPointer<T>::operator=(T* v)
{
	if (m_refcount && (*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}

	if (v)
	{
		m_ptr = v;
		m_refcount = new int(1);
	}
	else
	{
		m_ptr = 0;
		m_refcount = 0;
	}

	return *this;
}

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

template<class T> CRefcountObject<T>::CRefcountObject(const T& v)
{
	m_ptr = new T(v);
	m_refcount = new int(1);
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

// The same for the uninitialized version
template<class T> bool CRefcountObject_Uninitialized<T>::operator==(const CRefcountObject_Uninitialized<T>& cmp) const
{
	if (m_ptr == cmp.m_ptr)
		return true;

	return *m_ptr == *cmp.m_ptr;
}

template<class T> CRefcountObject_Uninitialized<T>::CRefcountObject_Uninitialized()
{
	m_refcount = 0;
	m_ptr = 0;
}

template<class T> CRefcountObject_Uninitialized<T>::CRefcountObject_Uninitialized(const CRefcountObject_Uninitialized<T>& v)
{
	m_refcount = v.m_refcount;
	if (m_refcount)
		(*m_refcount)++;
	m_ptr = v.m_ptr;
}

template<class T> CRefcountObject_Uninitialized<T>::CRefcountObject_Uninitialized(const T& v)
{
	m_ptr = new T(v);
	m_refcount = new int(1);
}

template<class T> CRefcountObject_Uninitialized<T>::~CRefcountObject_Uninitialized()
{
	if (!m_refcount)
		return;

	if (*m_refcount == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}
	else
		(*m_refcount)--;
}

template<class T> T& CRefcountObject_Uninitialized<T>::Get()
{
	if (!m_refcount)
	{
		m_refcount= new int(1);
		m_ptr = new T;
	}
	else if (*m_refcount != 1)
	{
		(*m_refcount)--;
		m_refcount = new int(1);
		T* ptr = new T(*m_ptr);
		m_ptr = ptr;
	}

	return *m_ptr;
}

template<class T> CRefcountObject_Uninitialized<T>& CRefcountObject_Uninitialized<T>::operator=(const CRefcountObject_Uninitialized<T>& v)
{
	if (m_ptr == v.m_ptr)
		return *this;
	if (m_refcount && (*m_refcount)-- == 1)
	{
		delete m_refcount;
		delete m_ptr;
	}

	m_refcount = v.m_refcount;
	if (m_refcount)
		(*m_refcount)++;
	m_ptr = v.m_ptr;
	return *this;
}

template<class T> bool CRefcountObject_Uninitialized<T>::operator<(const CRefcountObject_Uninitialized<T>& cmp) const
{
	if (m_ptr == cmp.m_ptr)
		return false;
	if (!m_ptr)
		return true;
	if (!cmp.m_ptr)
		return false;

	return *m_ptr < *cmp.m_ptr;
}

template<class T> void CRefcountObject_Uninitialized<T>::clear()
{
	if (!m_refcount)
		return;
	else if (*m_refcount != 1)
		(*m_refcount)--;
	else
	{
		delete m_ptr;
		delete m_refcount;
	}
	m_refcount = 0;
	m_ptr = 0;
}

template<class T> const T& CRefcountObject_Uninitialized<T>::operator*() const
{
	return *m_ptr;
}

template<class T> const T* CRefcountObject_Uninitialized<T>::operator->() const
{
	return m_ptr;
}

#endif //__REFCOUNT_H__
