/*
 */
#if defined(_WIN32) || defined(_WIN64)
#  include <winsock2.h>
#endif
#include "udp_utils.h"
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#endif

#define TIMEVAL         struct timeval

typedef struct hostent  HOSTENT ;
typedef HOSTENT*        PHOSTENT ;

#ifdef DO_NOT_USE

const char *dump_hex( const char *buf, int16_t n, char *dst ) 
{
  if ((buf == NULL) || (dst == NULL))
    return NULL ;

  char *c = dst ;
  c[0] = '\0';
  for (int i=0; i<n; i++)
  {
    sprintf( c, "%02x", ((uint8_t*)buf)[i] ) ;
    n -= 2;
    c += 2 ;
    if ((i % 8) == 7)
    {
//      sprintf( c, " " ) ;
      strcat(c, " ");
      c++ ;
    }
  }
  *c = '\0' ;
  return dst ;
} // :: dump_hex 

IpAddress get_local_address()
{
  char        name[255] ;
//  PHOSTENT    hostinfo ; 
  name[0] = '\0'; 
  if (gethostname ( name, 255) == 0)
  {
    int status;
    char ip_addr[INET6_ADDRSTRLEN];
    struct sockaddr_in* remote;
    struct addrinfo hints, * res, * p;

//    ZeroMemory(&hints, sizeof(hints));
    memset( &hints, 0x00, sizeof(hints));
    hints.ai_family = AF_INET; // AF_UNSPEC;
    hints.aisocktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((status = getaddrinfo( name, "135", &hints, &res)) != 0) 
    {
      printf("getaddrinfo failed: %s", gai_strerror(status));
      return IpAddress();
    }

    for (p = res; p != NULL; p = p->ai_next) 
    {
      void* addr;
      remote = (struct sockaddr_in*)p->ai_addr;
      addr = &(remote->sin_addr);
      // Convert IP to string
      inet_ntop(p->ai_family, addr, ip_addr, sizeof(ip_addr));
      printf("ip:  %s\n", ip_addr);
    }
    
/*
    if ((hostinfo = gethostbyname(name)) != NULL)
    {
      struct sockaddr_in  addr = {} ;
      addr.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list ;
      return NetAddress( addr ) ;
    }
*/
  }

  return IpAddress() ;
} // get_local_address

#endif // DO_NOT_USE


SOCKET udp_init(uint16_t port)
{
//  char  ip[256];
  SOCKET  sock = INVALID_SOCKET;
  struct sockaddr_in      serv_addr;

  // open UDP socket 
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
  {
    printf("error opening socket.\n");
    return -2;
  }

  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int));

  memset(&serv_addr, 0x00, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
//  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl( INADDR_ANY ) ;
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr );
  serv_addr.sin_port = htons((unsigned int16_t)port);

  if (bind(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
  {
    ::close(sock);
    sock = -1 ;
    printf("error binding to port %d \n", port);

  }

  return sock;
} // :: udp_init

void udp_close( SOCKET &s )
{
  if (s != INVALID_SOCKET)
  {
    close(s);
    s = INVALID_SOCKET;
  }
} // :: udp_close

int16_t udp_recvfrom( SOCKET sock, uint8_t *&recv_buf, uint16_t &buf_sz, struct sockaddr &src_addr, uint32_t timeout ) 
{
  socklen_t   n = 0 ;
  TIMEVAL     timeval ;
  timeval.tv_sec  = (timeout / 1000) ;
  timeval.tv_usec = (timeout % 1000) * 1000 ;

#if defined(_WIN32) || defined(_WIN64)
  fd_set      readfds ;
  fd_set      writefds ;
  fd_set      exceptfds ;
  memset( &readfds  , 0x00, sizeof(fd_set) ) ;
  memset( &writefds , 0x00, sizeof(fd_set) ) ;
  memset( &exceptfds, 0x00, sizeof(fd_set) ) ;
  readfds.fd_count = 1 ;
  readfds.fd_array[0] = sock ;

  n = ::select( 1, &readfds, &writefds, &exceptfds, &timeval ) ;
#else
  fd_set readfds ;
  FD_ZERO( &readfds ) ;
  FD_SET ( sock, &readfds ) ;
  n = ::select( FD_SETSIZE, &readfds, NULL, NULL, &timeval ) ;
#endif
  if (n <= 0) // timeout or error
    return n;

  n = sizeof(src_addr) ;
  if (recv_buf == nullptr) {
    buf_sz = 2 * 1024;
    recv_buf = new uint8_t[buf_sz];
  }
  int amt = recvfrom(sock, (char*)recv_buf, buf_sz, MSG_PEEK, &src_addr, &n);
  if (amt >= buf_sz) {
    // mesage too big, reallocate recv buffer
    delete[] recv_buf;
    buf_sz = amt + 1024;
    recv_buf = new uint8_t[buf_sz];
  }
  return recvfrom( sock, (char*)recv_buf, buf_sz, 0, &src_addr, &n ) ;
} // :: udp_recvfrom

int16_t udp_recvfrom( SOCKET sock, UdpPacket &pkt, uint32_t timeout ) 
{
  struct sockaddr_in addr ;
  pkt.used_sz = pkt.bufsz ;
//  int16_t rc = udp_recvfrom( sock, pkt.buf, pkt.used_sz, (struct sockaddr &)pkt.ip.inet, timeout ) ;
  int16_t rc = udp_recvfrom( sock, pkt.buf, pkt.used_sz, (struct sockaddr &)addr, timeout ) ;
  pkt.ip = addr ;
  return rc ;
} // :: udp_recvfrom

int16_t udp_sendto( SOCKET sock, const struct sockaddr_in &dest, int16_t sz, const void *buf )
{
  int rc = sendto( sock, (char*)buf, sz, 0, (const struct sockaddr*)&dest, sizeof(dest) ) ;

  if (rc != sz)
  {
    printf( "[udp_sendto] msg not sent properly.  sz(%d)  rc(%d)\n", sz, rc ) ;
  }
  else
  {
    printf( "[udp_sendto] sent %d \n", rc ) ;
  }
  return rc ;
} // udp_sendto

int16_t udp_sendto( SOCKET sock, UdpPacket &pkt ) 
{
  return udp_sendto( sock, pkt.ip.inet, pkt.bufsz, pkt.buf ) ;
} // :: udp_sendto
