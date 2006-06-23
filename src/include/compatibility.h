#ifndef __COMPATIBILITY_H__
#define __COMPATIBILITY_H__

// This file checks the used wxWidgets version and sets some defines
// so that FileZilla can compile with different versions of wxWidgets.

#ifndef wxCONV_FAILED
#define wxCONV_FAILED ((size_t)-1)
#endif

#endif //__COMPATIBILITY_H__
