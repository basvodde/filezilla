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
