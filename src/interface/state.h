#ifndef __STATE_H__
#define __STATE_H__

class CLocalListView;
class CState
{
public:
	CState();
	~CState();

	wxString GetLocalDir() const;
	bool SetLocalDir(wxString dir);

	void SetLocalListView(CLocalListView *pLocalListView);

protected:
	wxString m_localDir;
	CLocalListView *m_pLocalListView;
};

#endif

