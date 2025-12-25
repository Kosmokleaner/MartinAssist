
#include <assert.h>
#include <cstring>  // strcmp

inline void check(bool a) 
{
    assert(a);
#ifdef WIN32
    if(!a)
        __debugbreak();
#endif
}


#include <stdint.h>   // uint32_t

#ifndef WIN32
//    #define sprintf_s(buf, ...) snprintf((buf), sizeof(buf), __VA_ARGS__)
    #define sprintf_s(buf,size, ...) snprintf((buf), (size), __VA_ARGS__)
#endif
