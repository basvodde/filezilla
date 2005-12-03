#ifndef __IOTHREAD_H__
#define __IOTHREAD_H__

#define BUFFERCOUNT 3
#define BUFFERSIZE 65536

class CIOThreadEvent : public wxEvent
{
public:
	CIOThreadEvent(int id = wxID_ANY);

	virtual wxEvent *Clone() const;
};

typedef void (wxEvtHandler::*CIOThreadEventFunction)(CIOThreadEvent&);

extern const wxEventType fzEVT_IOTHREAD;
#define EVT_IOTHREAD(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_IOTHREAD, id, -1, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( CIOThreadEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

enum IORet
{
	IO_Success = 0,
	IO_Error = -2,
	IO_Again = -1
};

class CIOThread : public wxThread
{
public:
	CIOThread();
	virtual ~CIOThread();

	bool Create(wxFile* pFile, bool read, bool binary);
	virtual void Destroy(); // Only call that might be blocking
	
	// Call before first call to one of the GetNext*Buffer functions
	// This handler will receive the CIOThreadEvent events. The events
	// get triggerd iff a buffer is available after a call to the
	// GetNext*Buffer functions returned IO_Again
	void SetEventHandler(wxEvtHandler* handler){ m_evtHandler = handler; }

	// Gets next buffer
	// Return value:  IO_Success on EOF
	//                IO_Again if it would block
	//                IO_Error on error
	//                buffersize else
	int GetNextReadBuffer(char** pBuffer);

	// Gets next write buffer
	// Return value: IO_Again if it would block
	//               IO_Error on error
	//               IO_Success else
	int GetNextWriteBuffer(char** pBuffer, int len = BUFFERSIZE);
	
	bool Finalize(int len);

protected:
	virtual ExitCode Entry();

	wxEvtHandler* m_evtHandler;

	bool m_read;
	bool m_binary;
	wxFile* m_pFile;

	char* m_buffers[3];
	unsigned int m_bufferLens[3];

	wxMutex m_mutex;
	wxCondition m_condition;

	int m_curAppBuf;
	int m_curThreadBuf;

	bool m_error;
	bool m_running;
	bool m_threadWaiting;
	bool m_appWaiting;

	bool m_destroyed;
};

#endif //__IOTHREAD_H__
