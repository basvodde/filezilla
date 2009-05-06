#ifndef __VERIFYHOSTKEYDIALOG_H__
#define __VERIFYHOSTKEYDIALOG_H__

/* Full handling is done inside fzsftp, this class is just to display the
 * dialog and for temporary session trust, lost on restart of FileZilla.
 */
class CVerifyHostkeyDialog
{
public:
	static bool IsTrusted(CHostKeyNotification* pNotification);
	static void ShowVerificationDialog(wxWindow* parent, CHostKeyNotification* pNotification);

protected:
	struct t_keyData
	{
		wxString host;
		wxString fingerprint;
	};
	static std::list<t_keyData> m_sessionTrustedKeys;
};

#endif //__VERIFYHOSTKEYDIALOG_H__
