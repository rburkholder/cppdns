/* 
 * File:   session.h
 * Author: Raymond Burkholder
 *         raymond@burkholder.net
 *
 * Created on October 22, 2018, 8:50 PM
 */

#ifndef SESSION_H
#define SESSION_H

#include <queue>
#include <mutex>
#include <atomic>

#include <boost/asio.hpp>

#include "common.h"
//#include "bridge.h"

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(boost::asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)), m_transmitting( 0 )
  { 
      std::cout << "session construct" << std::endl;
  }
    
    virtual ~session() {
      std::cout << "session destruct" << std::endl;
    }

  void start();

private:

  enum { max_length = 17000 };  // not sure where I got 17k from.

// need to use a function object instead so that the functions are embedded.
// can be stack based function object or a heap based function object
// supplied by the primary data structure being built
// then can embed the related dependencies for building the various fields.  
  struct build {
    typedef std::function<size_t(void)> fSize_t;
    typedef std::function<void(vByte_t&)> fAppend_t;
  };
  
  std::vector<build> m_vBuild; // used for building up a packet from composite structures.

  void do_read();
  
  //void do_write(std::size_t length);

  // TODO:  need a write queue, need to update the xid value in the header
  //    so, create a method or class for handling queued messages and transactions
  //void do_write( vChar_t& v );
  void do_write();

  boost::asio::ip::tcp::socket m_socket;
  
  vByte_t m_vRx;
  vByte_t m_vReassembly;
  typedef vByte_t::iterator vByte_iter_t;
  
  typedef std::queue<vByte_t> qBuffers_t; 
  
  std::mutex m_mutex;
  std::atomic<uint32_t> m_transmitting;
  
  qBuffers_t m_qBuffersAvailable;
  qBuffers_t m_qTxBuffersToBeWritten;
  vByte_t m_vTxInWrite;
  
//  Bridge m_bridge;
  
  void ProcessPacket( uint8_t* pBegin, const uint8_t* pEnd );
  
  void GetAvailableBuffer( vByte_t& v );
  vByte_t GetAvailableBuffer();
  void QueueTxToWrite( vByte_t  );
  void LoadTxInWrite();
  void UnloadTxInWrite();
  
};


#endif /* TCP_SESSION_H */

