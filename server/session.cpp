#include "session.hpp"
#include <iostream>
using asio::ip::tcp;
using namespace std;

/**
 * @brief 设置接受回调函数
 * @details
 * callback函数格式：
 *
 * `void DataCallback(const std::string&, std::shared_ptr<Session>)`
 * @param callback 用来处理数据的回调函数
 */
void Session::set_on_data(DataCallback callback)
{
    on_data_callback_ = std::move(callback);
};

/**
 * @brief 设置发生错误时的回调函数
 * @details
 * callback函数格式：
 *
 * `void EeeoeCallback(const asio::error_code&)`
 */
void Session::set_on_error(ErrorCallback callback)
{
    on_error_callback_ = std::move(callback);
};

/**
 * @brief 异步发送数据方法入口
 * @details
 * 将数据加入发送队列
 *
 * @param tag 要发送的数据
 */
void Session::Send(const string &tag)
{
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(tag + "\n");

    if (!write_in_progress)
    {
        do_write();
    }
};

/**
 * @brief 异步接受数据（TCP阶段）
 * @details
 * 声明为`private`
 *
 * 由`Session::Recv`调用
 *
 * @param tag 数据接受载体
 */
void Session::do_read()
{
    async_read_until(socket_, buffer_, '\n',
                     [self = weak_from_this()](const asio::error_code &error, size_t length)
                     {
                         auto shared_self = self.lock();
                         if (shared_self)
                         {
                             shared_self->on_read(error, length);
                         }
                     });
};

/**
 * @brief 执行异步写操作
 * @details
 * 更改`is_writing_`的状态，并调用`on_write`执行下一步操作。
 */
void Session::do_write()
{
    if (write_queue_.empty())
    {
        is_writing_ = false;
        return;
    }

    is_writing_ = true;
    const std::string &data = write_queue_.front();

    async_write(socket_, asio::buffer(data),
                [self = weak_from_this()](const asio::error_code &error, size_t length)
                {
                    if (auto conn = self.lock())
                    {
                        conn->on_write(error, length);
                    }
                });
};

/**
 * @brief 从缓冲区读取到容器
 * @details
 * 声明为`private`
 *
 * 由`Session::do_read`调用
 *
 * @param error 捕获的错误（如果发生错误）
 * @param length 数据长度
 */
void Session::on_read(const asio::error_code &error, size_t length)
{
    if (!error)
    {
        istream is(&buffer_);
        string tag;
        getline(is, tag);

        if (on_data_callback_)
        {
            on_data_callback_(tag, shared_from_this());
        }

        do_read();
    }
    else
    {
        if (on_error_callback_)
        {
            on_error_callback_(error);
        }
    }
};

/**
 * @brief 写入完成回调
 * @details
 * + 将已发送的数据移除列表
 * + 发生错误时调用`handle_write_error`处理错误
 * + 更改`is_writing_`的状态
 */
void Session::on_write(const asio::error_code &error, size_t length)
{
    if (!error)
    {
        write_queue_.pop_front();

        if (!write_queue_.empty())
        {
            do_write();
        }
        else
        {
            is_writing_ = false;
        }
    }
    else
    {
        handle_write_error(error);

        write_queue_.clear();
        is_writing_ = false;
    }
};

/**
 * @brief 写入错误回调函数
 *
 * @param error 捕获到的错误对象
 */
void Session::handle_write_error(const asio::error_code &error)
{
    std::cout << "error at writing:" << error.message() << endl;
    if (socket_.is_open())
    {
        Close();
    }
}

/**
 * @brief 关闭连接
 * 
 */
void Session::Close(){
    if(socket_.is_open()){
        socket_.close();
    }
}