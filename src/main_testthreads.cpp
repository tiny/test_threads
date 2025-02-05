/*
  main_testthreads.cpp
*/
#include <stdio.h>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable> 
#include <string>
#include <cstdlib>
#include <stdint.h>

using namespace std ;

//--[ basic producer / consumer test ]-----------------------------------------
bool  g_done = false ;
mutex g_mtx ;
condition_variable g_sem ;
queue<string> g_strings ;
uint32_t  tick = 0 ;
uint32_t  tock = 0 ;

void consume_t( int32_t tid ) 
{
  printf( "consume.%d  start\n", tid ) ;
  uint32_t  cnt = 0 ;
  while (!g_done)
  {
    this_thread::sleep_for( chrono::milliseconds( (rand() % 100) + 50 )) ;
    while (g_strings.size() > 0) 
    {
      {
        unique_lock<mutex> lock(g_mtx) ;
        if (g_strings.size() == 0)
          break ; 
        g_sem.wait( lock, []() { return !g_strings.empty() ; } ) ;
        if (g_strings.size() == 0)
          break ; 
        string s = g_strings.front() ;
        g_strings.pop() ;
        cnt++ ;
        tock++ ;
        printf( "consume.%d  %s  (cnt: %d / %d)\n", tid, s.c_str(), cnt, tock ) ;
      }
    } 
  }
  printf( "consume.%d  end\n", tid ) ;
} // :: consume_t

void produce_t( int32_t tid ) 
{
  printf( "produce.%d  start\n", tid ) ;
  int32_t     cnt = 0 ;
  while (!g_done) 
  {
    {
      unique_lock<mutex> lock(g_mtx) ;
      if (g_done) 
        break ; 
      string s = string("string ") + to_string(tid) + "." + to_string(cnt) ;  
      cnt++ ;
      tick++ ;
      g_strings.push( s ) ;
      g_sem.notify_one() ;
      printf( "produce.%d  %s  (cnt: %d / %d)\n", tid, s.c_str(), cnt, tick ) ;
    }
    this_thread::sleep_for( chrono::milliseconds( (rand() % 400) + 100 )) ;
  }
  printf( "produce.%d  end  (cnt: %d)\n", tid, cnt ) ;
} // :: produce_t

void producer_consumer_test( int32_t n_producers, int32_t n_consumers ) 
{
  srand( time(NULL) ) ; 

  vector<thread*>  threads ;
  for( int i = 0 ; i < n_producers ; i++ )
  {
    threads.push_back( new thread( [i]() { 
      produce_t( i ) ;      
    } ) ) ;
  }
  for( int i = 0 ; i < n_consumers ; i++ )
  {
    threads.push_back( new thread( [i]() { 
      consume_t( i ) ;      
    } ) ) ;
  }

  chrono::_V2::system_clock::time_point   now_ts = chrono::system_clock::now() ;
  chrono::_V2::system_clock::time_point   end_ts = now_ts + chrono::seconds(1) ; 

  printf( "running...\n" ) ; 
  do {
    this_thread::sleep_for( chrono::milliseconds(100) ) ;
    now_ts = chrono::system_clock::now() ; 
  } while (now_ts < end_ts) ;
  g_done = true ;

  printf( "joining threads...\n" ) ; 
  for( auto t : threads )
  {
    t->join() ;
    delete t ;
  } 
  threads.clear() ;
  
  printf( "q_strings.size:  %ld \n", g_strings.size() ) ;  
  printf( "done.\n" ) ; 
} // :: producer_consumer_test

//--[ client/server ]----------------------------------------------------------
#include "udp_utils.h"

PacketPool  g_packetPool ;
PacketQueue g_packetQueue ;
SOCKET      g_sock = -1 ;
uint32_t    g_cnt = 0 ;
uint32_t    g_userid = 0 ;

struct Message 
{
  uint32_t  msgid ;
  uint32_t  cnt ;
  uint16_t  payload_sz ;
  char      payload[2] ;

  void  to_host() {
    msgid = ntohl( msgid ) ;
    cnt = ntohl( cnt ) ;
    payload_sz = ntohs( payload_sz ) ;
  }
  void  to_network() {
    msgid = htonl( msgid ) ;
    cnt = htonl( cnt ) ;
    payload_sz = htons( payload_sz ) ;
  }
  uint16_t size() {
    return sizeof(msgid) + sizeof(cnt) + sizeof(payload_sz) + payload_sz ;
  } 
} ; // Message

void server_process_t( SOCKET sock ) 
{
  printf( "server_process_t\n" ) ;
  UdpPacket  *pkt = nullptr ;
  Message    *m = nullptr ; 
  while (!g_done) {
    pkt = g_packetQueue.pop() ;
    if (pkt == nullptr) {
      continue ;
    }
    // process the packet
    m = (Message*)pkt->buf ;  

    printf( "recvd:  %d.%d.%d\n", pkt->ip.port, m->msgid, m->cnt ) ;
    g_cnt++ ;
    // push the packet back on the pool
    g_packetPool.push( pkt ) ;
  }
  printf( "server_process_t end\n" ) ;
} // :: server_process_t

