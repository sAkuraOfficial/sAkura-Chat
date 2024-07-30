#include "server.hpp"
#include <boost/json.hpp>
#include <curl/curl.h>
#include <iostream>
std::vector<Session *> clients;
void Session::Start()
{
    memset(_data, 0, max_length);
    _socket.async_read_some(
        boost::asio::buffer(_data, max_length),
        std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2)
    );
}
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    output->append((char *)contents, size * nmemb);
    return size * nmemb;
}
void Session::handle_read(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    try
    {

        if (!error)
        {
            std::cout << "server receive data is:" << _data << std::endl;

            auto msg = boost::json::parse(std::string(_data)).as_object();
            if (msg["type"].as_string().compare("get_AI") == 0)
            {
                // 问ai
                auto msg_to_ai = msg.at("message").as_string();
                std::string json_to_ai;
                std::string json_to_ai_result;
                {
                    // 构建json
                    boost::json::object json;
                    json["model"] = "gpt-3.5-turbo";
                    json["messages"] = {{{{"role", "user"}, {"content", msg_to_ai.c_str()}}}};
                    json_to_ai = boost::json::serialize(json);
                }
                {
                    CURL *curl;
                    CURLcode res;
                    curl = curl_easy_init();
                    if (curl)
                    {
                        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
                        curl_easy_setopt(curl, CURLOPT_URL, "https://api.chatanywhere.tech/v1/chat/completions");
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
                        struct curl_slist *headers = NULL;
                        headers = curl_slist_append(headers, "Authorization: Bearer sk-k9Lqis415fGSBwEsJEtJeWNp6nz8pXDJCJQ0gO2rsksaXimu");
                        headers = curl_slist_append(headers, "User-Agent: Apifox/1.0.0 (https://apifox.com)");
                        headers = curl_slist_append(headers, "Content-Type: application/json");
                        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                        const char *data = json_to_ai.c_str();

                        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                        // 将输出字符串的地址传递给回调函数
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_to_ai_result);
                        res = curl_easy_perform(curl);
                        // std::cout << json_to_ai_result << std::endl;
                    }
                    curl_easy_cleanup(curl);
                }
                // 广播

                boost::json::object val;
                val["type"] = "AI_RESULT";
                val["sender"] = {
                    {"sender_ID", "SERVER_GPT"}
                };
                val["message"] = boost::json::parse(json_to_ai_result).as_object();

                auto result__ = boost::json::serialize(val);

                std::cout << "send::::" << std::endl;
                std::cout << result__.c_str() << std::endl;

                char *data_to_all = new char[max_length]{0};
                char *data_length_to_all = new char[sizeof(std::size_t)]{0};
                std::size_t bytes = result__.length();
                strcpy(data_to_all, result__.c_str());
                strcpy(data_length_to_all, reinterpret_cast<char *>(&bytes));

                for (auto &i : clients) // 广播给所有用户
                {
                    bool isLast = i == clients.back(); // 是否最后一个

                    boost::asio::async_write(
                        i->Socket(),
                        boost::asio::buffer(data_length_to_all, sizeof(std::size_t)),
                        std::bind(
                            &Session::handle_write_length_all,
                            this,
                            std::placeholders::_1,
                            data_length_to_all,
                            isLast
                        )
                    );
                    boost::asio::async_write(
                        i->Socket(),
                        boost::asio::buffer(data_to_all, bytes),
                        std::bind(
                            &Session::handle_write_all, // 广播给所有用户
                            this,
                            std::placeholders::_1,
                            data_to_all,
                            isLast
                        )
                    );
                }
            }
            else
            {
                // 直接广播
                char *data_to_all = new char[max_length]{0};
                char *data_length_to_all = new char[sizeof(std::size_t)]{0};
                // strcpy_s(data_to_all, bytes_transferred, _data);
                strcpy(data_to_all, _data);
                strcpy(data_length_to_all, reinterpret_cast<char *>(&bytes_transferred));

                for (auto &i : clients) // 广播给所有用户
                {
                    bool isLast = i == clients.back(); // 是否最后一个

                    boost::asio::async_write(
                        i->Socket(),
                        boost::asio::buffer(data_length_to_all, sizeof(std::size_t)),
                        std::bind(
                            &Session::handle_write_length_all,
                            this,
                            std::placeholders::_1,
                            data_length_to_all,
                            isLast
                        )
                    );
                    boost::asio::async_write(
                        i->Socket(),
                        boost::asio::buffer(data_to_all, bytes_transferred),
                        std::bind(
                            &Session::handle_write_all, // 广播给所有用户
                            this,
                            std::placeholders::_1,
                            data_to_all,
                            isLast
                        )
                    );
                }
            }
        }
        else
        {
            std::cout << "read error" << std::endl;
            if (_disconnect_handler)
            {
                _disconnect_handler(this);
            }
            // delete this; // 销毁掉当前session，断开连接
            //  这种写法有隐患，实际生产不会使用
        }
    }
    catch (boost::exception &e)
    {
        std::cout << "read error" << std::endl;
        if (_disconnect_handler)
        {
            _disconnect_handler(this);
        }
    }
}

void Session::handle_write_length(const boost::system::error_code &error)
{
    if (!error)
    {
        memset(_data_length, 0, sizeof(std::size_t));
    }
    else
    {
        std::cout << "write error" << std::endl;
        if (_disconnect_handler)
        {
            _disconnect_handler(this);
        }
        // delete this; // 销毁掉当前session，断开连接
    }
}

void Session::handle_write_length_all(const boost::system::error_code &error, char *_data_length_to_all, bool isLast)
{
    if (!error)
    {
        if (isLast)
        {
            delete[] _data_length_to_all;
            if (isLast)
                memset(_data_length, 0, sizeof(std::size_t));
        }
    }
    else
    {
        std::cout << "write error" << std::endl;
        if (_disconnect_handler)
        {
            _disconnect_handler(this);
        }
        // delete this; // 销毁掉当前session，断开连接
    }
}

void Session::handle_write_all(const boost::system::error_code &error, char *_data_to_all, bool isLast) // 广播函数
{
    if (!error)
    {
        if (isLast)
        {
            delete[] _data_to_all;
            memset(_data, 0, max_length);
            _socket.async_read_some(
                boost::asio::buffer(_data, max_length),
                std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2)
            );
        }
    }
    else
    {
        std::cout << "write error" << std::endl;
        if (_disconnect_handler)
        {
            _disconnect_handler(this);
        }
        // delete this; // 销毁掉当前session，断开连接
    }
}

void Session::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
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
        if (_disconnect_handler)
        {
            _disconnect_handler(this);
        }
        // delete this; // 销毁掉当前session，断开连接
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
    _acceptor.async_accept(
        new_session->Socket(), // 分配给服务员
        std::bind(
            &Server::handle_accept,
            this,
            new_session,
            std::placeholders::_1
        )
    );
}

void Server::handle_accept(Session *new_session, const boost::system::error_code &error)
{
    if (!error)
    {
        new_session->set_disconnect_handler(
            [this](Session *s) {
                auto it = std::find(clients.begin(), clients.end(), s); // 寻找s
                if (it != clients.end())
                {
                    // 找到了
                    clients.erase(it); // 从vector中删除
                    delete s;
                }
            }
        );
        // 这里我要备注一下,
        //  上面并不是要运行lambda函数内的命令,
        //  而是提前预设好如果要删除session应该执行什么

        clients.push_back(new_session); // push到vector中
        new_session->Start();           // socket开始读咯,开始收发客户端的消息
    }
    else
    {
        delete new_session; // 出现错误
    }

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