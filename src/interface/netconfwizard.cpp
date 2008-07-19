#include "FileZilla.h"
#include "netconfwizard.h"
#include "Options.h"
#include "dialogex.h"
#include "filezillaapp.h"
#include "externalipresolver.h"

BEGIN_EVENT_TABLE(CNetConfWizard, wxWizard)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CNetConfWizard::OnPageChanging)
EVT_WIZARD_PAGE_CHANGED(wxID_ANY, CNetConfWizard::OnPageChanged)
EVT_SOCKET(wxID_ANY, CNetConfWizard::OnSocketEvent)
EVT_FZ_EXTERNALIPRESOLVE(wxID_ANY, CNetConfWizard::OnExternalIPAddress)
EVT_BUTTON(XRCID("ID_RESTART"), CNetConfWizard::OnRestart)
EVT_WIZARD_FINISHED(wxID_ANY, CNetConfWizard::OnFinish)
EVT_TIMER(wxID_ANY, CNetConfWizard::OnTimer)
END_EVENT_TABLE()

CNetConfWizard::CNetConfWizard(wxWindow* parent, COptions* pOptions)
: m_parent(parent), m_pOptions(pOptions), m_pSocketServer(0)
{
	m_socket = 0;
	m_pIPResolver = 0;
	m_pSendBuffer = 0;
	m_pSocketServer = 0;
	m_pDataSocket = 0;

	m_timer.SetOwner(this);

	ResetTest();
}

CNetConfWizard::~CNetConfWizard()
{
	delete m_socket;
	delete m_pIPResolver;
	delete [] m_pSendBuffer;
	delete m_pSocketServer;
}

bool CNetConfWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Firewall and router configuration wizard"), wxNullBitmap, wxPoint(0, 0)))
		return false;

	wxSize minPageSize = GetPageAreaSizer()->GetMinSize();

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
	wxGetApp().GetWrapEngine()->WrapRecursive(windows, 1.7, "Netconf", wxSize(), minPageSize);

	CenterOnParent();

	// Load values

	switch (m_pOptions->GetOptionVal(OPTION_USEPASV))
	{
	default:
	case 1:
		XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->SetValue(true);
		break;
	case 0:
		XRCCTRL(*this, "ID_ACTIVE", wxRadioButton)->SetValue(true);
		break;
	}

	XRCCTRL(*this, "ID_FALLBACK", wxCheckBox)->SetValue(m_pOptions->GetOptionVal(OPTION_ALLOW_TRANSFERMODEFALLBACK) != 0);

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
	return RunWizard(m_pages.front());
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
			if (!IsIpAddress(ip))
			{
				wxMessageBox(_("You have to enter a valid IP address."));
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
				min < 1024 || max > 65535 || min > max)
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
		m_socket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
		m_socket->Notify(true);
		m_recvBufferPos = 0;

		wxIPV4address addr;
		addr.Hostname(_T("probe.filezilla-project.org"));
		addr.Service(21);

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
		wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
		pNext->SetFocus();
	}
}

void CNetConfWizard::OnSocketEvent(wxSocketEvent& event)
{
	if (!m_socket)
		return;

	if (event.GetSocket() == m_socket)
	{
		switch (event.GetSocketEvent())
		{
		case wxSOCKET_INPUT:
			OnReceive();
			break;
		case wxSOCKET_OUTPUT:
			OnSend();
			break;
		case wxSOCKET_LOST:
			OnClose();
			break;
		case wxSOCKET_CONNECTION:
			OnConnect();
			break;
		}
	}
	else if (event.GetSocket() == m_pSocketServer)
	{
		switch (event.GetSocketEvent())
		{
		case wxSOCKET_LOST:
			PrintMessage(_("Listen socket closed"), 1);
			CloseSocket();
			break;
		case wxSOCKET_CONNECTION:
			OnAccept();
			break;
		default:
			break;
		}
	}
	else if (event.GetSocket() == m_pDataSocket)
	{
		switch (event.GetSocketEvent())
		{
		case wxSOCKET_LOST:
			OnDataClose();
			break;
		case wxSOCKET_INPUT:
			OnDataReceive();
			break;
		default:
			break;
		}
	}
}

