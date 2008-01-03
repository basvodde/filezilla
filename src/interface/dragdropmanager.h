#ifndef __DRAGDROPMANAGER__
#define __DRAGDROPMANAGER__

// wxWidgets doesn't provide any means to check on the type of objects
// while an object hasn't been dropped yet and is still being moved around
// At least on Windows, that appears to be a limitation of the native drag
// and drop system.

// As such, keep track on the objects.

class CDragDropManager
{
public:
	static const CDragDropManager* Get() { return m_pDragDropManager; }

	static CDragDropManager* Init();
	void Release();

	const wxWindow* pDragSource;

	wxString localParent;
	std::list<wxString> m_localFiles;

	CServer server;
	CServerPath remoteParent;

protected:
	CDragDropManager();
	virtual ~CDragDropManager() {}

	static CDragDropManager* m_pDragDropManager;
};

#endif //__DRAGDROPMANAGER__
