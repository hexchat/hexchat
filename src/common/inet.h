/* include stuff for internet */

#ifndef WIN32

#ifdef WANTSOCKET
#include <sys/types.h>
#include <sys/socket.h>
#endif
#ifdef WANTARPA
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef WANTDNS
#include <netdb.h>
#endif
#define closesocket close
#define set_blocking(sok) fcntl(sok, F_SETFL, 0)
#define set_nonblocking(sok) fcntl(sok, F_SETFL, O_NONBLOCK)
#define would_block_again() (errno == EAGAIN || errno == EWOULDBLOCK)
#define would_block() (errno == EWOULDBLOCK)
#define sock_error() (errno)

#else

#include <winsock.h>

#define set_blocking(sok)	{ \
									unsigned long zero = 0; \
									ioctlsocket (sok, FIONBIO, &zero); \
									}
#define set_nonblocking(sok)	{ \
										unsigned long one = 1; \
										ioctlsocket (sok, FIONBIO, &one); \
										}
#define would_block_again() (WSAGetLastError() == WSAEWOULDBLOCK || \
									  WSAGetLastError() == WSATRY_AGAIN)
#define would_block() (WSAGetLastError() == WSAEWOULDBLOCK)
#define sock_error WSAGetLastError

#endif
