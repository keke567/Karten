#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <ctime>
#include <chrono>
#include <memory>
#include <functional>
#include <queue>
#include <sstream>
#include <atomic>
#define ASIO_STANDALONE
#define True true
#define False false
#include <asio.hpp>
using asio::ip::tcp;
using namespace std;
//=============================================================
// 基础数据结构
//=============================================================
// 牌类
class Card
{
public:
    int suit;   // 花色   0:黑桃, 1:红心, 2:梅花, 3:方块
    int value;  // 点数
    int id;     // 唯一标识
    int weight; // 权重  用于比大小
    Card(int card_id = 0) : id(card_id)
    {
        if (id < 0 || id > 53)
        {
            cout << "错误！" << endl;
            return;
        }
        if (id == 52)
        {
            // 小王
            suit = 0; // 花色只有黑桃
            value = 16;
            weight = 16;
        }
        else if (id == 53)
        {
            // 大王
            suit = 0;
            value = 17;
            weight = 17;
        }
        else
        {
            // 普通牌
            suit = id / 13;
            value = 3 + (id % 13);
            weight = value;
        }
    }
    string getName() const
    {
        // 花色和名字
        map<int, string> suitNames = {
            {0, "??"}, {1, "??"}, {2, "??"}, {3, "?"}};
        // 点数名字
        map<int, string> valueNames = {
            {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"}, {8, "8"}, {9, "9"}, {10, "10"}, {11, "J"}, {12, "Q"}, {13, "K"}, {14, "A"}, {15, "2"}, {16, "小王"}, {17, "大王"}};
        if (value >= 16)
        {
            return valueNames.at(value);
        }
        // 其他：点数+花色
        string suit_str = (suitNames.count(suit) ? suitNames.at(suit) : "?");
        string value_str = (valueNames.count(value) ? valueNames.at(value) : "?");
        return value_str + suit_str;
    }
    // 打印牌型
    void print() const
    {
        cout << getName();
    }
    // 比较牌的大小
    bool operator<(const Card &other) const
    {
        return weight < other.weight;
    }
    // 判断牌型是否相同
    bool operator==(const Card &other) const
    {
        return id == other.id;
    }
};

enum CardType
{
    INVALID_TYPE,  // 无效牌型invalid
    SINGLE,        // 单张single
    PAIR,          // 对子pair
    TREE,          // 三张tree
    TREE_WITH_ONE, // 三带一
    TREE_WITH_TWO, // 三带二
    STRAIGHT,      // 顺子 (5张或以上连续单牌)
    BOMB,          // 炸弹 (四张相同)
    ROCKET,        // 王炸 (大王+小王)
    AIRPLANE,
    AIRPLANE_WITH_SINGLE,
    AIRPLANE_WITH_PAIR
};
/*
 * 1. 判断牌型是否合法
 * 2. 判断能否压过上家的牌
 * 3. 计算牌型分数
 * 4. 其他游戏规则
 */
class GameLogic
{
public:
    // 检查牌型
    static CardType checkCardType(const vector<Card> &cards)
    {
        if (cards.empty())
            return INVALID_TYPE;
        int n = (int)cards.size();
        vector<Card> sorted = cards;
        sort(sorted.begin(), sorted.end());

        // 统计点数出现次数
        map<int, int> cnt;
        for (const auto &c : sorted)
            cnt[c.value]++;

        if (n == 1)
            return SINGLE;

        if (n == 2)
        {
            // 王炸
            if ((sorted[0].value == 16 && sorted[1].value == 17) ||
                (sorted[0].value == 17 && sorted[1].value == 16))
                return ROCKET;
            if (sorted[0].value == sorted[1].value)
                return PAIR;
            return INVALID_TYPE;
        }

        if (n == 3)
        {
            if (cnt.size() == 1)
                return TREE;
            return INVALID_TYPE;
        }

        if (n == 4)
        {
            // 炸弹：四张相同
            for (auto &p : cnt)
                if (p.second == 4)
                    return BOMB;
            // 三带一
            for (auto &p : cnt)
                if (p.second == 3)
                    return TREE_WITH_ONE;
            return INVALID_TYPE;
        }

        if (n == 5)
        {
            // 三带二
            bool has3 = false, has2 = false;
            for (auto &p : cnt)
            {
                if (p.second == 3)
                    has3 = true;
                if (p.second == 2)
                    has2 = true;
            }
            if (has3 && has2)
                return TREE_WITH_TWO;

            // 顺子（5张连续，不包含2或王）
            if (cnt.size() == 5)
            {
                if (sorted.back().value >= 15)
                    return INVALID_TYPE; // 不能包含2或王
                bool ok = true;
                for (int i = 1; i < n; i++)
                {
                    if (sorted[i].value != sorted[i - 1].value + 1)
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                    return STRAIGHT;
            }
            return INVALID_TYPE;
        }

        // n >= 6：目前只支持顺子（至少5张连续单牌）
        if (n >= 5)
        {
            if (cnt.size() == n)
            {
                if (sorted.back().value >= 15)
                    return INVALID_TYPE;
                for (int i = 1; i < n; i++)
                {
                    if (sorted[i].value != sorted[i - 1].value + 1)
                        return INVALID_TYPE;
                }
                return STRAIGHT;
            }
            if (n == 6 && sorted[0].value == sorted[1].value && sorted[2].value == sorted[3].value && sorted[4].value == sorted[6].value)
            {
                return AIRPLANE;
            }
            if ((n == 7 && sorted[0].value == sorted[1].value && sorted[2].value == sorted[3].value && sorted[4].value == sorted[5].value) || (sorted[1].value == sorted[2].value && sorted[3].value == sorted[4].value && sorted[5].value == sorted[6].value))
            {
                return AIRPLANE_WITH_SINGLE;
            }
            if (n == 8 && sorted[0].value == sorted[1].value && sorted[2].value == sorted[3].value && sorted[4].value == sorted[5].value && sorted[6].value == sorted[7].value)
            {
                return AIRPLANE_WITH_SINGLE;
            }
            return INVALID_TYPE;
        }
        return INVALID_TYPE;
    }
    // 转化成字符串
    static string getTypeName(CardType type)
    {
        map<CardType, string> typeNames = {
            {INVALID_TYPE, "无效牌型"},
            {SINGLE, "单张"},
            {PAIR, "对子"},
            {TREE, "三张"},
            {TREE_WITH_ONE, "三带一"},
            {TREE_WITH_TWO, "三带二"},
            {STRAIGHT, "顺子"},
            {BOMB, "炸弹"},
            {ROCKET, "王炸"},
            {AIRPLANE, "飞机"},
            {AIRPLANE_WITH_SINGLE, "飞机带单"},
            {AIRPLANE_WITH_PAIR, "飞机带双"}};
        return (typeNames.count(type) ? typeNames.at(type) : "未知牌型");
    }

    /*
     *检查能否出牌
     *检查上家出的牌
     *检查能否出牌
     */

    static bool canPlayCards(const vector<Card> &lastCards,
                             const vector<Card> &currentCards)
    {
        // 检查当前牌是否合法
        CardType currentType = checkCardType(currentCards);
        if (currentType == INVALID_TYPE)
        {
            return false;
        }
        if (lastCards.empty())
        {
            return true; // 第一次出能出任何合法的牌
        }
        // 检查上家牌牌型
        CardType lastType = checkCardType(lastCards);
        if (lastType == INVALID_TYPE)
        {
            return false;
        }
        if (currentType == ROCKET)
        {
            return true;
        }
        if (currentType == BOMB)
        {
            if (lastType != BOMB && lastType != ROCKET)
            {
                return true;
            }
        }
        // 相同牌型
        if (currentType == lastType)
        {
            if (currentType == STRAIGHT &&
                currentCards.size() != lastCards.size())
            {
                return false;
            }
            // 比较主牌
            int currentMain = getMainValue(currentCards, currentType);
            int lastMain = getMainValue(lastCards, lastType);
            return (currentMain > lastMain) ? true : false;
        }
        return false;
    }
    // 取主值
    static int getMainValue(const vector<Card> &cards, CardType type)
    {
        if (cards.empty())
            return 0;
        vector<Card> sorted = cards;
        sort(sorted.begin(), sorted.end());
        switch (type)
        {
        case SINGLE:
        case PAIR:
        case TREE:
        case BOMB:
            return sorted.back().weight;
        case TREE_WITH_ONE:
        case TREE_WITH_TWO:
        {
            map<int, int> mp;
            for (const auto &it : sorted)
            {
                mp[it.value]++;
            }
            for (const auto &it : mp)
            {
                if (it.second == 3)
                    return it.first;
            }
            return 0;
        }
        case STRAIGHT:
            return sorted.back().weight;
        case ROCKET:
            return 100;
        case AIRPLANE:
            return sorted[5].weight;
        case AIRPLANE_WITH_SINGLE:
            if (sorted[0].value == sorted[1].value)
                return sorted.back().weight;
        case AIRPLANE_WITH_PAIR:
            return sorted.back().weight;
        default:
            return 0;
        }
    }

    static void printCards(const vector<Card> &cards, const string &title = "")
    {
        if (!title.empty())
        {
            cout << title << ": ";
        }

        for (size_t i = 0; i < cards.size(); i++)
        {
            cards[i].print();
            if (i < cards.size() - 1)
            {
                cout << " ";
            }
        }

        if (!cards.empty())
        {
            CardType type = checkCardType(cards);
            cout << " 【" << getTypeName(type) << "】";
        }
        cout << endl;
    }
    static void sortCards(vector<Card> &cards)
    {
        sort(cards.begin(), cards.end());
    }
};
//====================================================================
// 第三部分：游戏工具
//====================================================================
/*
发牌
*/
class GameUtils
{
public:
    static vector<int> createDeck()
    {
        vector<int> deck;
        for (int i = 0; i < 54; i++)
        {
            deck.push_back(i);
        }
        return deck;
    }
    // 洗牌
    static void shuffledeck(vector<int> &deck)
    {
        static std::mt19937 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
        std::shuffle(deck.begin(), deck.end(), rng);
    }
    static vector<vector<Card>> dealCards()
    {
        vector<int> deck = createDeck();
        shuffledeck(deck);
        vector<vector<Card>> result(4); // 三个玩家和一个底牌
        // 三个玩家
        for (int i = 0; i < 51; i++)
        {
            int p = i % 3;
            result[p].push_back(Card(deck[i]));
        }
        // 底牌
        for (int i = 51; i < 54; i++)
        {
            result[3].push_back(Card(deck[i]));
        }
        // 帮玩家排序
        for (int i = 0; i < 4; i++)
        {
            GameLogic::sortCards(result[i]);
        }
        return result;
    }
};
//============================================================
// 服务器实现
//============================================================

bool START = false;

/**
 * @class Session
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
    bool is_writing_ = False;
    void Send(string &tag);
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
void Session::Send(string &tag)
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
            is_writing_ = False;
        }
    }
    else
    {
        handle_write_error(error);

        write_queue_.clear();
        is_writing_ = False;
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
/**
 * @class Server
 * @brief Session管理器，实现对不同连接的简单管理
 * @details
 * 
 * 
 * @author MyslZhao
 */
class Server
{
    public:
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

// ====================================================
// 第四部分：主函数
// ====================================================

void showWelcome()
{
    cout << "=========================================" << endl;
    cout << "      斗地主游戏逻辑与服务器系统        " << endl;
    cout << "=========================================" << endl;
    cout << endl;
    cout << "系统包含：" << endl;
    cout << "1. 游戏逻辑模块 - 处理出牌规则、牌型判断" << endl;
    cout << "2. 网络服务器模块 - 基于Asio的TCP服务器" << endl;
    cout << endl;
    cout << "编译命令：g++ -std=c++11 -pthread main.cpp -o doudizhu" << endl;
    cout << "=========================================" << endl;
}

void showMenu()
{
    cout << "\n============ 斗地主系统主菜单 ============" << endl;
    cout << "1. 启动游戏服务器（端口8888）" << endl;
    cout << "2. 启动游戏服务器（自定义端口）" << endl;
    cout << "3. 停止游戏服务器" << endl;
    cout << "4. 查看服务器状态" << endl;
    cout << "5. 测试游戏逻辑" << endl;
    cout << "0. 退出程序" << endl;
    cout << "=========================================" << endl;
    cout << "请输入选项 (0-5): ";
}

void testGameLogic()
{
    cout << "\n=== 游戏逻辑测试 ===" << endl;

    // 测试一些牌型
    vector<Card> single = {Card(0)};
    vector<Card> pair = {Card(0), Card(13)};
    vector<Card> bomb = {Card(0), Card(13), Card(26), Card(39)};

    cout << "测试牌型识别:" << endl;
    GameLogic::printCards(single, "单张");
    GameLogic::printCards(pair, "对子");
    GameLogic::printCards(bomb, "炸弹");

    // 测试发牌
    cout << "\n测试发牌功能:" << endl;
    auto cards = GameUtils::dealCards();
    for (int i = 0; i < 3; i++)
    {
        cout << "玩家" << (i + 1) << " (" << cards[i].size() << "张): ";
        for (const auto &card : cards[i])
        {
            card.print();
            cout << " ";
        }
        cout << endl;
    }

    cout << "\n游戏逻辑测试完成！" << endl;
}

// 回huan地址127.0.0.1  8080  utf8编码
int main()
{
    return 0;
};
