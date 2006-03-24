#ifndef __NETCONFWIZARD_H__
#define __NETCONFWIZARD_H__

#include <wx/wizard.h>
#include "wrapengine.h"
#include "externalipresolver.h"

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
	
	void OnReceive();
	void OnClose();
	void OnConnect();
	void OnSend();
	void CloseSocket();
	bool Send(wxString cmd);

	void SendNextCommand();

	wxString GetExternalIPAddress();

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
		externalfailed
	} m_testResult;

	CExternalIPResolver* m_pIPResolver;

	char* m_pSendBuffer;
};

#endif //__NETCONFWIZARD_H__
