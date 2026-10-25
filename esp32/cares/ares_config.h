/* SPDX-License-Identifier: MIT */
/* c-ares configuration for ESP-IDF (newlib) with lwIP, written for
   esp32/ThirdParty.cmake instead of the one c-ares's configure
   checks generate: these cannot run with the ESP-IDF toolchain */

#define HAVE_AF_INET6 1
#define HAVE_PF_INET6 1
#define HAVE_ARPA_INET_H 1
#define HAVE_ASSERT_H 1
#define HAVE_CLOCK_GETTIME_MONOTONIC 1
#define HAVE_CONNECT 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL 1
#define HAVE_FCNTL_H 1
#define HAVE_FCNTL_O_NONBLOCK 1
#define HAVE_FREEADDRINFO 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETENV 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LONGLONG 1
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_POLL 1
#define HAVE_POLL_H 1
#define HAVE_RECV 1
#define HAVE_RECVFROM 1
#define HAVE_SEND 1
#define HAVE_SENDTO 1
#define HAVE_SETSOCKOPT 1
#define HAVE_SOCKET 1
#define HAVE_STAT 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRNLEN 1
#define HAVE_STRUCT_ADDRINFO 1
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WRITEV 1

#define GETHOSTNAME_TYPE_ARG2 size_t

#define RECVFROM_TYPE_RETV ssize_t
#define RECVFROM_TYPE_ARG1 int
#define RECVFROM_TYPE_ARG2 void *
#define RECVFROM_TYPE_ARG3 size_t
#define RECVFROM_TYPE_ARG4 int
#define RECVFROM_TYPE_ARG5 struct sockaddr *
#define RECVFROM_QUAL_ARG5
#define RECVFROM_TYPE_ARG6 socklen_t *

#define RECV_TYPE_RETV ssize_t
#define RECV_TYPE_ARG1 int
#define RECV_TYPE_ARG2 void *
#define RECV_TYPE_ARG3 size_t
#define RECV_TYPE_ARG4 int

#define SEND_TYPE_RETV ssize_t
#define SEND_TYPE_ARG1 int
#define SEND_TYPE_ARG2 const void *
#define SEND_TYPE_ARG3 size_t
#define SEND_TYPE_ARG4 int

/* no /dev/urandom: c-ares falls back to its own generator */
#define CARES_RANDOM_FILE "/dev/urandom"

/* lwIP has no services database: service names give no port, and
   ports are shown as numbers */
#include <stddef.h>

struct servent {
  char *s_name;
  char **s_aliases;
  int s_port;
  char *s_proto;
};

static inline struct servent *
getservbyname(const char *name, const char *proto)
{
  (void)name;
  (void)proto;
  return NULL;
}

static inline struct servent *
getservbyport(int port, const char *proto)
{
  (void)port;
  (void)proto;
  return NULL;
}
