/*
Copyright (c) 2006 Tim Kosse

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
