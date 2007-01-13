#include "FileZilla.h"
#include "dndobjects.h"

#if FZ3_USESHELLEXT

#include <initguid.h>
#include "../fzshellext/shellext.h"
#include <shlobj.h>

CShellExtensionInterface::CShellExtensionInterface()
{
	m_shellExtension = 0;

	bool result = (CoCreateInstance(CLSID_ShellExtension, NULL,
      CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown,
      &m_shellExtension) == S_OK);

	if (m_shellExtension)
		m_hMutex = CreateMutex(0, false, _T("FileZilla3DragDropExtLogMutex"));
	else
		m_hMutex = 0;

	m_hMapping = 0;
}

CShellExtensionInterface::~CShellExtensionInterface()
{
	if (m_shellExtension)
	{
		((IUnknown*)m_shellExtension)->Release();
		CoFreeUnusedLibraries();
	}

	if (m_hMapping)
		CloseHandle(m_hMapping);

	if (m_hMutex)
		CloseHandle(m_hMutex);
}

bool CShellExtensionInterface::InitDrag(const wxString& source)
{
	if (!m_shellExtension)
		return false;

	if (!m_hMutex)
		return false;

	if (source.Len() > MAX_PATH)
		return false;

	m_hMapping = CreateFileMapping(0, 0, PAGE_READWRITE, 0, DRAG_EXT_MAPPING_LENGTH, DRAG_EXT_MAPPING);
	if (!m_hMapping)
		return false;

	char* data = (char*)MapViewOfFile(m_hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, DRAG_EXT_MAPPING_LENGTH);
	if (!data)
	{
		CloseHandle(m_hMapping);
		m_hMapping = 0;
		return false;
	}

	DWORD result = WaitForSingleObject(m_hMutex, 250);
	if (result != WAIT_OBJECT_0)
	{
		UnmapViewOfFile(data);
		return false;
	}

	*data = DRAG_EXT_VERSION;
	data[1] = 1;
	wcscpy((wchar_t*)(data + 2), source.wc_str());

	ReleaseMutex(m_hMutex);

	UnmapViewOfFile(data);

	return true;
}

wxString CShellExtensionInterface::GetTarget()
{
	if (!m_shellExtension)
		return _T("");

	if (!m_hMutex)
		return _T("");

	if (!m_hMapping)
		return _T("");

	char* data = (char*)MapViewOfFile(m_hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, DRAG_EXT_MAPPING_LENGTH);
	if (!data)
	{
		CloseHandle(m_hMapping);
		m_hMapping = 0;
		return _T("");
	}

	DWORD result = WaitForSingleObject(m_hMutex, 250);
	if (result != WAIT_OBJECT_0)
	{
		UnmapViewOfFile(data);
		return _T("");
	}

	wxString target;
	const char reply = data[1];
	if (reply == 2)
	{
		data[DRAG_EXT_MAPPING_LENGTH - 1] = 0;
#if wxUSE_UNICODE
		target = (wchar_t*)(data + 2);
#else
		target = wxString((wchar_t*)(data + 2), wxConvFile);
#endif
	}
	
	ReleaseMutex(m_hMutex);

	UnmapViewOfFile(data);

	return target;
}

//{7BB79969-2C7E-4107-996C-36DB90890AB2}

#endif //__WXMSW__

CRemoteDataObject::CRemoteDataObject(const CServer& server)
	: wxDataObjectSimple(wxDataFormat(_T("FileZilla3RemoteDataObject"))),
	  m_server(server)
{
	m_didSendData = false;
}

size_t CRemoteDataObject::GetDataSize(const wxDataFormat& format ) const
{
	if (format == m_fileDataObject.GetPreferredFormat())
		return m_fileDataObject.GetDataSize(format);

	if (format != m_remoteFormat)
		return 0;

	wxCHECK(m_xmlFile.GetElement(), 0);

	return const_cast<CRemoteDataObject*>(this)->m_xmlFile.GetRawDataLength();
}

bool CRemoteDataObject::GetDataHere(const wxDataFormat& format, void *buf ) const
{
	if (format == m_fileDataObject.GetPreferredFormat())
		return m_fileDataObject.GetDataHere(format, buf);

	if (format != m_remoteFormat)
		return false;

	wxCHECK(m_xmlFile.GetElement(), false);

	const_cast<CRemoteDataObject*>(this)->m_xmlFile.GetRawDataHere((char*)buf);

	const_cast<CRemoteDataObject*>(this)->m_didSendData = true;
	return true;
}

void CRemoteDataObject::Finalize()
{
	// Convert data into XML
	TiXmlElement* pElement = m_xmlFile.CreateEmpty();
	pElement = pElement->InsertEndChild(TiXmlElement("RemoteDataObject"))->ToElement();

	AddTextElement(pElement, "ProcessId", wxGetProcessId());

	TiXmlElement* pServer = pElement->InsertEndChild(TiXmlElement("Server"))->ToElement();
	SetServer(pServer, m_server);
}

bool CRemoteDataObject::SetData(size_t len, const void* buf)
{
	char* data = (char*)buf;
	if (data[len - 1] != 0)
		return false;

	return m_xmlFile.ParseData(data);
}
