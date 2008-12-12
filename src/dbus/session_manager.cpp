/* This class uses the D-BUS API of the GNOME Session Manager.
 * See http://www.gnome.org/~mccann/gnome-session/docs/gnome-session.html for details
 */
#include <wx/wx.h>
#include "session_manager.h"
#include "wxdbusconnection.h"
#include "wxdbusmessage.h"
#include <memory>

enum state
{
	error,
	register_client,
	initialized,
	query_end_session,
	end_session,
	ended_session,
	unregister
};

class CSessionManagerImpl : public wxEvtHandler
{
public:
	CSessionManagerImpl();
	virtual ~CSessionManagerImpl();

	bool ConfirmEndSession();
	bool Unregister();

	bool SendEvent(bool query, bool* veto = 0);

	wxDBusConnection* m_pConnection;

	DECLARE_EVENT_TABLE()
	void OnSignal(wxDBusConnectionEvent& event);
	void OnAsyncReply(wxDBusConnectionEvent& event);

	enum state m_state;

	char* m_client_object_path;

	bool m_debug;
};

static CSessionManagerImpl* the_gnome_session_manager;


BEGIN_EVENT_TABLE(CSessionManagerImpl, wxEvtHandler)
EVT_DBUS_SIGNAL(wxID_ANY, CSessionManagerImpl::OnSignal)
EVT_DBUS_ASYNC_RESPONSE(wxID_ANY, CSessionManagerImpl::OnAsyncReply)
END_EVENT_TABLE()

CSessionManager::CSessionManager()
{
}

CSessionManager::~CSessionManager()
{
}

CSessionManagerImpl::CSessionManagerImpl()
{
#ifdef __WXDEBUG__
	m_debug = true;
#else
	m_debug = false;
	wxString v;
	if (wxGetEnv(_T("FZDEBUG"), &v) && v == _T("1"))
		m_debug = true;
#endif
	m_pConnection = new wxDBusConnection(wxID_ANY, this, false);
	if (!m_pConnection->IsConnected())
	{
		if (m_debug)
			printf("wxD-Bus: Could not connect to session bus\n");
		m_state = error;
	}
	else
	{
		wxDBusMethodCall* call = new wxDBusMethodCall(
			"org.gnome.SessionManager",
			"/org/gnome/SessionManager",
			"org.gnome.SessionManager",
			"RegisterClient");

		m_state = register_client;

		char pid[sizeof(unsigned long) * 8 + 10];
		sprintf(pid, "FileZilla %lu", wxGetProcessId()); 
		call->AddString(pid);
		call->AddString(pid); // What's this "startup identifier"?

		if (!call->CallAsync(m_pConnection, 1000))
			m_state = error;
	}

	m_client_object_path = 0;
}

CSessionManagerImpl::~CSessionManagerImpl()
{
	delete [] m_client_object_path;
	delete m_pConnection;
}

bool CSessionManager::Init()
{
	if (!the_gnome_session_manager)
		the_gnome_session_manager = new CSessionManagerImpl;
	return true;
}

bool CSessionManager::Uninit()
{
	if (the_gnome_session_manager)
	{
		if (!the_gnome_session_manager->ConfirmEndSession())
			the_gnome_session_manager->Unregister();
	}

	delete the_gnome_session_manager;
	the_gnome_session_manager = 0;
	return true;
}

void CSessionManagerImpl::OnSignal(wxDBusConnectionEvent& event)
{
	std::auto_ptr<wxDBusMessage> msg(wxDBusMessage::ExtractFromEvent(&event));
	if (m_state == error || m_state == unregister)
	{
		if (m_debug)
			printf("wxD-Bus: OnSignal during bad state: %d\n", m_state);
		return;
	}

	if (msg->GetType() == DBUS_MESSAGE_TYPE_ERROR)
	{
		if (m_debug)
			printf("wxD-Bus: Signal: Error: %s\n", msg->GetString());
		return;
	}

	const char* path = msg->GetPath();
	if (!path)
	{
		if (m_debug)
			printf("wxD-Bus: Signal contains no path\n");
		return;
	}

	if (!strcmp(path, "org.gnome.SessionManager"))
	{
		if (m_debug)
			printf("wxD-Bus: Signal from %s, member %s\n", msg->GetPath(), msg->GetMember());
		return;
	}
	
	if (m_client_object_path && !strcmp(path, m_client_object_path))
	{
		const char* member = msg->GetMember();
		if (m_debug)
			printf("wxD-Bus: Signal from %s, member %s\n", msg->GetPath(), member);
		if (!strcmp(member, "QueryEndSession"))
		{
			m_state = query_end_session;

			bool veto;

			wxUint32 flags;
			if (msg->GetUInt(flags) && flags & 1)
				veto = true;
			else
				veto = false;

			bool res = SendEvent(true, &veto);

			wxDBusMethodCall* call = new wxDBusMethodCall(
				"org.gnome.SessionManager",
				m_client_object_path,
				0,//"org.gnome.SessionManager.ClientPrivate",
				"EndSessionResponse");
			call->AddBool(!res || !veto);
			call->AddString("No reason given");
			call->CallAsync(m_pConnection, 1000);
		}
		else if (!strcmp(member, "EndSession"))
		{
			m_state = end_session;

			if (!SendEvent(false))
				ConfirmEndSession();
		}
		return;
	}
}