void CNetConfWizard::OnSend()
{
	if (!m_pSendBuffer)
		return;

	if (!m_socket)
		return;

	int len = strlen(m_pSendBuffer);
	m_socket->Write(m_pSendBuffer, len);
	if (m_socket->Error())
	{
		if (m_socket->LastError() != wxSOCKET_WOULDBLOCK)
		{
			PrintMessage(_("Failed to send command."), 1);
			CloseSocket();
		}
		return;
	}
	int written = m_socket->LastCount();
	if (written == len)
	{
		delete m_pSendBuffer;
		m_pSendBuffer = 0;
	}
	else
		memmove(m_pSendBuffer, m_pSendBuffer + written, len - written + 1);
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

	for (int i = 0; i < m_recvBufferPos - 1; i++)
	{
		if (m_recvBuffer[i] == '\n')
		{
			m_testResult = servererror;
			PrintMessage(_("Invalid data received"), 1);
			CloseSocket();
			return;
		}
		if (m_recvBuffer[i] != '\r')
			continue;
		
		if (m_recvBuffer[i + 1] != '\n')
		{
			m_testResult = servererror;
			PrintMessage(_("Invalid data received"), 1);
			CloseSocket();
			return;
		}
		m_recvBuffer[i] = 0;

		if (!*m_recvBuffer)
		{
			m_testResult = servererror;
			PrintMessage(_("Invalid data received"), 1);
			CloseSocket();
			return;
		}

		ParseResponse(m_recvBuffer);

		if (!m_socket)
			return;

		memmove(m_recvBuffer, m_recvBuffer + i + 2, m_recvBufferPos - i - 2);
		m_recvBufferPos -= i + 2;
		i = -1;
	}
}

