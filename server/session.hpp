#include <asio.hpp>
#include <queue>
using asio::ip::tcp;
using namespace std;

/**
 * @class S/ession
 * @brief 异步服务器任务类
 * @details
 * 将连接封装成只有`Recv`与`Send`方法的对象，
 * 方便后续随调随用
 * @author MyslZhao
 */
class Session : public enable_shared_from_this<Session>
{
public:
    // 接受数据回调函数组
    using DataCallback = std::function<void(const std::string &, std::shared_ptr<Session>)>;
    using ErrorCallback = std::function<void(const asio::error_code &)>;
    void set_on_data(DataCallback callback);
    void set_on_error(ErrorCallback callback);

    Session(tcp::socket socket) : socket_(move(socket)) { do_read(); };
    std::deque<std::string> write_queue_;
    bool is_writing_ = false;
    void Send(const string &tag);
    void Close(void);

private:
    DataCallback on_data_callback_;
    ErrorCallback on_error_callback_;
    tcp::socket socket_;
    asio::streambuf buffer_;
    void do_read();
    void on_read(const asio::error_code &error, size_t length);
    void do_write();
    void on_write(const asio::error_code &error, size_t length);
    void handle_write_error(const asio::error_code &error);
};