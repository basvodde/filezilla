#include "FileZilla.h"
#include "netconfwizard.h"
#include "Options.h"
#include "dialogex.h"
#include "filezillaapp.h"
#include "externalipresolver.h"

BEGIN_EVENT_TABLE(CNetConfWizard, wxWizard)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CNetConfWizard::OnPageChanging)
EVT_WIZARD_PAGE_CHANGED(wxID_ANY, CNetConfWizard::OnPageChanged)
EVT_SOCKET(0, CNetConfWizard::OnSocketEvent)
EVT_FZ_EXTERNALIPRESOLVE(wxID_ANY, CNetConfWizard::OnExternalIPAddress)
EVT_BUTTON(XRCID("ID_RESTART"), CNetConfWizard::OnRestart)
END_EVENT_TABLE()

CNetConfWizard::CNetConfWizard(wxWindow* parent, COptions* pOptions)
: m_parent(parent), m_pOptions(pOptions)
{
	m_socket = 0;
	m_pIPResolver = 0;
	ResetTest();
}

CNetConfWizard::~CNetConfWizard()
{
	delete m_socket;
	delete m_pIPResolver;
}

bool CNetConfWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Firewall and router configuration wizard")))
		return false;

	for (int i = 1; i <= 7; i++)
	{
		wxWizardPageSimple* page = new wxWizardPageSimple();
		bool res = wxXmlResource::Get()->LoadPanel(page, this, wxString::Format(_T("NETCONF_PANEL%d"), i));
		if (!res)
			return false;
		page->Show(false);

		m_pages.push_back(page);
	}
	for (unsigned int i = 0; i < (m_pages.size() - 1); i++)
		m_pages[i]->Chain(m_pages[i], m_pages[i + 1]);

	GetPageAreaSizer()->Add(m_pages[0]);

	std::vector<wxWindow*> windows;
	for (unsigned int i = 0; i < m_pages.size(); i++)
		windows.push_back(m_pages[i]);
	wxGetApp().GetWrapEngine()->WrapRecursive(windows, 1.7, "Netconf");

	// Load values

	switch (m_pOptions->GetOptionVal(OPTION_USEPASV))
	{
	default:
	case 0:
		XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->SetValue(true);
		break;
	case 1:
		XRCCTRL(*this, "ID_ACTIVE", wxRadioButton)->SetValue(true);
		break;
	}
	switch (m_pOptions->GetOptionVal(OPTION_PASVREPLYFALLBACKMODE))
	{
	default:
	case 0:
		XRCCTRL(*this, "ID_PASSIVE_FALLBACK1", wxRadioButton)->SetValue(true);
		break;
	case 1:
		XRCCTRL(*this, "ID_PASSIVE_FALLBACK2", wxRadioButton)->SetValue(true);
		break;
	}
	switch (m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE))
	{
	default:
	case 0:
		XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->SetValue(true);
		break;
	case 1:
		XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->SetValue(true);
		break;
	case 2:
		XRCCTRL(*this, "ID_ACTIVEMODE3", wxRadioButton)->SetValue(true);
		break;
	}
	switch (m_pOptions->GetOptionVal(OPTION_LIMITPORTS))
	{
	default:
	case 0:
		XRCCTRL(*this, "ID_ACTIVE_PORTMODE1", wxRadioButton)->SetValue(true);
		break;
	case 1:
		XRCCTRL(*this, "ID_ACTIVE_PORTMODE2", wxRadioButton)->SetValue(true);
		break;
	}
	XRCCTRL(*this, "ID_ACTIVE_PORTMIN", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_LOW)));
	XRCCTRL(*this, "ID_ACTIVE_PORTMAX", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_HIGH)));
	XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl)->SetValue(m_pOptions->GetOption(OPTION_EXTERNALIP));
	XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl)->SetValue(m_pOptions->GetOption(OPTION_EXTERNALIPRESOLVER));
	XRCCTRL(*this, "ID_NOEXTERNALONLOCAL", wxCheckBox)->SetValue(m_pOptions->GetOptionVal(OPTION_NOEXTERNALONLOCAL) != 0);

	return true;
}

bool CNetConfWizard::Run()
{
	RunWizard(m_pages.front());
	return true;
}

