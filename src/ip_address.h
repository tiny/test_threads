/*
  ip_address.h
  IpAddress class definition

  © 2024 Robert McInnis <r_mcinnis@solidice.com>
*/
#pragma once 
#ifndef IP_ADDRESS_H
#define IP_ADDRESS_H

#include <stdint.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>

  class IpAddress
  {
    protected:
      void         parse( const std::string &s, std::string &a, uint16_t &b ) {
                     size_t pos = s.find(':');
                     a = ""; b = 0;
                     if (pos != std::string::npos) {
                       a = s.substr(0, pos) ;
                       b = std::stoi(s.substr(pos + 1));
                     }
                   }
    public:
    std::string    ip ;
    std::string    ipstr ;
    uint16_t       port ;
    sockaddr_in    inet ;
    bool           valid;

                   IpAddress( const std::string &addr ) // ie:  127.0.0.1:1234
                   : ip("") 
                   { parse( addr, ip, port ) ;
                     set( ip, port ) ; 
                   }
                   IpAddress( const std::string &addr, uint16_t p ) 
                   : ip(addr) 
                   { set( addr, p ) ; }
                   IpAddress( const IpAddress &i ) 
                   { *this = i ; }

                   bool operator< ( const IpAddress &c ) const
                   {
                     return (inet.sin_addr.s_addr == c.inet.sin_addr.s_addr) ? 
                     (inet.sin_port < c.inet.sin_port) : 
                     (inet.sin_addr.s_addr < c.inet.sin_addr.s_addr) ;
                   } 
                   bool operator== ( const IpAddress &c ) const
                   {
                     return ((inet.sin_port == c.inet.sin_port) &&
                     (inet.sin_addr.s_addr == c.inet.sin_addr.s_addr) &&
                     (inet.sin_family == c.inet.sin_family)) ;
                   }
    IpAddress     &operator= ( const IpAddress&i )
                   { if (this != &i) {
                       ip = i.ip ;
                       ipstr = i.ipstr ;
                       port = i.port ;
                       valid = i.valid ;
                       memcpy( &inet, &i.inet, sizeof(inet)) ;
                     }
                     return *this ;
                   }
    IpAddress     &operator= ( struct sockaddr_in &addr )
                   { if (&addr != &inet) {
                       inet = addr ;
                       port = ntohs( addr.sin_port ) ;
                       inet.sin_family = AF_INET;
                       char buf[256] ;
                       valid = (inet_ntop( AF_INET, &addr, buf, 255 ) != NULL);
                       ip = buf;
                       ipstr = ip + ":" + std::to_string(port) ;
                     }
                     return *this ;
                   }
    void           set( const std::string &addr, uint16_t p ) {
                     ip = addr ;
                     port = p ;
                     inet.sin_family = AF_INET;
                     inet.sin_port = htons(port);
                     valid = (inet_pton(AF_INET, ip.c_str(), &inet.sin_addr) > 0) ;
                     ipstr = ip + ":" + std::to_string(p) ;
                     printf( "valid(%s) ipstr(%s) \n", valid ? "true" : "false", ipstr.c_str() ) ;
                   }
  } ; // class IpAddress

#endif
