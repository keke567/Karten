#include "session.hpp"

/**
 * @class Server
 * @brief Session管理器，实现对不同连接的简单管理
 * @details
 * 
 * @author MyslZhao
 */
class Server
{
    public:
    vector<string> MSG;
    // 新连接回调函数
    using NewConnectionCallback = std::function<void(std::shared_ptr<Session>)>;
    // 关闭连接回调函数
    using ConnectionClosedCallback = std::function<void(std::shared_ptr<Session>)>;

    Server(asio::io_context& io_context, short port);

    void set_on_new_connection(NewConnectionCallback callback);
    void set_on_connection_closed(ConnectionClosedCallback callback);

    size_t connection_count() const;

    void close_all();

    // 给所有连接发消息
    void broadcast(const std::string& message);

    // 向指定连接发消息
    bool send_to(size_t index, const std::string& messgae);

    private:
    void do_accept();
    void remove_connection(std::shared_ptr<Session> session);

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<Session>> connections_;

    NewConnectionCallback on_new_connection_;
    ConnectionClosedCallback on_connection_closed_;

    static constexpr size_t MAX_CONNECTIONS = 3;
};