void CNetConfWizard::OnPageChanging(wxWizardEvent& event)
{
	if (event.GetPage() == m_pages[3])
	{
		int mode = XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->GetValue() ? 0 : (XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->GetValue() ? 1 : 2);
		if (mode == 1)
		{
			wxTextCtrl* control = XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl);
			wxString ip = control->GetValue();
			if (ip == _T(""))
			{
				wxMessageBox(_("Please enter your external IP address"));
				control->SetFocus();
				event.Veto();
				return;
			}
		}
		else if (mode == 2)
		{
			wxTextCtrl* pResolver = XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl);
			wxString address = pResolver->GetValue();
			if (address == _T(""))
			{
				wxMessageBox(_("Please enter an URL where to get your external address from"));
				pResolver->SetFocus();
				event.Veto();
				return;
			}
		}
	}
	else if (event.GetPage() == m_pages[4])
	{
		int mode = XRCCTRL(*this, "ID_ACTIVE_PORTMODE1", wxRadioButton)->GetValue() ? 0 : 1;
		if (mode)
		{
			wxTextCtrl* pPortMin = XRCCTRL(*this, "ID_ACTIVE_PORTMIN", wxTextCtrl);
			wxTextCtrl* pPortMax = XRCCTRL(*this, "ID_ACTIVE_PORTMAX", wxTextCtrl);
			wxString portMin = pPortMin->GetValue();
			wxString portMax = pPortMax->GetValue();

			long min = 0, max = 0;
			if (!portMin.ToLong(&min) || !portMax.ToLong(&max) ||
				min < 1 || max > 65535 || min > max)
			{
				wxMessageBox(_("Please enter a valid portrange."));
				pPortMin->SetFocus();
				event.Veto();
				return;
			}
		}
	}
	else if (event.GetPage() == m_pages[5] && !event.GetDirection())
	{
		wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
		pNext->SetLabel(m_nextLabelText);
	}
	else if (event.GetPage() == m_pages[5] && event.GetDirection())
	{
		if (m_testDidRun)
			return;

		m_testDidRun = true;

		wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
		pNext->Disable();
		wxButton* pPrev = wxDynamicCast(FindWindow(wxID_BACKWARD), wxButton);
		pPrev->Disable();
		event.Veto();

		PrintMessage(wxString::Format(_("Connecting to %s"), _T("probe.filezilla-project.org")), 0);
		m_socket = new wxSocketClient(wxSOCKET_NOWAIT);
		m_socket->SetEventHandler(*this, 0);
		m_socket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
		m_socket->Notify(true);
		m_recvBufferPos = 0;

		wxIPV4address addr;
		addr.Hostname(_T("probe.filezilla-project.org"));
		addr.Service(14148);

		m_socket->Connect(addr, false);
	}
}

void CNetConfWizard::OnPageChanged(wxWizardEvent& event)
{
	if (event.GetPage() == m_pages[5])
	{
		wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
		m_nextLabelText = pNext->GetLabel();
		pNext->SetLabel(_("&Test"));
	}
	else if (event.GetPage() == m_pages[6])
	{
		wxButton* pPrev = wxDynamicCast(FindWindow(wxID_BACKWARD), wxButton);
		pPrev->Disable();
	}
}

void CNetConfWizard::OnSocketEvent(wxSocketEvent& event)
{
	if (!m_socket)
		return;

	switch (event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		OnReceive();
		break;
	case wxSOCKET_LOST:
		OnClose();
		break;
	case wxSOCKET_CONNECTION:
		OnConnect();
		break;
	}
}

void CNetConfWizard::OnClose()
{
	CloseSocket();
}

void CNetConfWizard::OnConnect()
{
	m_connectSuccessful = true;
}

void CNetConfWizard::OnReceive()
{
	m_socket->Read(m_recvBuffer + m_recvBufferPos, NETCONFBUFFERSIZE - m_recvBufferPos);
	unsigned int len;
	if (m_socket->Error() || !(len = m_socket->LastCount()))
	{
		PrintMessage(_("Connection lost"), 1);
		CloseSocket();
		return;
	}

	m_recvBufferPos += len;

	if (m_recvBufferPos < 3)
		return;

	if (m_recvBuffer[m_recvBufferPos - 1] != '\n' || m_recvBuffer[m_recvBufferPos - 2] != '\r')
	{
		if (m_recvBufferPos == NETCONFBUFFERSIZE)
		{
			PrintMessage(_("Invalid data received"), 1);
			CloseSocket();
		}
		return;
	}
	m_recvBuffer[m_recvBufferPos - 2] = 0;
	
	wxString msg(m_recvBuffer, wxConvLocal);
	wxString str = _("Response:");
	str += _T(" ");
	str += msg;
	PrintMessage(str, 3);

	if (m_recvBufferPos < 3)
	{
		m_testResult = servererror;
		PrintMessage(_("Server sent invalid reply."), 1);
		CloseSocket();
		return;
	}

	switch (m_state)
	{
	case 3:
		if (m_recvBuffer[0] == '2')
			break;

		if (m_recvBuffer[1] == '0' && m_recvBuffer[2] == '1')
		{
			PrintMessage(_("Communication tainted by router"), 1);
			m_testResult = tainted;
			CloseSocket();
			return;
		}
		else if (m_recvBuffer[1] == '1' && m_recvBuffer[2] == '0')
		{
			PrintMessage(_("Wrong external IP address"), 1);
			m_testResult = mismatch;
			CloseSocket();
			return;
		}
		else if (m_recvBuffer[1] == '1' && m_recvBuffer[2] == '1')
		{
			PrintMessage(_("Wrong external IP address"), 1);
			PrintMessage(_("Communication tainted by router"), 1);
			m_testResult = mismatchandtainted;
			CloseSocket();
			return;
		}
		else
		{
			m_testResult = servererror;
			PrintMessage(_("Server sent invalid reply."), 1);
			CloseSocket();
			return;
		}
		break;
	default:
		break;
	}

	m_recvBufferPos = 0;
	m_state++;

	SendNextCommand();
}

