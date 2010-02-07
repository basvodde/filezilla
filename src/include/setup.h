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
  // IE 6 or higher
  #ifndef _WIN32_IE 
    #define _WIN32_IE 0x0600
  #elif _WIN32_IE <= 0x0600
    #undef _WIN32_IE
    #define _WIN32_IE 0x0600
  #endif

  // Win XP or higher
  #ifndef _WIN32_WINNT 
    #define _WIN32_WINNT 0x0501
  #elif _WIN32_WINNT <= 0x0501
    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
  #endif

  // Win XP or higher
  #ifndef WINVER 
    #define WINVER 0x0501
  #elif WINVER <= 0x0501
    #undef WINVER
    #define WINVER 0x0501
  #endif
#endif
