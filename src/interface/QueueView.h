#ifndef __QUEUEVIEW_H__
#define __QUEUEVIEW_H__

class CQueueItem
{
public:
protected:
	CQueueItem();
	~CQueueItem();
	int numChildren;
};

class CServerItem : public CQueueItem
{
public:
protected:
};

class CFolderItem : public CQueueItem
{
public:
protected:
};

class CFileItem : public CQueueItem
{
public:
	CFileItem(const wxString& localFile, const wxString& remoteFile, const CServerPath& remotePath);
	virtual ~CFileItem();
protected:
};

class CQueueView : public wxListCtrl
{
public:
	CQueueView(wxWindow* parent, wxWindowID id);
	virtual ~CQueueView();
	
	bool QueueFile(bool download, const wxString& localFile, const wxString& remoteFile, const CServerPath& remotePath, const CServer& server);
	bool QueueFolder(bool download, const wxString& localPath, const CServerPath& remotePath, const CServer& server);
	
	bool IsEmpty() const;
	bool IsActive() const;
	bool SetActive(bool activate = true);
};

#endif