void CNetConfWizard::PrintMessage(const wxString& msg, int type)
{
	XRCCTRL(*this, "ID_RESULTS", wxTextCtrl)->AppendText(msg + _T("\n"));
}

void CNetConfWizard::CloseSocket()
{
	if (!m_socket)
		return;

	PrintMessage(_("Connection closed"), 0);

	wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
	pNext->Enable();
	pNext->SetLabel(m_nextLabelText);

	wxString text[5];
	if (!m_connectSuccessful)
	{
		text[0] = _("Connection with the test server failed.");
		text[1] = _("Please check on http://filezilla-project.org/probe.php that the server is running and carefully check your settings again.");
	}
	else
	{
		switch (m_testResult)
		{
		case unknown:
			text[0] = _("Connection with server got closed prematurely.");
			text[1] = _("Please ensure you have a stable internet connection and check on http://filezilla-project.org/probe.php that the server is running.");
			break;
		case successful:
			text[0] = _("Congratulations, your configuration seems to be working.");
			text[1] = _("You should have no problems connecting to other servers, file transfers should work properly.");
			text[2] = _("If you keep having problems with a specific server, the server itself or a remote router might be misconfigured. In this case try to toggle passive mode and contact the server administrator for help.");
			text[3] = _("Please run this wizard again should you change your network environment or in case you suddenly encounter problems with servers that did work previously.");
			text[4] = _("Click on Finish to save your configuration.");
			break;
		case servererror:
			text[0] = _("The server sent an unrecognized reply.");
			text[1] = _("Please make sure you are running the latest version of FileZilla. Visit http://filezilla-project.org for details.");
			text[2] = _("Submit a bug report if this problem persists even with the latest version of FileZilla.");
			break;
		case tainted:
			text[0] = _("Active mode FTP test failed. FileZilla knows the correct external IP address, but your router has misleadingly modified the sent address.");
			text[1] = _("Please make sure your router is using the latest available firmware. Furthermore, your router has to be configured properly. You will have to manual port forwarding. Don't run your router in the so called 'DMZ mode' or 'game mode'.");
			text[2] = _("If this problem stays, please contact your router manufacturer.");
			text[3] = _("Unless this problem gets fixed, active mode FTP will not work and passive mode has to be used.");
			if (XRCCTRL(*this, "ID_ACTIVE", wxRadioButton)->GetValue())
			{
				XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->SetValue(true);
				text[3] += _T(" ");
				text[3] += _("Passive mode has been set as default transfer mode.");
			}
			break;
		case mismatchandtainted:
			text[0] = _("Active mode FTP test failed. FileZilla does not know the correct external IP address. In addition to that, your router has modified the sent address.");
			text[1] = _("Please enter your external IP address on the active mode page of this wizard. In case you have a dynamic address or don't know your external address, use the external resolver option.");
			text[2] = _("Please make sure your router is using the latest available firmware. Furthermore, your router has to be configured properly. You will have to manual port forwarding. Don't run your router in the so called 'DMZ mode' or 'game mode'.");
			text[3] = _("If your router keeps changing the IP address, please contact your router manufacturer.");
			text[4] = _("Unless these problems get fixed, active mode FTP will not work and passive mode has to be used.");
			if (XRCCTRL(*this, "ID_ACTIVE", wxRadioButton)->GetValue())
			{
				XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->SetValue(true);
				text[4] += _T(" ");
				text[4] += _("Passive mode has been set as default transfer mode.");
			}
			break;
		case mismatch:
			text[0] = _("Active mode FTP test failed. FileZilla does not know the correct external IP address.");
			text[1] = _("Please enter your external IP address on the active mode page of this wizard. In case you have a dynamic address or don't know your external address, use the external resolver option.");
			text[2] = _("Unless these problems get fixed, active mode FTP will not work and passive mode has to be used.");
			if (XRCCTRL(*this, "ID_ACTIVE", wxRadioButton)->GetValue())
			{
				XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->SetValue(true);
				text[2] += _T(" ");
				text[2] += _("Passive mode has been set as default transfer mode.");
			}
			break;
		case externalfailed:
			text[0] = _("Failed to retrieve the external IP address.");
			text[1] = _("Pleae make sure FileZilla is allowed to establish outgoing connections and make sure you typed the address of the address resolver correctly.");
			text[2] = wxString::Format(_("The address you entered was: %s"), XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl)->GetValue().c_str());
			break;
		}
	}
	for (unsigned int i = 0; i < 5; i++)
	{
		wxString name = wxString::Format(_T("ID_SUMMARY%d"), i + 1);
		int id = wxXmlResource::GetXRCID(name);
		wxDynamicCast(FindWindowById(id, this), wxStaticText)->SetLabel(text[i]);
	}
	wxGetApp().GetWrapEngine()->WrapRecursive(m_pages[6], m_pages[6]->GetSizer(), wxGetApp().GetWrapEngine()->GetWidthFromCache("Netconf"));

	delete m_socket;
	m_socket = 0;
}

