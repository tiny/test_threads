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

void producer_consumer_test( int32_t N_PRODUCERS, int32_t N_CONSUMERS ) 
{
  srand( time(NULL) ) ; 

  vector<thread*>  threads ;
  for( int i = 0 ; i < N_PRODUCERS ; i++ )
  {
    threads.push_back( new thread( [i]() { 
      produce_t( i ) ;      
    } ) ) ;
  }
  for( int i = 0 ; i < N_CONSUMERS ; i++ )
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


//--[ main program ]-----------------------------------------------------------
#define N_PRODUCERS  8
#define N_CONSUMERS  2

int main( int argc, const char *argv[] )
{
  producer_consumer_test( N_PRODUCERS, N_CONSUMERS) ;

  return 0 ;
} // :: main
