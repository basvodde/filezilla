#ifndef __STATE_H__
#define __STATE_H__

class CLocalListView;
class CLocalTreeView;
class CRemoteListView;
class CDirectoryListing;
class CLocalViewHeader;
class CRemoteViewHeader;
class CFileZillaEngine;
class CCommandQueue;
class CMainFrame;
class CState
{
	friend class CCommandQueue;
public:
	CState(CMainFrame* pMainFrame);
	~CState();

	bool CreateEngine();
	void DestroyEngine();

	wxString GetLocalDir() const;
	bool SetLocalDir(wxString dir);

	bool Connect(const CServer& server, bool askBreak, const CServerPath& path = CServerPath());

	bool SetRemoteDir(const CDirectoryListing *m_pDirectoryListing, bool modified = false);
	const CDirectoryListing *GetRemoteDir() const;
	const CServerPath GetRemotePath() const;
	
	const CServer* GetServer() const;

	void SetLocalListView(CLocalListView *pLocalListView);
	void SetLocalTreeView(CLocalTreeView *m_pLocalTreeView);
	void SetRemoteListView(CRemoteListView *pRemoteListView);

	void RefreshLocal();

	void SetLocalViewHeader(CLocalViewHeader* pLocalViewHeader) { m_pLocalViewHeader = pLocalViewHeader; }
	void SetRemoteViewHeader(CRemoteViewHeader* pRemoteViewHeader) { m_pRemoteViewHeader = pRemoteViewHeader; }

	void ApplyCurrentFilter();

	static CState* GetState();

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;

protected:
	void SetServer(const CServer* server);

	wxString m_localDir;
	CDirectoryListing *m_pDirectoryListing;

	CLocalListView *m_pLocalListView;
	CLocalTreeView *m_pLocalTreeView;
	CRemoteListView *m_pRemoteListView;
	CServer* m_pServer;
	CLocalViewHeader* m_pLocalViewHeader;
	CRemoteViewHeader* m_pRemoteViewHeader;

	CMainFrame* m_pMainFrame;
};

#endif

