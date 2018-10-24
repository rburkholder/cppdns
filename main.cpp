/* 
 * File:   main.cpp
 * Author: Raymond Burkholder
 *         raymond@burkholder.net *
 * Created on October 22, 2018, 7:11 PM
 */

#include <iostream>

//#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/icmp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "session.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_datagram_socket/async_receive_from.html
// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_datagram_socket/async_send_to.html
// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_datagram_socket/async_receive.html
// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_datagram_socket/async_send.html

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/networking/protocols.html

//  Data may be read from or written to an unconnected ICMP socket using the 
//    receive_from(), async_receive_from(), send_to() or async_send_to() member functions. 

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/examples/cpp03_examples.html#boost_asio.examples.cpp03_examples.icmp

class server_icmp {
public:
  server_icmp( asio::io_service& io_service )
    : m_endpoint( ip::icmp::v4(), 0 ),
      m_socket( io_service, m_endpoint ) 
    {
      // do_accept();
    }
protected:
private:
  ip::icmp::endpoint m_endpoint;
  ip::icmp::socket m_socket;
};

//  ==============

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/networking/protocols.html

// Data may be read from or written to an unconnected UDP socket using the 
//   receive_from(), async_receive_from(), send_to() or async_send_to() member functions. 
// For a connected UDP socket, use the 
//   receive(), async_receive(), send() or async_send() member functions. 

class server_udp {
public:
  server_udp( asio::io_service& io_service, short port )
    : m_endpoint( ip::udp::v4(), port ),
      m_socket( io_service, m_endpoint ) 
    {
      // do_accept();
    }
protected:
private:
  
  ip::udp::endpoint m_endpoint;
  ip::udp::socket m_socket;
  
};

//  ==============

// Data may be read from or written to a connected TCP socket using the 
//  receive(), async_receive(), send() or async_send() member functions. 
// However, as these could result in short writes or reads, 
// an application will typically use the following operations instead: 
//   read(), async_read(), write() and async_write(). 

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/core/streams.html - short writes/reads


class server_tcp {
public:
  server_tcp( asio::io_service& io_service, short port )
    : m_acceptor( io_service, ip::tcp::endpoint( ip::tcp::v4(), port ) ),
      m_socket( io_service )
  {
    do_accept();
  }

private:
  
  ip::tcp::acceptor m_acceptor;
  ip::tcp::socket m_socket;
  
  void do_accept() {
    m_acceptor.async_accept( m_socket,
        [this]( boost::system::error_code ec ) {
          if (!ec) {
            std::make_shared<session>( std::move( m_socket ) )->start();
          }

          // once one port started, start another acceptance
          // no recursion here as this is in a currently open session
          //   and making allowance for another session
          do_accept();
        });
  }

};

int main( int argc, char* argv[] ) {
  
  int port( 53 );
    
  asio::io_service io_service;

  // https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/signals.html
  boost::asio::signal_set signals( io_service, SIGINT, SIGTERM );
  signals.async_wait( []( const boost::system::error_code& error, int signal_number ){
    if ( !error ) {
      std::cerr << "signal " << signal_number << " received." << std::endl;
    }
  } );
  
  try   {
    if (argc != 2) {
      std::cerr << "Usage: cppdns <port> (using " << port << ")" << std::endl;;
//      return 1;
    }
    else {
      port = std::atoi(argv[1]);
    }

//    server_udp udpServer(io_service, port);
    server_tcp tcpServer( io_service, port );

    io_service.run();
  }
  catch ( std::exception& e )   {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
