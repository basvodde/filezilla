#ifndef __DNDOBJECTS_H__
#define __DNDOBJECTS_H__

#ifdef __WXMSW__
#define FZ3_USESHELLEXT 1
#else
#define FZ3_USESHELLEXT 0
#endif

#include "xmlfunctions.h"

class wxRemoteDataFormat : public wxDataFormat
{
public:
	wxRemoteDataFormat()
		: wxDataFormat(_T("FileZilla3 remote data format v1"))
	{
	}
};

class CRemoteDataObject : public wxDataObjectSimple
{
public:
	CRemoteDataObject(const CServer& server, const CServerPath& path);
	CRemoteDataObject();
	
	virtual size_t GetDataSize() const;
	virtual bool GetDataHere(void *buf ) const;

	virtual bool SetData(size_t len, const void* buf);

	// Finalize has to be called prior to calling wxDropSource::DoDragDrop
	void Finalize();

	bool DidSendData() const { return m_didSendData; }

	const CServer& GetServer() const { return m_server; }
	const CServerPath& GetServerPath() const { return m_path; }
	int GetProcessId() const { return m_processId; }

	struct t_fileInfo
	{
		wxString name;
		bool dir;
		wxLongLong size;
	};

	const std::list<t_fileInfo>& GetFiles() const { return m_fileList; }

	void AddFile(wxString name, bool dir, wxLongLong size);

protected:
	CServer m_server;
	CServerPath m_path;

	CXmlFile m_xmlFile;

	bool m_didSendData;

	int m_processId;

	std::list<t_fileInfo> m_fileList;
};

#if FZ3_USESHELLEXT

// This class checks if the shell extension is installed and
// communicates with it.
class CShellExtensionInterface
{
public:
	CShellExtensionInterface();
	virtual ~CShellExtensionInterface();

	bool IsLoaded() const { return m_shellExtension != 0; }

	wxString InitDrag();
	
	wxString GetTarget();

protected:
	bool CreateDragDirectory();

	void* m_shellExtension;
	HANDLE m_hMutex;
	HANDLE m_hMapping;

	wxString m_dragDirectory;
};

#endif //__WXMSW__

#endif //__DNDOBJECTS_H__
