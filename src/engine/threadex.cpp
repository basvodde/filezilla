#include "FileZilla.h"
#include "threadex.h"

class wxThreadExImpl : public wxThread
{
public:
	wxThreadExImpl(wxThreadEx* pOwner)
	{
		m_pOwner = pOwner;
	}

	virtual ExitCode Entry()
	{
		ExitCode exitCode = m_pOwner->Entry();
		m_pOwner->m_mutex.Lock();
		wxASSERT(!m_pOwner->m_finished);

		m_pOwner->m_exitCode = exitCode;
		m_pOwner->m_finished = true;
		m_pOwner->m_condition.Signal();
		m_pOwner->m_mutex.Unlock();

		return 0;
	}

protected:
	wxThreadEx *m_pOwner;
};

wxThreadEx::wxThreadEx(wxThreadKind kind /*=wxTHREAD_DETACHED*/)
	: m_condition(m_mutex)
{
	m_detached = kind == wxTHREAD_DETACHED;
	m_started = m_finished = false;
	m_pThread = new wxThreadExImpl(this);
}

wxThreadEx::~wxThreadEx()
{
	if (!m_detached)
		Wait();
}

wxThreadError wxThreadEx::Create(unsigned int stackSize /*=0*/)
{
	return m_pThread->Create();
}

wxThreadError wxThreadEx::Run()
{
	wxThreadError error = m_pThread->Run();
	if (error == wxTHREAD_NO_ERROR)
		m_started = true;

	return error;
}

wxThread::ExitCode wxThreadEx::Wait()
{
	wxASSERT(!m_detached);
	if (m_detached || !m_started)
		return (wxThread::ExitCode)-1;

	m_mutex.Lock();
	if (!m_finished)
		m_condition.Wait();

	wxASSERT(m_finished);

	m_mutex.Unlock();

	return m_exitCode;
}