void server_test( uint16_t port ) 
{
  printf( "server_test\n" ) ;
  g_sock = udp_init( port ) ;

  if (g_sock < 0) {
    printf( "error binding to port %d \n", port ) ;
    return ;
  }

  vector<thread*>  threads ;
  for( int i = 0 ; i < 2 ; i++ )
  {
    threads.push_back( new thread( []() { 
      server_process_t( g_sock ) ;      
    } ) ) ;
  }

  UdpPacket  *pkt = nullptr ;
  int16_t     rc = 0 ;
  while (!g_done) {
    if (pkt == nullptr)
      pkt = g_packetPool.pop() ;
    rc = udp_recvfrom( g_sock, *pkt ) ;
    if (rc < 0) {
      printf( "error receiving packet  (%d)\n", rc ) ;
      break ;
    } 
    else if (rc > 0) {
      ((Message*)pkt->buf)->to_host() ;
      printf( "recv'd pkt from %s:%d   sz: %d\n", pkt->ip.ip.c_str(), pkt->ip.port, pkt->bufsz ) ;
      g_packetQueue.push( pkt ) ;
    }
  }
  udp_close( g_sock ) ;
  printf( "server_test end\n" ) ;
} // :: server_test

void generate_pkt( UdpPacket &pkt ) 
{
  memset( pkt.buf, 0x00, pkt.bufsz ) ;
  Message *m = (Message*)pkt.buf ;
  m->msgid = 100 ;
  m->cnt = g_cnt++ ;
  m->payload_sz = 0 ;
  pkt.used_sz = m->size() ;
  m->to_network() ;

  ::usleep( 250 * 1000 );
} // :: generate_pkt

void client_test( const string &ip, uint16_t port ) 
{
  printf( "client_test\n" ) ;
  g_sock = udp_init( 0 ) ;
  if (g_sock < 0) {
    printf( "error creating socket\n" ) ;
    return ;
  }

  UdpPacket   pkt ;
  int16_t     rc = 0 ;

  pkt.ip.set( ip, port ) ;
  while (!g_done) {
    generate_pkt( pkt ) ;
    printf( "about to send\n") ;
    rc = udp_sendto( g_sock, pkt ) ;
    printf( "sent (rc = %d)\n", rc ) ;
    if (rc < 0) {
      printf( "error sending packet\n" ) ;
      break ;
    } 
  }
  udp_close( g_sock ) ;
  printf( "client_test end\n" ) ;
} // :: client_test 

//--[ main program ]-----------------------------------------------------------
#define N_PRODUCERS  8
#define N_CONSUMERS  2

struct Settings 
{
  bool     is_pc ;
  int16_t  n_producers ;
  int16_t  n_consumers ;
  bool     is_cs ;
  bool     is_client ;
  string   ip ;
  bool     is_server ;
  uint16_t port ;

  Settings() {
    is_pc = false ;
    n_producers = N_PRODUCERS ;
    n_consumers = N_CONSUMERS ;
    is_cs = false ;
    is_client = false ;
    ip = "" ;
    is_server = false ;
    port = 15005 ;
  }
} ;

Settings  settings ;

void usage() 
{
  printf( "usage:  <[pc [<a> <b>]]|[cs <c <ip[:port]>|s [port]]> \n" ) ;
  printf( "  pc   - producer / consumer test\n" ) ;
  printf( "  a    - number of producers (default: %d)\n", N_PRODUCERS ) ;
  printf( "  b    - number of consumers (default: %d)\n", N_CONSUMERS ) ;
  printf( "  cs   - client / server test\n" ) ;  
  printf( "  c    - client\n" ) ;
  printf( "  s    - server\n" ) ;
  printf( "  ip   - ip of server\n" ) ; 
  printf( "  port - port of server (default: 15005)\n" ) ;
} // :: usage

int16_t process_args( int argc, const char *argv[] )
{
  if (argc <= 1)
    return -1 ;

  string  arg = argv[1] ;
  if (arg == "pc") {
    settings.is_pc = true ;
    if (argc > 2) {
      settings.n_producers = atoi( argv[2] ) ;
      if (settings.n_producers < 1)
        settings.n_producers = N_PRODUCERS ;
    }
    if (argc > 3) {
      settings.n_consumers = atoi( argv[3] ) ;
      if (settings.n_consumers < 1)
        settings.n_consumers = N_CONSUMERS ;
    }
  } else if (arg == "cs") {
    settings.is_cs = true ;
    if (argc > 2) {
      arg = argv[2] ;
      if (arg == "c") {
        settings.is_client = true ;
        if (argc > 3) {
          settings.ip = argv[3] ;
          size_t  pos = settings.ip.find( ':' ) ;
          if (pos != string::npos) {
            string   port_str = settings.ip.substr( pos + 1 ) ;
            settings.ip = settings.ip.substr( 0, pos ) ;
            settings.port = atoi( port_str.c_str() ) ;
          }
        }
      } else if (arg == "s") {
        settings.is_server = true ;
        if (argc > 3) {
          settings.port = atoi( argv[3] ) ;
        }
      }
    }
  }

  return 0 ;  
} // :: process_args  

//-----------------------------------------------------------------------------
int main( int argc, const char *argv[] )
{
  if (argc <= 1) {
    usage() ;
    return 0 ;
  }
  if ((process_args( argc, argv ) < 0) || (!settings.is_pc && !settings.is_cs)) { 
    usage() ;
    return -1 ;
  }

  if (settings.is_pc)
    producer_consumer_test( settings.n_producers, settings.n_consumers ) ;
  else if (settings.is_cs) {
    if (!settings.is_client && !settings.is_server) {
      usage() ;
      return -1 ;
    }
    if (settings.is_client) {
      printf( "client\n" ) ;
      printf( "  ip:   %s\n", settings.ip.c_str() ) ;
      printf( "  port: %d\n", settings.port ) ;
      client_test( settings.ip, settings.port ) ;
    } else if (settings.is_server) {
      printf( "server\n" ) ;
      printf( "  port: %d\n", settings.port ) ;
      server_test( settings.port ) ;
    } 
  }

  return 0 ;
} // :: main
