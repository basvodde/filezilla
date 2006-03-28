#ifndef __THREADEX_H__
#define __THREADEX_H__

// wxThread has one flaw: On joinable threads, wxThread::Wait does not really
// wait. Instead it continues to dispatch messages from the message queue
// This confuses the program logic.
// wxThreadEx is a simple wrapper class that emulates joinable threads using
// detached threads.
// Wait is emulated by waiting for the variable m_finished to be set to true
// which is done as last operation by the detached thread.

// Implementation is fully transparent: All the user has to do is to replace
// wxThread with wxThreadEx.

// TODO: Implement missing functions as provided by wxThread should they ever 
//       be needed

#include <wx/thread.h>

class wxThreadExImpl;
class wxThreadEx
{
	friend class wxThreadExImpl;
public:
	typedef wxThread::ExitCode ExitCode;

	wxThreadEx(wxThreadKind kind = wxTHREAD_DETACHED);
	virtual ~wxThreadEx();

	wxThreadError Create(unsigned int stackSize = 0);
	wxThreadError Run();
	wxThread::ExitCode Wait();

	virtual wxThread::ExitCode Entry() = 0;

private:
	wxMutex m_mutex;
	wxCondition m_condition;

	bool m_detached;
	bool m_started;
	bool m_finished;

	wxThreadExImpl *m_pThread;
	wxThread::ExitCode m_exitCode;
};

#endif //__THREADEX_H__
