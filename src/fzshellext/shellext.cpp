#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

//---------------------------------------------------------------------------
#ifdef _MSC_VER
	#include <objbase.h>
	#define snprintf _snprintf
	
	// The decision what will get exported is done using fzshellext.def 
	#define STDEXPORTAPI STDAPI
#else
	#define STDEXPORTAPI extern "C" __declspec(dllexport) HRESULT STDAPICALLTYPE

	// Currently, the MinGW w32api has no unicode version of ICopyHook. As such,
	// declare ICopyHookW manually.

	// Use some #define magic to prevent LPCOPYHOOK being declared
	#define LPCOPYHOOK LPCOPYHOOKA
		#include <shlobj.h>
	#undef LPCOPYHOOK

	#define INTERFACE ICopyHookW
	DECLARE_INTERFACE_(ICopyHookW, IUnknown)
	{
		STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
		STDMETHOD_(ULONG,AddRef)(THIS) PURE;
		STDMETHOD_(ULONG,Release)(THIS) PURE;
		STDMETHOD_(UINT,CopyCallback)(THIS_ HWND,UINT,UINT,LPCWSTR,DWORD,LPCWSTR,DWORD) PURE;
	};
	#undef INTERFACE
	typedef ICopyHookW *LPCOPYHOOK;

#endif

//---------------------------------------------------------------------------
#include <initguid.h>
#include <shlguid.h>
#include <stdio.h>
#include <shlobj.h>
#include <olectl.h>
#include <time.h>
#include "shellext.h"
#include <tchar.h>

//---------------------------------------------------------------------------
#define DEBUG_MSG(MSG) \
  if (GLogOn) \
  { \
    Debug(MSG); \
  }

#define DEBUG_MSG_W(MSG) \
  if (GLogOn) \
  { \
    DebugW(MSG); \
  }
	
//---------------------------------------------------------------------------
#define DRAG_EXT_REG_KEY _T("Software\\FileZilla 3\\fzshellext")
#define DRAG_EXT_REG_KEY_PARENT _T("Software\\FileZilla 3")
#define DRAG_EXT_NAME _T("FileZilla 3 Shell Extension")
#define THREADING_MODEL _T("Apartment")
#define CLSID_SIZE 39

//---------------------------------------------------------------------------
class CShellExtClassFactory : public IClassFactory
{
public:
  CShellExtClassFactory();
  virtual ~CShellExtClassFactory();

  // IUnknown members
  STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR*);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory members
  STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR*);
  STDMETHODIMP LockServer(BOOL);

protected:
  unsigned long FReferenceCounter;
};
//---------------------------------------------------------------------------
class CShellExt : public IShellExtInit, ICopyHookW
{
public:
  CShellExt();
  virtual ~CShellExt();

  // IUnknown members
  STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR*);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IShellExtInit methods
  STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

  // ICopyHook method
  STDMETHODIMP_(UINT) CopyCallback(HWND Hwnd, UINT Func, UINT Flags,
    LPCTSTR SrcFile, DWORD SrcAttribs, LPCTSTR DestFile, DWORD DestAttribs);
  
protected:
  unsigned long FReferenceCounter;
  LPDATAOBJECT FDataObj;
  HANDLE FMutex;
  unsigned long FLastTicks;
};
//---------------------------------------------------------------------------
unsigned int GRefThisDll = 0;
bool GEnabled = false;
char GLogFile[MAX_PATH] = "";
bool GLogOn = false;
FILE* GLogHandle = NULL;
HANDLE GLogMutex;
HINSTANCE GInstance;
//---------------------------------------------------------------------------
void Debug(const char* Message)
{
  if (GLogOn)
  {
    unsigned long WaitResult = WaitForSingleObject(GLogMutex, 1000);
    if (WaitResult != WAIT_TIMEOUT)
    {
      try
      {
        if (GLogHandle == NULL)
        {
          if (strlen(GLogFile) == 0)
          {
            GLogOn = false;
          }
          else
          {
            GLogHandle = fopen(GLogFile, "at");
            if (GLogHandle == NULL)
            {
              GLogOn = false;
            }
            else
            {
              setbuf(GLogHandle, NULL);
              fprintf(GLogHandle, "----------------------------\n");
            }
          }
        }

        if (GLogOn)
        {
          SYSTEMTIME Time;
          GetSystemTime(&Time);

          fprintf(GLogHandle, "[%2d/%2d/%4d %2d:%02d:%02d.%03d][%04x] %s\n",
            Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute,
            Time.wSecond, Time.wMilliseconds, GetCurrentThreadId(), Message);
        }
      }
      catch(...)
      {
      }
      ReleaseMutex(GLogMutex);
    }
  }
}