void CNetConfWizard::ParseResponse(const char* line)
{
	if (m_timer.IsRunning())
		m_timer.Stop();

	const int len = strlen(line);
	wxString msg(line, wxConvLocal);
	wxString str = _("Response:");
	str += _T(" ");
	str += msg;
	PrintMessage(str, 3);

	if (len < 3)
	{
		m_testResult = servererror;
		PrintMessage(_("Server sent invalid reply."), 1);
		CloseSocket();
		return;
	}
	if (line[3] && line[3] != ' ')
	{
		m_testResult = servererror;
		PrintMessage(_("Server sent invalid reply."), 1);
		CloseSocket();
		return;
	}

	if (line[0] == '1')
		return;

	switch (m_state)
	{
	case 3:
		if (line[0] == '2')
			break;

		if (line[1] == '0' && line[2] == '1')
		{
			PrintMessage(_("Communication tainted by router or firewall"), 1);
			m_testResult = tainted;
			CloseSocket();
			return;
		}
		else if (line[1] == '1' && line[2] == '0')
		{
			PrintMessage(_("Wrong external IP address"), 1);
			m_testResult = mismatch;
			CloseSocket();
			return;
		}
		else if (line[1] == '1' && line[2] == '1')
		{
			PrintMessage(_("Wrong external IP address"), 1);
			PrintMessage(_("Communication tainted by router or firewall"), 1);
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
	case 4:
		if (line[0] != '2')
		{
			m_testResult = servererror;
			PrintMessage(_("Server sent invalid reply."), 1);
			CloseSocket();
			return;
		}
		else
		{
			const char* p = line + len;
			while (*(--p) != ' ')
			{
				if (*p < '0' || *p > '9')
				{
					m_testResult = servererror;
					PrintMessage(_("Server sent invalid reply."), 1);
					CloseSocket();
					return;
				}
			}
			m_data = 0;
			while (*++p)
				m_data = m_data * 10 + *p - '0';
		}
		break;
	case 5:
		if (line[0] == '2')
			break;


		if (line[0] == '5' && line[1] == '0' && (line[2] == '1' || line[2] == '2'))
		{
			m_testResult = tainted;
			PrintMessage(_("PORT command tainted by router or firewall."), 1);
			CloseSocket();
			return;
		}

		m_testResult = servererror;
		PrintMessage(_("Server sent invalid reply."), 1);
		CloseSocket();
		return;
	case 6:
		if (line[0] != '2' && line[0] != '3')
		{
			m_testResult = servererror;
			PrintMessage(_("Server sent invalid reply."), 1);
			CloseSocket();
			return;
		}
		if (m_pDataSocket)
		{
			if (gotListReply)
			{
				m_testResult = servererror;
				PrintMessage(_("Server sent invalid reply."), 1);
				CloseSocket();
			}
			gotListReply = true;
			return;
		}
		break;
	default:
		if (line[0] != '2' && line[0] != '3')
		{
			m_testResult = servererror;
			PrintMessage(_("Server sent invalid reply."), 1);
			CloseSocket();
			return;
		}
		break;
	}
	
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
			text[2] = _("If you keep having problems with a specific server, the server itself or a remote router or firewall might be misconfigured. In this case try to toggle passive mode and contact the server administrator for help.");
			text[3] = _("Please run this wizard again should you change your network environment or in case you suddenly encounter problems with servers that did work previously.");
			text[4] = _("Click on Finish to save your configuration.");
			break;
		case servererror:
			text[0] = _("The server sent an unrecognized reply.");
			text[1] = _("Please make sure you are running the latest version of FileZilla. Visit http://filezilla-project.org for details.");
			text[2] = _("Submit a bug report if this problem persists even with the latest version of FileZilla.");
			break;
		case tainted:
			text[0] = _("Active mode FTP test failed. FileZilla knows the correct external IP address, but your router or firewall has misleadingly modified the sent address.");
			text[1] = _("Please update your firewall and make sure your router is using the latest available firmware. Furthermore, your router has to be configured properly. You will have to use manual port forwarding. Don't run your router in the so called 'DMZ mode' or 'game mode'. Things like protocol inspection or protocol specific 'fixups' have to be disabled");
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
			text[2] = _("Please make sure your router is using the latest available firmware. Furthermore, your router has to be configured properly. You will have to use manual port forwarding. Don't run your router in the so called 'DMZ mode' or 'game mode'.");
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
			text[1] = _("Please make sure FileZilla is allowed to establish outgoing connections and make sure you typed the address of the address resolver correctly.");
			text[2] = wxString::Format(_("The address you entered was: %s"), XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl)->GetValue().c_str());
			break;
		case datatainted:
			text[0] = _("Transferred data got tainted.");
			text[1] = _("You likely have a router or firewall which errorneously modified the transferred data.");
			text[2] = _("Please disable settings like 'DMZ mode' or 'Game mode' on your router.");
			text[3] = _("If this problem persists, please contact your router or firewall manufacturer for a solution.");
			break;
		}
	}
	for (unsigned int i = 0; i < 5; i++)
	{
		wxString name = wxString::Format(_T("ID_SUMMARY%d"), i + 1);
		int id = wxXmlResource::GetXRCID(name);
		wxDynamicCast(FindWindowById(id, this), wxStaticText)->SetLabel(text[i]);
	}
	m_pages[6]->GetSizer()->Layout();
	m_pages[6]->GetSizer()->Fit(m_pages[6]);
	wxGetApp().GetWrapEngine()->WrapRecursive(m_pages[6], m_pages[6]->GetSizer(), wxGetApp().GetWrapEngine()->GetWidthFromCache("Netconf"));

	// Focus one so enter key hits finish and not the restart button by default
	XRCCTRL(*this, "ID_SUMMARY1", wxStaticText)->SetFocus();

	delete m_socket;
	m_socket = 0;

	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;

	delete m_pSocketServer;
	m_pSocketServer = 0;

	delete m_pDataSocket;
	m_pDataSocket = 0;

	if (m_timer.IsRunning())
		m_timer.Stop();
}

