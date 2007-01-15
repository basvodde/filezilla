#ifndef __SHELLEXT_H__
#define __SHELLEXT_H__

// Shell extension to handle dragging of files from FileZilla 3 into Explorer.
// Based on code from WinSCP (http://winscp.net/)
// Has to be compiled in Unicode mode.

#define DRAG_EXT_MAPPING _T("FileZilla3DragDropExtMapping")
#define DRAG_EXT_MUTEX _T("FileZilla3DragDropExtMutex")
#define DRAG_EXT_DUMMY_DIR_PREFIX _T("fz3-")
#define DRAG_EXT_DUMMY_DIR_PREFIX_LEN 4

// {DB70412E-EEC9-479c-BBA9-BE36BFDDA41B}
DEFINE_GUID(CLSID_ShellExtension, 
0xdb70412e, 0xeec9, 0x479c, 0xbb, 0xa9, 0xbe, 0x36, 0xbf, 0xdd, 0xa4, 0x1b);

// Internal structure of the file mapping
// (Note: direct mappings to some struct cannot be used since structs are 
//  usually aligned differently depending on used compiler)
//
// Version number: 1 byte
// Active drag&drop operation: 1 byte
//   Values: 0 - inactive
//           1 - Awaiting reply from shell extension
//           2 - shell extension has filled in the data
//           3 - shell extension failed to fill in the data
// Filename: Given as wide character string. Zero-terminated. Capped to MAX_PATH wide-characters + terminating 0.

const int DRAG_EXT_MAPPING_LENGTH = 1 + 1 + (MAX_PATH + 1) * 2 + 1;

const int DRAG_EXT_VERSION = 1;

#endif //__SHELLEXT_H__