void DebugW(const wchar_t* Message)
{
	int bytes = WideCharToMultiByte(CP_UTF8, 0, Message, -1, 0, 0, 0, 0);
	if (bytes <= 0)
	{
		Debug("WideCharToMultiByte failed");
		return;
	}
	char *buffer = new char[bytes + 1];
	int written = WideCharToMultiByte(CP_UTF8, 0, Message, -1, buffer, bytes, 0, 0);
	if (!written)
		Debug("WideCharToMultiByte failed");
	else
	{
        buffer[written] = 0;
		Debug(buffer);
	}
	delete [] buffer;
}

//---------------------------------------------------------------------------
void LogVersion(HINSTANCE HInstance)
{
  if (GLogOn)
  {
    char FileName[MAX_PATH];
    if (GetModuleFileNameA(HInstance, FileName, sizeof(FileName)) > 0)
    {
      Debug(FileName);
      
      unsigned long InfoHandle, Size;
      Size = GetFileVersionInfoSizeA(FileName, &InfoHandle);
      if (Size > 0)
      {
        void* Info;
        Info = new char[Size];
        if (GetFileVersionInfoA(FileName, InfoHandle, Size, Info) != 0)
        {
          VS_FIXEDFILEINFO* VersionInfo;
          unsigned int VersionInfoSize;
          if (VerQueryValueA(Info, "\\", reinterpret_cast<void**>(&VersionInfo),
                &VersionInfoSize) != 0)
          {
            char VersionStr[100];
            snprintf(VersionStr, sizeof(VersionStr), "LogVersion %d.%d.%d.%d",
              HIWORD(VersionInfo->dwFileVersionMS),
              LOWORD(VersionInfo->dwFileVersionMS),
              HIWORD(VersionInfo->dwFileVersionLS),
              LOWORD(VersionInfo->dwFileVersionLS));
            Debug(VersionStr);
          }
          else
          {
            Debug("LogVersion no fixed version info");
          }
        }
        else
        {
          Debug("LogVersion cannot read version info");
        }
      }
      else
      {
        Debug("LogVersion no version info");
      }
    }
  }
}
//---------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE HInstance, DWORD Reason, LPVOID Reserved)
{
  if (Reason == DLL_PROCESS_ATTACH)
  {
    GInstance = HInstance;
  }

  if (GRefThisDll == 0)
  {
    GLogMutex = CreateMutex(NULL, false, _T("FileZilla3DragDropExtLogMutex"));

    for (int Root = 0; Root <= 1; Root++)
    {
      HKEY Key;
      if (RegOpenKeyEx(Root == 0 ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
            DRAG_EXT_REG_KEY, 0,
            STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
            &Key) == ERROR_SUCCESS)
      {
        unsigned long Type;
        unsigned long Value;
        unsigned long Size;
        char Buf[MAX_PATH];

        Size = sizeof(Value);
        if ((RegQueryValueEx(Key, _T("Enable"), NULL, &Type,
               reinterpret_cast<LPBYTE>(&Value), &Size) == ERROR_SUCCESS) &&
            (Type == REG_DWORD))
        {
          GEnabled = (Value != 0);
        }

        Size = sizeof(Buf);
        if ((RegQueryValueExA(Key, "LogFile", NULL, &Type,
               reinterpret_cast<LPBYTE>(&Buf), &Size) == ERROR_SUCCESS) &&
            (Type == REG_SZ))
        {
          strncpy(GLogFile, Buf, sizeof(GLogFile));
          GLogFile[sizeof(GLogFile) - 1] = '\0';
          GLogOn = true;
        }

        RegCloseKey(Key);
      }
    }
    DEBUG_MSG("DllMain loaded settings");
    DEBUG_MSG(GEnabled ? "DllMain enabled" : "DllMain disabled");
    LogVersion(HInstance);
  }
  else
  {
    DEBUG_MSG("DllMain settings already loaded");
  }

  DEBUG_MSG("DllMain leave");

  return 1;   // ok
}
//---------------------------------------------------------------------------
STDEXPORTAPI DllCanUnloadNow(void)
{
  bool CanUnload = (GRefThisDll == 0);
  DEBUG_MSG(CanUnload ? "DllCanUnloadNow can" : "DllCanUnloadNow cannot");
  return (CanUnload ? S_OK : S_FALSE);
}
//---------------------------------------------------------------------------
STDEXPORTAPI DllGetClassObject(REFCLSID Rclsid, REFIID Riid, LPVOID* PpvOut)
{
  DEBUG_MSG("DllGetClassObject");

  *PpvOut = NULL;

  if (IsEqualIID(Rclsid, CLSID_ShellExtension))
  {
    DEBUG_MSG("DllGetClassObject is ShellExtension");

    CShellExtClassFactory* Pcf = new CShellExtClassFactory;

    return Pcf->QueryInterface(Riid, PpvOut);
  }

  return CLASS_E_CLASSNOTAVAILABLE;
}
//---------------------------------------------------------------------------
bool RegisterServer(bool AllUsers)
{
  DEBUG_MSG("RegisterServer enter");

  DEBUG_MSG(AllUsers ? "RegisterServer all users" : "RegisterServer current users");

  bool Result = false;
  HKEY RootKey = AllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  HKEY HKey;
  DWORD Unused;
  wchar_t ClassID[CLSID_SIZE];

  StringFromGUID2(CLSID_ShellExtension, ClassID, CLSID_SIZE);

  if ((RegOpenKeyEx(RootKey, _T("Software\\Classes"), 0, KEY_WRITE, &HKey) ==
         ERROR_SUCCESS) &&
      (RegCreateKeyEx(HKey, _T("CLSID"), 0, NULL,
         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &HKey, &Unused) ==
           ERROR_SUCCESS))
  {
    if (RegCreateKey(HKey, ClassID, &HKey) == ERROR_SUCCESS)
    {
      RegSetValueEx(HKey, NULL, 0, REG_SZ,
        reinterpret_cast<const unsigned char*>(DRAG_EXT_NAME), sizeof(DRAG_EXT_NAME));

      if (RegCreateKey(HKey, _T("InProcServer32"), &HKey) == ERROR_SUCCESS)
      {
        wchar_t Filename[MAX_PATH];
        GetModuleFileName(GInstance, Filename, MAX_PATH);
        RegSetValueEx(HKey, NULL, 0, REG_SZ,
          reinterpret_cast<LPBYTE>(Filename), (_tcslen(Filename) + 1) * sizeof(TCHAR));

        RegSetValueEx(HKey, _T("ThreadingModel"), 0, REG_SZ,
          reinterpret_cast<const unsigned char*>(THREADING_MODEL),
          sizeof(THREADING_MODEL));
      }
    }
    RegCloseKey(HKey);

    if ((RegOpenKeyEx(RootKey, _T("Software\\Classes"),
           0, KEY_WRITE, &HKey) == ERROR_SUCCESS) &&
        (RegCreateKeyEx(HKey,
           _T("directory\\shellex\\CopyHookHandlers\\FileZilla3CopyHook"),
           0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &HKey,
           &Unused) == ERROR_SUCCESS))
    {
      RegSetValueEx(HKey, NULL, 0, REG_SZ,
        reinterpret_cast<LPBYTE>(ClassID), (_tcslen(ClassID) + 1) * 2);
      RegCloseKey(HKey);

      if ((RegCreateKeyEx(RootKey, DRAG_EXT_REG_KEY,
             0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &HKey,
             &Unused) == ERROR_SUCCESS))
      {
        unsigned long Value = 1;
        RegSetValueEx(HKey, _T("Enable"), 0, REG_DWORD,
          reinterpret_cast<unsigned char*>(&Value), sizeof(Value));
        
        RegCloseKey(HKey);
        
        Result = true;
      }
      
      SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
    }
  }

  DEBUG_MSG("RegisterServer leave");

  return Result;
}
//---------------------------------------------------------------------------
STDEXPORTAPI DllRegisterServer()
{
  DEBUG_MSG("DllRegisterServer enter");

  HRESULT Result;
  if (RegisterServer(true) || RegisterServer(false))
  {
    Result = S_OK;
  }
  else
  {
    Result = SELFREG_E_CLASS;
  }
  
  DEBUG_MSG("DllRegisterServer leave");

  return Result;
}
//---------------------------------------------------------------------------
static bool RegDeleteEmptyKey(HKEY root, LPCTSTR name)
{
	HKEY key;
	
	// Can't use SHDeleteEmptyKey, it gives a linking error
	if (RegOpenKeyEx(root, name, 0, KEY_READ, &key) != ERROR_SUCCESS)
		return false;

	DWORD subKeys, values;
	int ret = RegQueryInfoKey(key, 0, 0, 0, &subKeys, 0, 0, &values, 0, 0, 0,0);
	
	RegCloseKey(key);
	
	if (ret != ERROR_SUCCESS)
		return false;
	
	if (subKeys || values)
		return false;

	RegDeleteKey(root, name);
	
	return true;
}

