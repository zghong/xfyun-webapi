#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;

class WSClient
{
public:
    WSClient()
    {
        // 开启相关日志
        m_client.set_access_channels(websocketpp::log::alevel::all);
        m_client.clear_access_channels(websocketpp::log::alevel::all);
        m_client.set_error_channels(websocketpp::log::elevel::all);

        // 初始化Asio
        m_client.init_asio();

        // 绑定事件
        using websocketpp::lib::bind;
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        m_client.set_open_handler(bind(&WSClient::on_open, this, _1));
        m_client.set_close_handler(bind(&WSClient::on_close, this, _1));
        m_client.set_fail_handler(bind(&WSClient::on_fail, this, _1));
        m_client.set_message_handler(bind(&WSClient::on_message, this, _1, _2));
    }

    // 运行客户端
    void run(const string &uri)
    {
        // 创建一个新的连接请求
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection(uri, ec);
        if (ec)
        {
            m_client.get_alog().write(websocketpp::log::alevel::app,
                                      "Get Connection Error: " + ec.message());
            return;
        }

        // 连接到uri
        m_client.connect(con);
        // 运行
        m_client.run();
    }

    // 连接事件回调函数
    void on_open(websocketpp::connection_hdl hdl)
    {
        cout << "on_open" << endl;
        this->m_client.send(hdl, "haha", websocketpp::frame::opcode::text);
    }

    // 关闭事件回调函数
    void on_close(websocketpp::connection_hdl hdl)
    {
        cout << "on_close" << endl;
    }

    // 错误事件回调函数
    void on_fail(websocketpp::connection_hdl hdl)
    {
        cout << "on_fail" << endl;
    }

    // 收到消息事件回调函数
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
    {
        cout << "on_message" << endl;
        cout << msg->get_payload() << endl;
    }

private:
    client m_client;
};

int main(int argc, char *argv[])
{
    WSClient c;

    string uri = "ws://localhost:9002";
    if (argc == 2)
    {
        uri = argv[1];
    }

    c.run(uri);

}