bool CNetConfWizard::Send(wxString cmd)
{
	wxASSERT(!m_pSendBuffer);

	if (!m_socket)
		return false;

	PrintMessage(cmd, 2);

	cmd += _T("\r\n");
	wxCharBuffer buffer = cmd.mb_str();
	unsigned int len = strlen(buffer);
	m_pSendBuffer = new char[len + 1];
	memcpy(m_pSendBuffer, buffer, len + 1);

	m_timer.Start(15000, true);
	OnSend();

	return m_socket != 0;
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
			m_externalIP = ip;

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
		{
			int port = CreateListenSocket();
			if (!port)
			{
				PrintMessage(_("Failed to create listen socket, aborting"), 1);
				CloseSocket();
				return;
			}
			m_listenPort = port;
			Send(wxString::Format(_T("PREP %d"), port));
			break;
		}
	case 5:
		{
			wxString cmd = wxString::Format(_T("PORT %s,%d,%d"), m_externalIP.c_str(), m_listenPort / 256, m_listenPort % 256);
			cmd.Replace(_T("."), _T(","));
			Send(cmd);
		}
		break;
	case 6:
		Send(_T("LIST"));
		break;
	case 7:
		m_testResult = successful;
		Send(_T("QUIT"));
		break;
	case 8:
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
	if (m_timer.IsRunning())
		m_timer.Stop();

	m_state = 0;
	m_connectSuccessful = false;

	m_testDidRun = false;
	m_testResult = unknown;
	m_recvBufferPos = 0;
	gotListReply = false;

	if (!m_pages.empty())
		XRCCTRL(*this, "ID_RESULTS", wxTextCtrl)->SetLabel(_T(""));
}

void CNetConfWizard::OnFinish(wxWizardEvent& event)
{
	if (m_testResult != successful)
	{
		if (wxMessageBox(_("The test did not succeed. Do you really want to save the settings?"), _("Save settings?"), wxYES_NO | wxICON_QUESTION) != wxYES)
			return;
	}

	m_pOptions->SetOption(OPTION_USEPASV, XRCCTRL(*this, "ID_PASSIVE", wxRadioButton)->GetValue() ? 1 : 0);
	m_pOptions->SetOption(OPTION_ALLOW_TRANSFERMODEFALLBACK, XRCCTRL(*this, "ID_FALLBACK", wxCheckBox)->GetValue() ? 1 : 0);

	m_pOptions->SetOption(OPTION_PASVREPLYFALLBACKMODE, XRCCTRL(*this, "ID_PASSIVE_FALLBACK1", wxRadioButton)->GetValue() ? 0 : 1);

	if (XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->GetValue())
		m_pOptions->SetOption(OPTION_EXTERNALIPMODE, 0);
	else
		m_pOptions->SetOption(OPTION_EXTERNALIPMODE, XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->GetValue() ? 1 : 2);

	m_pOptions->SetOption(OPTION_LIMITPORTS, XRCCTRL(*this, "ID_ACTIVE_PORTMODE1", wxRadioButton)->GetValue() ? 0 : 1);

	long tmp;
	XRCCTRL(*this, "ID_ACTIVE_PORTMIN", wxTextCtrl)->GetValue().ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_LOW, tmp);
	XRCCTRL(*this, "ID_ACTIVE_PORTMAX", wxTextCtrl)->GetValue().ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_HIGH, tmp);
	
	m_pOptions->SetOption(OPTION_EXTERNALIP, XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl)->GetValue());
	m_pOptions->SetOption(OPTION_EXTERNALIPRESOLVER, XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl)->GetValue());
	m_pOptions->SetOption(OPTION_NOEXTERNALONLOCAL, XRCCTRL(*this, "ID_NOEXTERNALONLOCAL", wxCheckBox)->GetValue());
}

int CNetConfWizard::CreateListenSocket()
{
	if (m_pSocketServer)
		return 0;

	if (XRCCTRL(*this, "ID_ACTIVE_PORTMODE1", wxRadioButton)->GetValue())
	{
		return CreateListenSocket(0);
	}
	else
	{
		long low;
		long high;
		XRCCTRL(*this, "ID_ACTIVE_PORTMIN", wxTextCtrl)->GetValue().ToLong(&low);
		XRCCTRL(*this, "ID_ACTIVE_PORTMAX", wxTextCtrl)->GetValue().ToLong(&high);
		
		int mid = GetRandomNumber(low, high);
		wxASSERT(mid >= low && mid <= high);

		for (int port = mid; port <= high; port++)
			if (CreateListenSocket(port))
				return port;
		for (int port = low; port < mid; port++)
			if (CreateListenSocket(port))
				return port;

		return 0;
	}
}