bool UnregisterServer(bool AllUsers)
{
	DEBUG_MSG("UnregisterServer enter");

	DEBUG_MSG(AllUsers ? "UnregisterServer all users" : "UnregisterServer current users");

	bool Result = false;
	wchar_t ClassID[CLSID_SIZE];

	StringFromGUID2(CLSID_ShellExtension, ClassID, CLSID_SIZE);

	HKEY RootKey = AllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	HKEY HKey;

	if ((RegOpenKeyEx(RootKey, _T("Software\\Classes"), 0, KEY_WRITE, &HKey) ==
		ERROR_SUCCESS) &&
		(RegOpenKeyEx(HKey, _T("directory\\shellex\\CopyHookHandlers"),
		0, KEY_WRITE, &HKey) == ERROR_SUCCESS))
	{
		RegDeleteKey(HKey, _T("FileZilla3CopyHook"));

		RegCloseKey(HKey);
	}

	if ((RegOpenKeyEx(RootKey, _T("Software\\Classes"), 0, KEY_WRITE, &HKey) ==
		ERROR_SUCCESS) &&
		(RegOpenKeyEx(HKey, _T("CLSID"), 0, KEY_WRITE, &HKey) ==
		ERROR_SUCCESS))
	{
		if (RegOpenKeyEx(HKey, ClassID, 0, KEY_WRITE, &HKey) == ERROR_SUCCESS)
		{
			RegDeleteKey(HKey, _T("InProcServer32"));

			RegCloseKey(HKey);

			if ((RegOpenKeyEx(RootKey, _T("Software\\Classes"), 0, KEY_WRITE, &HKey) ==
				ERROR_SUCCESS) &&
				(RegOpenKeyEx(HKey, _T("CLSID"), 0, KEY_WRITE, &HKey) ==
				ERROR_SUCCESS))
			{
				RegDeleteKey(HKey, ClassID);

				RegCloseKey(HKey);

				Result = true;
			}
		}
	}

	if ((RegOpenKeyEx(RootKey, DRAG_EXT_REG_KEY, 0, KEY_WRITE, &HKey) ==
		ERROR_SUCCESS))
	{
		RegDeleteValue(HKey, _T("Enable"));
		RegCloseKey(HKey);

		Result = true;
	}
	RegDeleteEmptyKey(RootKey, DRAG_EXT_REG_KEY);
	RegDeleteEmptyKey(RootKey, DRAG_EXT_REG_KEY_PARENT);

	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

	DEBUG_MSG("UnregisterServer leave");

	return Result;
}
//---------------------------------------------------------------------------
STDEXPORTAPI DllUnregisterServer()
{
  DEBUG_MSG("DllUnregisterServer enter");

  HRESULT Result = SELFREG_E_CLASS;
  if (UnregisterServer(true))
  {
    Result = S_OK;
  }

  if (UnregisterServer(false))
  {
    Result = S_OK;
  }

  DEBUG_MSG("DllUnregisterServer leave");

  return Result;
}
//---------------------------------------------------------------------------
CShellExtClassFactory::CShellExtClassFactory()
{
  DEBUG_MSG("CShellExtClassFactory");

  FReferenceCounter = 0;

  GRefThisDll++;
}
//---------------------------------------------------------------------------
CShellExtClassFactory::~CShellExtClassFactory()
{
  DEBUG_MSG("~CShellExtClassFactory");

  GRefThisDll--;
}
//---------------------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID Riid, LPVOID FAR* Ppv)
{
  DEBUG_MSG("QueryInterface");

  *Ppv = NULL;

  // Any interface on this object is the object pointer

  if (IsEqualIID(Riid, IID_IUnknown) || IsEqualIID(Riid, IID_IClassFactory))
  {
    DEBUG_MSG("QueryInterface is IUnknown or IClassFactory");

    *Ppv = (LPCLASSFACTORY)this;

    AddRef();

    return NOERROR;
  }

  return E_NOINTERFACE;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
  DEBUG_MSG("AddRef");
  return ++FReferenceCounter;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
  DEBUG_MSG("Release");

  if (--FReferenceCounter)
  {
    return FReferenceCounter;
  }

  delete this;

  return 0;
}
//---------------------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN UnkOuter,
  REFIID Riid, LPVOID* PpvObj)
{
  DEBUG_MSG("CreateInstance");

  *PpvObj = NULL;

  // Shell extensions typically don't support aggregation (inheritance)

  if (UnkOuter)
  {
    return CLASS_E_NOAGGREGATION;
  }

  // Create the main shell extension object.  The shell will then call
  // QueryInterface with IID_IShellExtInit--this is how shell extensions are
  // initialized.

  CShellExt* ShellExt = new CShellExt();  //Create the CShellExt object

  if (NULL == ShellExt)
  {
    return E_OUTOFMEMORY;
  }

  return ShellExt->QueryInterface(Riid, PpvObj);
}
//---------------------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::LockServer(BOOL Lock)
{
  DEBUG_MSG("LockServer");

  return NOERROR;
}
//---------------------------------------------------------------------------
// CShellExt
CShellExt::CShellExt()
{
  DEBUG_MSG("CShellExt enter");

  FReferenceCounter = 0L;
  FDataObj = NULL;

  FMutex = CreateMutex(NULL, false, DRAG_EXT_MUTEX);
  FLastTicks = 0;

  GRefThisDll++;

  DEBUG_MSG("CShellExt leave");
}
//---------------------------------------------------------------------------
CShellExt::~CShellExt()
{
  DEBUG_MSG("~CShellExt enter");

  if (FDataObj)
  {
    FDataObj->Release();
  }

  CloseHandle(FMutex);

  GRefThisDll--;

  DEBUG_MSG("~CShellExt leave");
}
//---------------------------------------------------------------------------
STDMETHODIMP CShellExt::QueryInterface(REFIID Riid, LPVOID FAR* Ppv)
{
  DEBUG_MSG("CShellExt::QueryInterface enter");

  HRESULT Result = E_NOINTERFACE;
  *Ppv = NULL;

  if (!GEnabled)
  {
    DEBUG_MSG("CShellExt::QueryInterface shelext disabled");
  }
  else
  {
    if (IsEqualIID(Riid, IID_IShellExtInit) || IsEqualIID(Riid, IID_IUnknown))
    {
      DEBUG_MSG("CShellExt::QueryInterface is IShellExtInit or IUnknown");
      *Ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(Riid, IID_IShellCopyHook))
    {
      DEBUG_MSG("CShellExt::QueryInterface is IShellCopyHook");
      *Ppv = (LPCOPYHOOK)this;
    }

    if (*Ppv)
    {
      AddRef();

      Result = NOERROR;
    }
  }

  DEBUG_MSG("CShellExt::QueryInterface leave");

  return Result;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
  DEBUG_MSG("CShellExt::AddRef");

  return ++FReferenceCounter;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExt::Release()
{
  DEBUG_MSG("CShellExt::Release");
  if (--FReferenceCounter)
  {
    return FReferenceCounter;
  }

  delete this;

  return 0;
}
//---------------------------------------------------------------------------
STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST IDFolder,
  LPDATAOBJECT DataObj, HKEY RegKey)
{
  DEBUG_MSG("CShellExt::Initialize enter");

  if (FDataObj != NULL)
  {
    FDataObj->Release();
    FDataObj = NULL;
  }

  // duplicate the object pointer and registry handle

  if (DataObj != NULL)
  {
    FDataObj = DataObj;
    DataObj->AddRef();
  }

  DEBUG_MSG("CShellExt::Initialize leave");

  return NOERROR;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(UINT) CShellExt::CopyCallback(HWND Hwnd, UINT Func, UINT Flags,
											LPCTSTR SrcFile, DWORD SrcAttribs, LPCTSTR DestFile, DWORD DestAttribs)
{
	DEBUG_MSG("CShellExt::CopyCallback enter");

	UINT Result = IDYES;

	if (GEnabled && ((Func == FO_COPY) || (Func == FO_MOVE)))
	{
		DEBUG_MSG("CShellExt::CopyCallback copy or move");

		unsigned long Ticks = GetTickCount();
		if (((Ticks - FLastTicks) >= 100) ||
			(FLastTicks > Ticks))
		{
			//USES_CONVERSION;
			DEBUG_MSG("CShellExt::CopyCallback interval elapsed");

			DEBUG_MSG("CShellExt::CopyCallback source / dest:");
			DEBUG_MSG_W(SrcFile);
			DEBUG_MSG_W(DestFile);

			FLastTicks = Ticks;
			LPCTSTR BackPtr = _tcsrchr(SrcFile, '\\');

			if ((BackPtr != NULL) &&
				(_tcsncmp(BackPtr + 1, DRAG_EXT_DUMMY_DIR_PREFIX,
				DRAG_EXT_DUMMY_DIR_PREFIX_LEN) == 0))
			{
				DEBUG_MSG("CShellExt::CopyCallback filename has prefix");

				HANDLE MapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, DRAG_EXT_MAPPING);

				if (MapFile != NULL)
				{
					DEBUG_MSG("CShellExt::CopyCallback mapfile found");

					char* data = reinterpret_cast<char *>(MapViewOfFile(MapFile, FILE_MAP_ALL_ACCESS, 0, 0, DRAG_EXT_MAPPING_LENGTH));

					if (data != NULL)
					{
						DEBUG_MSG("CShellExt::CopyCallback mapview created");
						unsigned long WaitResult = WaitForSingleObject(FMutex, 1000);
						if (WaitResult != WAIT_TIMEOUT)
						{
							DEBUG_MSG("CShellExt::CopyCallback mutex got");
							if (*data >= DRAG_EXT_VERSION)
							{
								DEBUG_MSG("CShellExt::CopyCallback supported structure version");
								if (data[1] == 1)
								{
									DEBUG_MSG("CShellExt::CopyCallback dragging");

								wchar_t* file = reinterpret_cast<wchar_t *>(data + 2);
								DEBUG_MSG("Dragged file:");
								DEBUG_MSG_W(file);
							
							  if (wcscmp(file, SrcFile) == 0)
							  {
								  data[1] = 2;
								  if (_tcslen(DestFile) > MAX_PATH)
								  {
									  DEBUG_MSG("CShellExt::CopyCallback length of DestFile exceeding MAX_PATH");
								  }
								  else
								  {
									  wcsncpy(file, DestFile, MAX_PATH);
									  file[MAX_PATH] = 0;
									  DEBUG_MSG("CShellExt::CopyCallback destination written into buffer");
								  }
								  Result = IDNO;
								  DEBUG_MSG("CShellExt::CopyCallback dragging refused");
							  }
							  else
							  {
								  data[1] = 3;
								  DEBUG_MSG("CShellExt::CopyCallback dragged file does NOT match");
									}
								}
								else
								{
									DEBUG_MSG("CShellExt::CopyCallback NOT dragging");
								}
							}
							else
							{
								DEBUG_MSG("CShellExt::CopyCallback unsupported structure version");
							}
							ReleaseMutex(FMutex);
							DEBUG_MSG("CShellExt::CopyCallback mutex released");
						}
						else
						{
							DEBUG_MSG("CShellExt::CopyCallback mutex timeout");
						}
						UnmapViewOfFile(data);
					}
					else
					{
						DEBUG_MSG("CShellExt::CopyCallback mapview NOT created");
					}

					CloseHandle(MapFile);
				}
				else
				{
					DEBUG_MSG("CShellExt::CopyCallback mapfile NOT found");
				}
			}
			else
			{
				DEBUG_MSG("CShellExt::CopyCallback filename has NOT prefix");
			}
		}
		else
		{
			DEBUG_MSG("CShellExt::CopyCallback interval NOT elapsed");
		}
	}
	else
	{
		DEBUG_MSG("CShellExt::CopyCallback NOT copy nor move");
	}

	DEBUG_MSG("CShellExt::CopyCallback leave");

	return Result;
}
