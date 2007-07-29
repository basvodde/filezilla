#ifndef __RECURSIVE_OPERATION_H__
#define __RECURSIVE_OPERATION_H__

#include "state.h"

class CChmodDialog;
class CQueueView;

class CRecursiveOperation : public CStateEventHandler
{
public:
	CRecursiveOperation(CState* pState);
	~CRecursiveOperation();

	enum OperationMode
	{
		recursive_none,
		recursive_download,
		recursive_addtoqueue,
		recursive_delete,
		recursive_chmod
	};

	void StartRecursiveOperation(enum OperationMode mode, const CServerPath& startDir);
	void StopRecursiveOperation();

	void AddDirectoryToVisit(const CServerPath& path, const wxString& subdir, const wxString& localDir = _T(""));

	enum OperationMode GetOperationMode() const { return m_operationMode; }

	// Needed for recursive_chmod
	void SetChmodDialog(CChmodDialog* pChmodDialog);

	void ListingFailed();

	void SetQueue(CQueueView* pQueue);
	
protected:
	// Processes the directory listing in case of a recursive operation
	void ProcessDirectoryListing(const CDirectoryListing* pDirectoryListing);
	bool NextOperation();

	virtual void OnStateChange(unsigned int event, const wxString& data);

	enum OperationMode m_operationMode;

	CState* m_pState;

	struct t_newDir
	{
		CServerPath parent;
		wxString subdir;
		wxString localDir;
		bool doVisit;
	};

	CServerPath m_startDir;
	std::list<CServerPath> m_visitedDirs;
	std::list<t_newDir> m_dirsToVisit;

	// Needed for recursive_chmod
	CChmodDialog* m_pChmodDlg;

	CQueueView* m_pQueue;
};

#endif //__RECURSIVE_OPERATION_H__
