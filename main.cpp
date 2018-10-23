/* 
 * File:   main.cpp
 * Author: Raymond Burkholder
 *         raymond@burkholder.net *
 * Created on October 22, 2018, 7:11 PM
 */

#include <iostream>

#include <boost/asio.hpp>

#include "session.h"

class server_tcp {
public:
  server_tcp(boost::asio::io_service& io_service, short port)
    : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept() {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec) {
            std::make_shared<session>(std::move(socket_))->start();
          }

          // once one port started, start another acceptance
          // no recursion here as this is in a currently open session
          //   and making allowance for another session
          do_accept();
        });
  }

  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
};

int main(int argc, char* argv[]) {
    
  int port( 53 );
    
  boost::asio::io_service io_service;

  try   {
    if (argc != 2) {
      std::cerr << "Usage: cppdns <port> (using " << port << ")\n";
//      return 1;
    }
    else {
      port = std::atoi(argv[1]);
    }

//    server_udp udpServer(io_service, port);
    server_tcp tcpServer(io_service, port);

    io_service.run();
  }
  catch (std::exception& e)   {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
