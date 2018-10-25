/* 
 * File:   session.cpp
 * Author: Raymond Burkholder
 *         raymond@burkholder.net
 * 
 * Created on October 22, 2018, 8:50 PM
 */

#include <iostream>
#include <iomanip>
#include <cstring>

//#include "protocol/ethernet.h"

#include "common.h"
//#include "hexdump.h"
#include "session.h"

void session::start() {
  std::cout << "start begin: " << std::endl;
  try {
    do_read();
  }
  catch(...) {
    std::cout << "do_read issues" << std::endl;
  }
  std::cout << "start end: " << std::endl;
}


void session::do_read() {
  //std::cout << "do_read begin: " << std::endl;
  // going to need to perform serialization as bytes come in (multiple packets joined together)?
  auto self(shared_from_this());
  m_vRx.resize( max_length );
  m_socket.async_read_some(boost::asio::buffer(m_vRx),
      [this, self](boost::system::error_code ec, const std::size_t lenRead)
      {
        //std::cout << "async_read begin: " << std::endl;
        if (!ec) {
          std::cout << ">>> total read length: " << lenRead << std::endl;

          // inbound data arrives
          // if reassembly data available;
          //    append to reassembly buffer, 
          //    clear inbound buffer
          //    move adjusted reassembly buffer to inbound buffer
          // loop on input buffer:
          //   if less then header size:
          //     move remaining octets to reassembly buffer
          //     exit loop for more
          //   if less than header.length:
          //     move remaining octets to reassembly buffer
          //     exit loop for more
          //   if >= header.length: 
          //     process packet
          //     move beginning to end
          //     loop for more
          
          std::size_t length = lenRead;
          
          if ( 0 != m_vReassembly.size() ) {
            m_vReassembly.insert( m_vReassembly.end(), m_vRx.begin(), m_vRx.begin() + lenRead );
            m_vRx.clear();
            m_vRx = std::move( m_vReassembly );
            length = m_vRx.size();
          }
          
          vByte_iter_t iterBegin = m_vRx.begin();
          vByte_iter_t iterEnd = iterBegin + length;
          
//          ofp141::ofp_header* pOfpHeader;
          bool bReassemble( false );
          
          bool bLooping( true );
          
          while ( bLooping ) {
            
            auto nOctetsRemaining = iterEnd - iterBegin;

//            if ( nOctetsRemaining < sizeof( ofp141::ofp_header ) ) bReassemble = true;
//            else {
//              pOfpHeader = new( &(*iterBegin) ) ofp141::ofp_header;
//              if ( nOctetsRemaining < pOfpHeader->length ) bReassemble = true;
//            }

            if ( bReassemble ) {
              // wait for more octets
              m_vReassembly = std::move( m_vRx );
              bLooping = false;
            }
            else {
              // normal packet processing
              uint8_t* pBegin = &(*iterBegin);
//              ProcessPacket( pBegin, pBegin + pOfpHeader->length );
//              iterBegin += pOfpHeader->length;
              bLooping = iterBegin != iterEnd;
           }
          }
          
          std::cout << "<<< end." << std::endl;

        } // end if ( ec )
        else {
          if ( 2 == ec.value() ) {
            assert( 0 );  // 'End of file', so need to re-open, or abort
          }
          else {
            std::cout << "read error: " << ec.value() << "," << ec.message() << std::endl;
          }
            // do we do another read or let it close implicitly or close explicitly?
        } // end else ( ec )
        //std::cout << "async_read end: " << std::endl;
        do_read();
      }); // end lambda
  //std::cout << "do_read end: " << std::endl;
}

void session::ProcessPacket( uint8_t* pBegin, const uint8_t* pEnd ) {
}


//void tcp_session::do_write(std::size_t length) {
//  auto self(shared_from_this());
//  boost::asio::async_write(
//    m_socket, boost::asio::buffer(m_vTx),
//      [this, self](boost::system::error_code ec, std::size_t /*length*/)
//      {
        //if (!ec) {
        //  do_read();
        //}
//      });
//}

// TODO:  need a write queue, need to update the xid value in the header
//    so, create a method or class for handling queued messages and transactions
//void tcp_session::do_write( vChar_t& v ) {
//  auto self(shared_from_this());
//  boost::asio::async_write(
//    m_socket, boost::asio::buffer( v ),
 //     [this, self](boost::system::error_code ec, std::size_t len )
 //     {
//        std::cout << "do_write complete:" << ec << "," << len << std::endl;
        //if (!ec) {
        //  do_read();
        //}
//      });
//}

void session::do_write() {
  auto self(shared_from_this());
  //std::cout << "do_write start: " << std::endl;
  boost::asio::async_write(
    m_socket, boost::asio::buffer( m_vTxInWrite ),
      [this, self](boost::system::error_code ec, std::size_t len )
      {
        UnloadTxInWrite();
//        std::cout << "do_write atomic: " << 
        if ( 2 <= m_transmitting.fetch_sub( 1, std::memory_order_release ) ) {
          //std::cout << "do_write with atomic at " << m_transmitting.load( std::memory_order_acquire ) << std::endl;
          LoadTxInWrite();
          do_write();
        }
        //std::cout << "do_write complete:" << ec << "," << len << std::endl;
        //if (!ec) {
        //  do_read();
        //}
      });
}

void session::GetAvailableBuffer( vByte_t& v ) {
  std::unique_lock<std::mutex> lock( m_mutex );
  if ( m_qBuffersAvailable.empty() ) {
    m_qBuffersAvailable.push( vByte_t() );
  }
  v = std::move( m_qBuffersAvailable.front() );
  m_qBuffersAvailable.pop();
}

vByte_t session::GetAvailableBuffer() {
  std::unique_lock<std::mutex> lock( m_mutex );
  if ( m_qBuffersAvailable.empty() ) {
    m_qBuffersAvailable.push( vByte_t() );
  }
  vByte_t v = std::move( m_qBuffersAvailable.front() );
  m_qBuffersAvailable.pop();
  return v;
}

void session::QueueTxToWrite( vByte_t v ) {
  std::unique_lock<std::mutex> lock( m_mutex );
  //std::cout << "QTTW: " << m_transmitting.load( std::memory_order_acquire ) << std::endl;
  if ( 0 == m_transmitting.fetch_add( 1, std::memory_order_acquire ) ) {
    //std::cout << "QTTW1: " << std::endl;
    m_vTxInWrite = std::move( v );
    do_write();
  }
  else {
    //std::cout << "QTTW2: " << std::endl;
    m_qTxBuffersToBeWritten.push( std::move( v ) );
    //m_qTxBuffersToBeProcessed.push( std::move( v ) );
    //m_qTxBuffersToBeWritten.back() = std::move( v );
  }
}

void session::LoadTxInWrite() {
  std::unique_lock<std::mutex> lock( m_mutex );
  //std::cout << "LoadTxInWrite: " << std::endl;
  //assert( 0 < m_transmitting.load( std::memory_order_acquire ) );
  m_vTxInWrite = std::move( m_qTxBuffersToBeWritten.front() );
  m_qTxBuffersToBeWritten.pop();
}

void session::UnloadTxInWrite() {
  std::unique_lock<std::mutex> lock( m_mutex );
  //std::cout << "UnloadTxInWrite: " << std::endl;
  m_vTxInWrite.clear();
  m_qBuffersAvailable.push( std::move( m_vTxInWrite ) );

}
