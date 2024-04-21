#include "server.hpp"
#include <iostream>
void Session::Start()
{
    memset(_data, 0, max_length);
    _socket.async_read_some(
        boost::asio::buffer(_data, max_length),
        std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void Session::handle_read(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (!error)
    {
        std::cout << "server receive data is:" << _data << std::endl;
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(_data, bytes_transferred),
            std::bind(
                &Session::handle_write,
                this,
                std::placeholders::_1
            )
        );
    }
    else
    {
        std::cout << "read error" << std::endl;
        delete this; // 销毁掉当前session，断开连接
        // 这种写法有隐患，实际生产不会使用
    }
}

void Session::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
        // 此时已经写完了，
        // 清空data，然后继续监听
        std::cout << "send:" << _data << std::endl;
        memset(_data, 0, max_length);
        _socket.async_read_some(
            boost::asio::buffer(_data, max_length),
            std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2)
        );
    }
    else
    {
        std::cout << "write error" << std::endl;
        delete this; // 销毁掉当前session，断开连接
        // 这种写法有隐患，实际生产不会使用
    }
}

Server::Server(boost::asio::io_context &ioc, short port)
    : _ioc(ioc), _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    std::cout << "start server success,on port" << port << std::endl;
    start_accept();
}

void Server::start_accept()
{
    Session *new_session = new Session(_ioc); // 传入上下文
    // session构建的时候，会在内部创建一个socket,socket就是服务员

    // acceptor相当于大堂经理
    _acceptor.async_accept(
        new_session->Socket(), // 分配给服务员
        std::bind(
            &Server::handle_accept,
            this,
            new_session,
            std::placeholders::_1
            // 个人心得！
            // boost调用这个回调函数的时候，他只能调用A(error_code)只有一个参数类型！
            // 但是很显然，我们的new_session函数(Session ,error_code )有两个参数！！
            // 我们使用bind函数，让他设置只有一个占位符，这样子bind就会生产一个包装好的新函数！！这样子就能被boost成功调用
            // 同时，我们还实现了传入额外数据到回调函数中
            // 嘎嘎厉害！！！
        )
    );
    // 这个函数是异步的，因此会启动新的线程，
    // 然后当前函数执行完毕
}

void Server::handle_accept(Session *new_session, const boost::system::error_code &error)
{
    if (!error)
    {
        // 正常
        // 此时代表已经正常连接到了一个客户端
        // 我们开启消息监听
        new_session->Start(); // socket开始读咯,开始收发客户端的消息
    }
    else
    {
        delete new_session; // 出现错误
    }

    // 前面已经处理完了一个连接
    // 再次调用accept,等待新的客人
    start_accept();
}

int main()
{
    try
    {
        boost::asio::io_context ioc;
        using namespace std;
        Server s(ioc, 10086);
        ioc.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "exception:" << e.what() << std::endl;
    }
}