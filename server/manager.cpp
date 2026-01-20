#include "manager.hpp"
#include <format>
#include <iostream>
using namespace std;

/**
 * @brief 用于生成消息日志
 * @details
 * 使用chrono库实现的简单的消息日志接口
 * @param msg 要输出的日志内容
 */
void logger(const std::string &msg)
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&t);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", now_tm);

    std::cout << buffer << " " << msg << std::endl;
}

Server::Server(asio::io_context &io_context, short port)
    : io_context_(io_context),
      acceptor_(io_context_, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port))
{
    logger(std::format("[log] Server starts on port {}, with max connectons {}", port, MAX_CONNECTIONS));
    do_accept();
}

void Server::set_on_new_connection(NewConnectionCallback callback)
{
    on_new_connection_ = std::move(callback);
}

void Server::set_on_connection_closed(ConnectionClosedCallback callback)
{
    on_connection_closed_ = std::move(callback);
}

size_t Server::connection_count() const
{
    return connections_.size();
}

void Server::close_all()
{
    for (auto &conn : connections_)
    {
        if (conn)
        {
            conn->Close();
        }
    }
    connections_.clear();
}

void Server::broadcast(const std::string &message)
{
    for (auto &conn : connections_)
    {
        if (conn)
        {
            conn->Send(message);
        }
    }
}

bool Server::send_to(size_t index, const std::string &message)
{
    if (index < connections_.size() && connections_[index])
    {
        connections_[index]->Send(message);
        return true;
    }
    return false;
}

void Server::do_accept()
{
    acceptor_.async_accept(
        [this](const asio::error_code &error, tcp::socket socket)
        {
            if (!error)
            {
                if (connections_.size() >= MAX_CONNECTIONS)
                {
                    logger(std::format("[warn] Connections reaches maxium, connections deny."));

                    socket.close();
                    do_accept();
                    return;
                }

                auto session = std::make_shared<Session>(std::move(socket));

                session->set_on_data([this, session](const std::string &data, std::shared_ptr<Session> self)
                                     {
                    // 接受信息回调
                    MSG.push_back(std::format("[{}]:{}", connections_.size() - 1, data));
                    logger(std::format("[log] Client[{}]:{}", connections_.size() - 1, data)); });

                session->set_on_error([this, session](const asio::error_code &ec)
                                      {
                    // 发生错误回调
                    logger(std::format("[error] Connections error:{}", ec.message()));
                    remove_connection(session); });

                connections_.push_back(session);

                logger(std::format("[log] New connections({}/{}).", connections_.size(), MAX_CONNECTIONS));

                if (on_new_connection_)
                {
                    on_new_connection_(session);
                }
            }
            else
            {
                logger(std::format("[error] Connection error while establishing:{}", error.message()));
            }
            do_accept();
        });
}

void Server::remove_connection(std::shared_ptr<Session> session)
{
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(), [session](const std::shared_ptr<Session>& conn)
        {
            return conn == session;
        }),
        connections_.end()
    );

    logger(std::format("[log] One client disconnect, current:({}/{})", connections_.size(), MAX_CONNECTIONS));

    if (on_connection_closed_)
    {
        on_connection_closed_(session);
    }
}
