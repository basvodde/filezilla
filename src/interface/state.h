#ifndef __STATE_H__
#define __STATE_H__

class CLocalListView;
class CRemoteListView;
class CDirectoryListing;
class CState
{
public:
	CState();
	~CState();

	wxString GetLocalDir() const;
	bool SetLocalDir(wxString dir);

	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing);
	const CDirectoryListing *GetRemoteDir() const;

	void SetLocalListView(CLocalListView *pLocalListView);
	void SetRemoteListView(CRemoteListView *pRemoteListView);

protected:
	wxString m_localDir;
	CDirectoryListing *m_pDirectoryListing;

	CLocalListView *m_pLocalListView;
	CRemoteListView *m_pRemoteListView;
};

#endif

