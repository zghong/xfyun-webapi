/**
 * @Copyright: https://www.xfyun.cn/
 * @Author: iflytek
 * @Data: 2019-12-20
 * 
 * 本文件包含“讯飞开放平台”的WebAPI接口，发送WebSocket(wss)请求的客户端类定义及实现
 * iflytek_wssclient类基于websocketpp 0.8.1开源框架实现，具体查看：https://github.com/zaphoyd/websocketpp
 * 
 * iflytek_wssclient类实现了向“讯飞开放平台”服务器发送基于wss的websocket请求
 * 如果需要实现ws的websocket请求请参考websoketpp帮助文档：https://www.zaphoyd.com/websocketpp
 */

#ifndef _IFLYTEK_WSSCLIENT_HPP
#define _IFLYTEK_WSSCLIENT_HPP

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> asio_tls_client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

/**
 * @brief 进行websocket通信的wss客户端
 * 
 * [public]
 * @func iflytek_wssclient 构造函数
 * @func run_client 运行客户端
 * 
 * [protected]
 * @func on_open websocket处于已连接状态时的回调函数
 * @func on_close websocket处于关闭状态时的回调函数
 * @func on_fail websocket发生错误时的回调函数
 * @func context_ptr tls初始化，用于wss
 * @func get_url [纯虚函数]获得建立连接的鉴权url
 * @func send_data [纯虚函数]向服务器发送数据
 * @func on_message [纯虚函数]websocket收到服务器数据时的回调函数
 * @member wssclient websocketpp的client对象
 */
class iflytek_wssclient
{
public:
    iflytek_wssclient();
    void run_client();

protected:
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);
    static context_ptr on_tls_init();

    // 派生类需要重载如下成员函数
    virtual std::string get_url() = 0;
    virtual void send_data(websocketpp::connection_hdl hdl) = 0;
    virtual void on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg) = 0;

    asio_tls_client wssclient;
};

/**
 * @brief 构造函数
 * 进行相关设置初始化
 * 绑定回调函数
 */
iflytek_wssclient::iflytek_wssclient()
{
    // 开启/关闭相关日志
    // this->wssclient.set_access_channels(websocketpp::log::alevel::all);
    this->wssclient.clear_access_channels(websocketpp::log::alevel::all);
    // this->wssclient.set_error_channels(websocketpp::log::elevel::all);
    this->wssclient.clear_error_channels(websocketpp::log::alevel::all);

    // 初始化Asio
    this->wssclient.init_asio();

    // 绑定事件
    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    this->wssclient.set_open_handler(bind(&iflytek_wssclient::on_open, this, _1));
    this->wssclient.set_close_handler(bind(&iflytek_wssclient::on_close, this, _1));
    this->wssclient.set_fail_handler(bind(&iflytek_wssclient::on_fail, this, _1));
    this->wssclient.set_message_handler(bind(&iflytek_wssclient::on_message, this, _1, _2));
    this->wssclient.set_tls_init_handler(bind(&iflytek_wssclient::on_tls_init)); // tls初始化，用于wss
}

/**
 * @brief 运行客户端
 * 获取鉴权url
 * 与该url建立wss通信
 */
void iflytek_wssclient::run_client()
{
    fprintf(stdout, "[INFO] WebSocket's STATE is ON_CONNECT...\n");
    // 获取鉴权url
    std::string url = this->get_url();
    fprintf(stdout, "[INFO] Authorization_URL is \"%s\"\n", url.c_str());

    // 创建一个新的连接请求
    websocketpp::lib::error_code ec;
    asio_tls_client::connection_ptr con = this->wssclient.get_connection(url, ec);
    if (ec)
    {
        fprintf(stderr, "[ERROR] Failed to connect: \"%s\"\n", ec.message().c_str());
        exit(1);
    }

    // 连接到url
    this->wssclient.connect(con);
    // 运行
    this->wssclient.run();
}

/**
 * @brief websocket处于已连接状态时的回调函数
 * 开启线程，向服务器发送数据
 * @param hdl 当前连接的句柄
 */
void iflytek_wssclient::on_open(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] WebSocket's STATE is ON_OPEN...\n");

    // 开启线程，向服务器发送数据
    websocketpp::lib::thread send_data_thread(&iflytek_wssclient::send_data, this, hdl);
    send_data_thread.detach();
}

/**
 * @brief websocket处于关闭状态时的回调函数
 * @param hdl 当前连接的句柄
 */
void iflytek_wssclient::on_close(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] WebSocket's STATE is ON_CLOSE...\n");

    // asio_tls_client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
    // cout << "[INFO] [websocketpp info] " << con->get_ec() << "-" << con->get_ec().message() << endl;
}

/**
 * @brief websocket发生错误时的回调函数
 * 输出相关错误日志
 * @param hdl 当前连接的句柄
 */
void iflytek_wssclient::on_fail(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] WebSocket's STATE is ON_FAIL...\n");

    asio_tls_client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
    std::cout << "[ERROR] [websocketpp info] " << con->get_ec() << "-" << con->get_ec().message() << std::endl;
    std::cout << "[ERROR] [server info] " << con->get_response_code() << "-" << con->get_response_msg() << std::endl;
    exit(1);
}

/**
 * @brief tls初始化，用于wss
 * @return ssl句柄
 */
context_ptr iflytek_wssclient::on_tls_init()
{
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "[ERROR] Failed to init tls, %s\n", e.what());
        exit(1);
    }
    return ctx;
}

#endif