bool CNetConfWizard::Send(wxString cmd)
{
	if (!m_socket)
		return false;

	PrintMessage(cmd, 2);

	cmd += _T("\r\n");
	const char* buffer = cmd.mb_str();
	unsigned int len = strlen(buffer);
	m_socket->Write(buffer, len);
	if (!m_socket->Error() && m_socket->LastCount() == len)
		return true;

	PrintMessage(_("Connection lost"), 1);	
	Close();

	return false;
}

wxString CNetConfWizard::GetExternalIPAddress()
{
	wxASSERT(m_socket);

	int mode = XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->GetValue() ? 0 : (XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->GetValue() ? 1 : 2);
	if (!mode)
	{
		wxIPV4address addr;
		if (!m_socket->GetLocal(addr))
		{
			PrintMessage(_("Failed to retrieve local ip address. Aborting"), 1);
			CloseSocket();
			return _T("");
		}

		return addr.IPAddress();
	}
	else if (mode == 1)
	{
		wxTextCtrl* control = XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl);
		return control->GetValue();
	}
	else if (mode == 2)
	{
		if (!m_pIPResolver)
		{
			wxTextCtrl* pResolver = XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl);
			wxString address = pResolver->GetValue();

			PrintMessage(wxString::Format(_("Retrieving external IP address from %s"), address.c_str()), 0);

			m_pIPResolver = new CExternalIPResolver(this);
			m_pIPResolver->GetExternalIP(address, true);
			if (!m_pIPResolver->Done())
				return _T("");
		}
		if (!m_pIPResolver->Successful())
		{
			delete m_pIPResolver;
			m_pIPResolver = 0;

			PrintMessage(_("Failed to retrieve external ip address, aborting"), 1);

			m_testResult = externalfailed;
			CloseSocket();
			return _T("");
		}

		wxString ip = m_pIPResolver->GetIP();

		delete m_pIPResolver;
		m_pIPResolver = 0;

		return ip;
	}

	return _T("");
}

void CNetConfWizard::OnExternalIPAddress(fzExternalIPResolveEvent& event)
{
	if (!m_pIPResolver)
		return;

	if (m_state != 3)
		return;

	if (!m_pIPResolver->Done())
		return;

	SendNextCommand();
}

void CNetConfWizard::SendNextCommand()
{
	switch (m_state)
	{
	case 1:
		if (!Send(_T("USER ") + wxString(PACKAGE_NAME, wxConvLocal)))
			return;
		break;
	case 2:
		if (!Send(_T("PASS ") + wxString(PACKAGE_VERSION, wxConvLocal)))
			return;
		break;
	case 3:
		{
			PrintMessage(_("Checking for correct external IP address"), 0);
			wxString ip = GetExternalIPAddress();
			if (ip == _T(""))
				return;

			wxString hexIP = ip;
			for (unsigned int i = 0; i < hexIP.Length(); i++)
			{
				wxChar& c = hexIP[i];
				if (c == '.')
					c = '-';
				else
					c = c - '0' + 'a';
			}

			if (!Send(_T("IP ") + ip + _T(" ") + hexIP))
				return;

		}
		break;
	case 4:
		m_testResult = successful;
		Send(_T("QUIT"));
		break;
	case 5:
		CloseSocket();
		break;
	}
}

void CNetConfWizard::OnRestart(wxCommandEvent& event)
{
	ResetTest();
	ShowPage(m_pages[0], false);
}

void CNetConfWizard::ResetTest()
{
	m_state = 0;
	m_connectSuccessful = false;

	m_testDidRun = false;
	m_testResult = unknown;
	m_recvBufferPos = 0;

	if (!m_pages.empty())
		XRCCTRL(*this, "ID_RESULTS", wxTextCtrl)->SetLabel(_T(""));
}
