#ifdef _MSC_VER
  #ifdef _DEBUG
    #include <crtdbg.h>
    #define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
  #else
    #define DEBUG_NEW new
  #endif
#else
  #define DEBUG_NEW new
#endif

#ifdef __WXMSW__
  #ifndef _WIN32_IE 
    #define _WIN32_IE 0x0500
  #elif _WIN32_IE <= 0x0500
    #undef _WIN32_IE
    #define _WIN32_IE 0x0500
  #endif

#endif
