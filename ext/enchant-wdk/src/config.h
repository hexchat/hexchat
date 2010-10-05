/*
 * Hand tailored config.h for windows.
 */

/* define ssize_t to int if <sys/types.h> doesn't define.*/
typedef int ssize_t;
/* #undef ssize_t */

#if defined(_MSC_VER)
#pragma warning(disable: 4996) /* The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name. */
#endif

#define ENCHANT_VERSION_STRING "1.6.0"
