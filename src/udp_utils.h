/*
 */
#pragma once

#include <stdint.h>
#include <unistd.h>
#include "ip_address.h"
#include "packet.h"

const char *dump_hex( const char *buf, int16_t n, char *dst ) ;
IpAddress get_local_address() ;
SOCKET udp_init(uint16_t port) ;
void udp_close( SOCKET &s ) ;
int16_t udp_recvfrom( SOCKET _sock, uint8_t *&recv_buf, uint16_t &buf_sz, struct sockaddr &src_addr, uint32_t timeout = 100 ) ;
int16_t udp_recvfrom( SOCKET _sock, UdpPacket &pkt, uint32_t timeout = 100 ) ;
int16_t udp_sendto( SOCKET _sock, const struct sockaddr_in &dest, int16_t sz, const void *buf ) ;
int16_t udp_sendto( SOCKET _sock, UdpPacket &pkt ) ;

