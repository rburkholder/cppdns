/* 
 * File:   main.cpp
 * Author: Raymond Burkholder
 *         raymond@burkholder.net *
 * Created on October 22, 2018, 7:11 PM
 */

#include <array>
#include <memory>
#include <string>
#include <iostream>

#include <boost/bind.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/icmp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/enable_shared_from_this.hpp>

//#include "session.h"

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
  server_icmp( asio::io_context& io_context )
    : m_endpoint( ip::icmp::v4(), 0 ),
      m_socket( io_context, m_endpoint ) 
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
// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/tutorial/tutdaytime7.html

// Data may be read from or written to an unconnected UDP socket using the 
//   receive_from(), async_receive_from(), send_to() or async_send_to() member functions. 
// For a connected UDP socket, use the 
//   receive(), async_receive(), send() or async_send() member functions. 

class server_udp {
public:
  server_udp( asio::io_context& io_context, short port )
    : m_socket( io_context, ip::udp::endpoint( ip::udp::v4(), port ) ) 
    {
      start_receive();
    }
protected:
private:
  
  ip::udp::socket m_socket;
  std::array<std::uint8_t, 1024> m_bufReceive;
  ip::udp::endpoint m_endpointRemote; // are multiple endpoints required?
  
  void send_complete( std::shared_ptr<std::string> message, const boost::system::error_code& ec, std::size_t bytes_transferred ) {
    // used for destroying the message for now
  }
  
  void handle_receive( const boost::system::error_code& ec, std::size_t /*bytes_transferred*/ ) {
    if ( !ec ) {
      
      std::shared_ptr<std::string> message( new std::string( "out test" ) );
      
      m_socket.async_send_to(
        asio::buffer( *message ),
        m_endpointRemote,
        boost::bind( 
          &server_udp::send_complete, this, 
          message, 
          asio::placeholders::error,
          asio::placeholders::bytes_transferred
        )
      );
      
      start_receive();  // start over again
    }
  }
  
  void start_receive() {
    m_socket.async_receive_from(
      asio::buffer( m_bufReceive ),
      m_endpointRemote,
      boost::bind(
        &server_udp::handle_receive, this,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
      )
    );
  }
  
};

//  ==============

// Data may be read from or written to a connected TCP socket using the 
//  receive(), async_receive(), send() or async_send() member functions. 
// However, as these could result in short writes or reads, 
// an application will typically use the following operations instead: 
//   read(), async_read(), write() and async_write(). 

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/core/streams.html - short writes/reads

// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/tutorial/tutdaytime3.html
// https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/example/cpp11/echo/async_tcp_echo_server.cpp

class session: public boost::enable_shared_from_this<session> {
public:
  
  typedef boost::shared_ptr<session> pointer;
  
  static pointer create( asio::io_context& io_context ) {
    return pointer( new session( io_context ) );
  }
  
  ip::tcp::socket& socket() { return m_socket; }
  
  void start() {
    m_message = "test this";  // keep the data valid until the asynchronous operation is complete
    
    asio::async_write( 
      m_socket, asio::buffer( m_message ),
      boost::bind(
        &session::handle_write_state, shared_from_this(),
        asio::placeholders::error,
        asio::placeholders::bytes_transferred
        )
      );
  }
  
protected:
private:
  
  ip::tcp::socket m_socket;
  std::string m_message;
  enum { max_buf_length = 1024 };
  std::array<std::uint8_t, max_buf_length> m_bufReceive;
  
  session( asio::io_context& io_context )
    : m_socket( io_context )
  {
  }
  
  void start_read() {
    
    auto self( shared_from_this() );
    
    m_socket.async_read_some(
      asio::buffer( 
        m_bufReceive, max_buf_length ), 
        [this, self]( const boost::system::error_code& ec, std::size_t length ){
          if ( !ec ) {
            // process buffer, then
            start_read();
          }
          else {
            // fix things and try again?
          }
        }
    );
  }

  void handle_write_state( const boost::system::error_code& ec, size_t bytes_transferred ) {
    
  }  
  
};


class server_tcp {
public:
  server_tcp( asio::io_context& io_context, short port )
    : m_acceptor( io_context, ip::tcp::endpoint( ip::tcp::v4(), port ) )
  {
    start_accept(); // accept first connection
  }

private:
  
  ip::tcp::acceptor m_acceptor;
  
  void handle_accept( session::pointer new_connection, const boost::system::error_code& ec ) {
    if ( !ec ) {
      new_connection->start();  // manage existing connection
      start_accept();  // accept another connection
    }
    else {
      // repair and restart?
    }
  }
  
  void start_accept() {
    session::pointer new_connection = session::create( m_acceptor.get_executor().context() );
    
    m_acceptor.async_accept( 
      new_connection->socket(),
      boost::bind( &server_tcp::handle_accept, this, new_connection, asio::placeholders::error )
    );
    
  }

};

//  ==============

int main( int argc, char* argv[] ) {
  
  int port( 53 ); // default but can be over-written
    
  asio::io_context io_context;

  // https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/signals.html
  boost::asio::signal_set signals( io_context, SIGINT, SIGTERM );
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
    server_tcp tcpServer( io_context, port );
    server_udp udpServer( io_context, port );
    server_icmp icmpServer( io_context );

    io_context.run();
  }
  catch ( std::exception& e )   {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