void CSessionManagerImpl::OnAsyncReply(wxDBusConnectionEvent& event)
{
	std::auto_ptr<wxDBusMessage> msg(wxDBusMessage::ExtractFromEvent(&event));

	if (m_state == error)
		return;

	if (msg->GetType() == DBUS_MESSAGE_TYPE_ERROR)
	{
		if (m_debug)
			printf("wxD-Bus: Reply: Error: %s\n", msg->GetString());
		m_state = error;
		return;
	}

	if (m_state == register_client)
	{
		const char* obj_path = msg->GetObjectPath();
		if (!obj_path)
		{
			if (m_debug)
				printf("wxD-Bus: Reply to RegisterClient does not contain our object path\n");
			m_state = error;
		}
		else
		{
			m_client_object_path = new char[strlen(obj_path) + 1];
			strcpy(m_client_object_path, obj_path);
			if (m_debug)
				printf("wxD-Bus: Reply to RegisterClient, our object path is %s\n", msg->GetObjectPath());
			m_state = initialized;
		}
	}
	else if (m_state == query_end_session)
	{
		m_state = initialized;
	}
	else if (m_state == ended_session)
	{
		if (m_debug)
			printf("wxD-Bus: Session is over\n");
		m_state = unregister;

		wxDBusMethodCall* call = new wxDBusMethodCall(
			"org.gnome.SessionManager",
			"/org/gnome/SessionManager",
			"org.gnome.SessionManager",
			"UnregisterClient");
		call->AddObjectPath(m_client_object_path);

		call->CallAsync(m_pConnection, 1000);

	}
	else if (m_state == unregister)
	{
		// We're so done
		m_state = error;
		if (m_debug)
			printf("wxD-Bus: Unregistered\n");
	}
	else
	{
		if (m_debug)
			printf("wxD-Bus: Unexpected reply in state %d\n", m_state);
		m_state = error;
	}
}

bool CSessionManagerImpl::ConfirmEndSession()
{
	if (m_state != end_session)
		return false;

	if (m_debug)
		printf("wxD-Bus: Confirm session end\n");

	m_state = ended_session;
	wxDBusMethodCall* call = new wxDBusMethodCall(
		"org.gnome.SessionManager",
		m_client_object_path,
		"org.gnome.SessionManager.ClientPrivate",
		"EndSessionResponse");
	call->AddBool(true);
	call->AddString("");
	call->CallAsync(m_pConnection, 1000);

	return true;
}

bool CSessionManagerImpl::SendEvent(bool query, bool* veto /*=0*/)
{
	wxApp* pApp = (wxApp*)wxApp::GetInstance();
	if (!pApp)
		return false;

	wxWindow* pTop = pApp->GetTopWindow();
	if (!pTop)
		return false;

	wxCloseEvent evt(query ? wxEVT_QUERY_END_SESSION : wxEVT_END_SESSION);
	evt.SetCanVeto(veto && *veto);

	if (!pTop->ProcessEvent(evt))
		return false;
	
	if (veto)
		*veto = evt.GetVeto();

	return true;
}

bool CSessionManagerImpl::Unregister()
{
	if (m_state == error || m_state == unregister || m_state == register_client)
		return false;

	m_state = unregister;

	wxDBusMethodCall* call = new wxDBusMethodCall(
		"org.gnome.SessionManager",
		"/org/gnome/SessionManager",
		"org.gnome.SessionManager",
		"UnregisterClient");
	call->AddObjectPath(m_client_object_path);

	if (call->Call(m_pConnection, 1000))
	{
		if (m_debug)
			printf("wxD-Bus: Unregistered\n");
	}

	m_state = error;

	return true;
}