int CNetConfWizard::CreateListenSocket(unsigned int port)
{
	wxIPV4address addr;
	if (!addr.AnyAddress())
		return 0;

	if (!addr.Service(port))
		return 0;

	m_pSocketServer = new wxSocketServer(addr, wxSOCKET_NOWAIT);
	if (!m_pSocketServer->Ok())
	{
		delete m_pSocketServer;
		m_pSocketServer = 0;
		return 0;
	}

	m_pSocketServer->SetEventHandler(*this);
	m_pSocketServer->SetFlags(wxSOCKET_NOWAIT);
	m_pSocketServer->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	m_pSocketServer->Notify(true);


	if (port)
		return port;

	// Get port number from socket
	if (!m_pSocketServer->GetLocal(addr))
	{
		delete m_pSocketServer;
		m_pSocketServer = 0;
		return 0;
	}
	return addr.Service();
}

void CNetConfWizard::OnAccept()
{
	if (m_pDataSocket)
		return;
	m_pDataSocket = m_pSocketServer->Accept(false);
	if (!m_pDataSocket)
		return;

	wxIPV4address peerAddr, dataPeerAddr;
	if (!m_socket->GetPeer(peerAddr))
	{
		delete m_pDataSocket;
		m_pDataSocket = 0;
		PrintMessage(_("Failed to get peer address of control connection, connection closed."), 1);
		CloseSocket();
		return;
	}
	if (!m_pDataSocket->GetPeer(dataPeerAddr))
	{
		delete m_pDataSocket;
		m_pDataSocket = 0;
		PrintMessage(_("Failed to get peer address of data connection, connection closed."), 1);
		CloseSocket();
		return;
	}
	if (peerAddr.IPAddress() != dataPeerAddr.IPAddress())
	{
		delete m_pDataSocket;
		m_pDataSocket = 0;
		PrintMessage(_("Warning, ignoring data connection from wrong IP."), 0);
		return;
	}
	delete m_pSocketServer;
	m_pSocketServer = 0;

	m_pDataSocket->SetEventHandler(*this);
	m_pDataSocket->SetFlags(wxSOCKET_NOWAIT);
	m_pDataSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	m_pDataSocket->Notify(true);
}

void CNetConfWizard::OnDataReceive()
{
	char buffer[100];
	m_pDataSocket->Read(buffer, 99);
	unsigned int len;
	if (m_pDataSocket->Error() || !(len = m_pDataSocket->LastCount()))
	{
		PrintMessage(_("Data socket closed too early."), 1);
		CloseSocket();
		return;
	}
	buffer[len] = 0;

	int data = 0;
	const char* p = buffer;
	while (*p && *p != ' ')
	{
		if (*p < '0' || *p > '9')
		{
			m_testResult = datatainted;
			PrintMessage(_("Received data tainted"), 1);
			CloseSocket();
			return;
		}
		data = data * 10 + *p++ - '0';
	}
	if (data != m_data)
	{
		m_testResult = datatainted;
		PrintMessage(_("Received data tainted"), 1);
		CloseSocket();
		return;
	}
	p++;
	if (p - buffer != (int)len - 4)
	{
		PrintMessage(_("Failed to receive data"), 1);
		CloseSocket();
		return;
	}

	wxUint32 ip = 0;
	for (const wxChar* q = m_externalIP.c_str(); *q; q++)
	{
		if (*q == '.')
			ip *= 256;
		else
			ip = ip - (ip % 256) + (ip % 256) * 10 + *q - '0';
	}
	ip = wxUINT32_SWAP_ON_LE(ip);
	if (memcmp(&ip, p, 4))
	{
		m_testResult = datatainted;
		PrintMessage(_("Received data tainted"), 1);
		CloseSocket();
		return;
	}
	
	delete m_pDataSocket;
	m_pDataSocket = 0;

	if (gotListReply)
	{
		m_state++;
		SendNextCommand();
	}
}

void CNetConfWizard::OnDataClose()
{
	OnDataReceive();
	if (m_pDataSocket)
	{
		PrintMessage(_("Data socket closed too early."), 0);
		CloseSocket();
		return;
	}
	delete m_pDataSocket;
	m_pDataSocket = 0;

	if (gotListReply)
	{
		m_state++;
		SendNextCommand();
	}
}

void CNetConfWizard::OnTimer(wxTimerEvent& event)
{
	if (event.GetId() != m_timer.GetId())
		return;

	PrintMessage(_("Connection timed out."), 0);
	CloseSocket();
}
