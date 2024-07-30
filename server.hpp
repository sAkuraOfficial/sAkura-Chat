#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <iostream>

using boost::asio::ip::tcp;

class Session
{
  public:
    using DisconnectHandler = std::function<void(Session *)>;
    void set_disconnect_handler(DisconnectHandler handler)
    {
        _disconnect_handler = handler;
    }
    Session(boost::asio::io_context &ioc)
        : _socket(ioc)
    {
    }

    tcp::socket &Socket()
    {
        return _socket;
    }
    void Start();

  private:
    // handle回调函数

    void handle_read(const boost::system::error_code &erro, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code &erro);
    void handle_write_length(const boost::system::error_code &erro);
    void handle_write_length_all(const boost::system::error_code &erro, char *_data_length_to_all, bool isLast);
    void handle_write_all(const boost::system::error_code &erro,char* _data_to_all,bool isLast);
    tcp::socket _socket;
    DisconnectHandler _disconnect_handler;
    enum
    {
        max_length = 4096
    };
    char _data[max_length];
    char _data_length[sizeof(std::size_t)] = {0};
};

class Server
{
  public:
    Server(boost::asio::io_context &ioc, short port);

  private:
    void start_accept();                                                              // 启用一个描述符
    void handle_accept(Session *new_session, const boost::system::error_code &error); // 有连接来的时候，会回调
    boost::asio::io_context &_ioc;
    boost::asio::ip::tcp::acceptor _acceptor;
};