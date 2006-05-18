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
