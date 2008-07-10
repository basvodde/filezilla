#ifndef __NETCONFWIZARD_H__
#define __NETCONFWIZARD_H__

#include <wx/wizard.h>
#include "wrapengine.h"
#include "externalipresolver.h"
#include <wx/socket.h>

#define NETCONFBUFFERSIZE 100

class COptions;

class CNetConfWizard : public wxWizard, protected CWrapEngine
{
public:
	CNetConfWizard(wxWindow* parent, COptions* pOptions);
	virtual ~CNetConfWizard();

	bool Load();
	bool Run();

protected:

	void PrintMessage(const wxString& msg, int type);

	void ResetTest();

	wxWindow* m_parent;
	COptions* m_pOptions;

	std::vector<wxWizardPageSimple*> m_pages;

	DECLARE_EVENT_TABLE();
	void OnPageChanging(wxWizardEvent& event);
	void OnPageChanged(wxWizardEvent& event);
	void OnSocketEvent(wxSocketEvent& event);
	void OnExternalIPAddress(fzExternalIPResolveEvent& event);
	void OnRestart(wxCommandEvent& event);
	void OnFinish(wxWizardEvent& event);
	void OnTimer(wxTimerEvent& event);
	
	void OnReceive();
	void ParseResponse(const char* line);
	void OnClose();
	void OnConnect();
	void OnSend();
	void CloseSocket();
	bool Send(wxString cmd);
	void OnDataReceive();
	void OnDataClose();

	void OnAccept();

	void SendNextCommand();

	wxString GetExternalIPAddress();
	
	int CreateListenSocket();
	int CreateListenSocket(unsigned int port);

	wxString m_nextLabelText;

	// Test data
	wxSocketClient* m_socket;
	int m_state;

	char m_recvBuffer[NETCONFBUFFERSIZE];
	int m_recvBufferPos;
	bool m_testDidRun;
	bool m_connectSuccessful;

	enum testResults
	{
		unknown,
		successful,
		mismatch,
		tainted,
		mismatchandtainted,
		servererror,
		externalfailed,
		datatainted
	} m_testResult;

	CExternalIPResolver* m_pIPResolver;
	wxString m_externalIP;

	wxSocketServer* m_pSocketServer;
	wxSocketBase* m_pDataSocket;
	int m_listenPort;
	bool gotListReply;
	int m_data;

	char* m_pSendBuffer;

	wxTimer m_timer;
};

#endif //__NETCONFWIZARD_H__
