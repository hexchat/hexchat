#ifndef SSIZE_T_DEFINED
#ifdef ssize_t
#undef ssize_t
#endif
#ifdef _WIN64
typedef __int64          ssize_t;
#else
typedef _W64 int         ssize_t;
#endif
#define SSIZE_T_DEFINED
#endif
