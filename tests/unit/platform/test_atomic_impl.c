#include "zoo.h"
int main()
{
#ifdef ZOO_OS_WINDOWS
    printf("Using Windows atomics\n");
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    printf("Using C11 atomics\n");
#else
    printf("Using fallback atomics\n");
#endif
    return 0;
}
