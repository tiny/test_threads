/*
  packet.h
  Packet class definition

  © 2024 Robert McInnis <r_mcinnis@solidice.com>
*/
#pragma once
#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "ip_address.h"

#define  UDP_BUFFER_SZ   1400
#define  SOCKET          int
#define  INVALID_SOCKET  -1

class Packet
{
  public:
    std::string str ;
    int fd ;

    Packet( int f, const std::string &s ) : str( s ) { fd = f ; }
} ; // class Packet

class UdpPacket
{
  public:
    IpAddress      ip;
    uint8_t       *buf ; // [UDP_BUFFER_SZ + 1];
    uint16_t       bufsz ;
    uint16_t       used_sz ;

    UdpPacket() : ip( "127.0.0.1", 0 ) { used_sz = 0 ; bufsz = UDP_BUFFER_SZ + 1 ; buf = new uint8_t[ bufsz ] ; }
    ~UdpPacket() { if (buf) delete[] buf ; }  
} ; // class UdpPacket

class PacketPool
{
  private:
    std::queue<UdpPacket*> _pool ;
    std::mutex             _mtx ;

  public:
    PacketPool() {}
    ~PacketPool() {
      while( _pool.size() > 0 )
      {
        UdpPacket *p = _pool.front() ;
        _pool.pop() ;
        delete p ;
      }
    }

    UdpPacket *pop() {
      std::lock_guard<std::mutex> lock( _mtx ) ;
      if (_pool.size() == 0)
        return new UdpPacket() ;
      UdpPacket *p = _pool.front() ;
      _pool.pop() ;
      return p ;
    }
    void push( UdpPacket *&p ) {
      std::lock_guard<std::mutex> lock( _mtx ) ;
      _pool.push( p ) ;
      p = nullptr ;
    }
} ; // class PacketPool 

class PacketQueue
{
  private:
    std::queue<UdpPacket*>  _queue ;
    std::mutex              _mtx ;
    std::condition_variable _sem ;
    int16_t                 _cnt ;

  public:
    PacketQueue() { _cnt = 0 ; }
    ~PacketQueue() {
      while( _queue.size() > 0 )
      {
        UdpPacket *p = _queue.front() ;
        _queue.pop() ;
        delete p ;
      }
    }

    void push( UdpPacket *&p ) {
      std::lock_guard<std::mutex> lock( _mtx ) ;
      _queue.push( p ) ;
      p = nullptr ;
      _cnt++; 
      _sem.notify_one() ;
    }
    UdpPacket *pop() {
      std::unique_lock<std::mutex> lock( _mtx ) ;

      if (_sem.wait_for( lock, std::chrono::milliseconds( 100 ) ) == std::cv_status::timeout) 
        return nullptr ;

      UdpPacket *p = _queue.front() ;
      _queue.pop() ;
      _cnt-- ;
      return p ;
    }
} ; // class PacketQueue

#endif

