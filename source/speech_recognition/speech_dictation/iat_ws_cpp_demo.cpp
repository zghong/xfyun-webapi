#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "../../../include/utils.hpp"

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;

/*
* 相关参数类
*/
class WSParam
{
public:
    WSParam(string APPID, string APISecret, string APIKey, string AudioFile)
        : APPID(APPID), APISecret(APISecret), APIKey(APIKey), AudioFile(AudioFile)
    {
        // # 公共参数(common)
        // self.CommonArgs = {"app_id": self.APPID}
        // # 业务参数(business)，更多个性化参数可在官网查看
        // self.BusinessArgs = {"domain": "iat", "language": "zh_cn", "accent": "mandarin", "vinfo":1,"vad_eos":10000}
    }

    // 生成接口鉴权url
    string create_url()
    {
        // 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 CST"
        string date = get_time_rfc1123();
        // string date = "Thu, 05 Dec 2019 09:54:17 CST";

        // 拼接signature的原始字符串
        string signature_origin = "host: iat-api.xfyun.cn\n";
        signature_origin += "date: " + date + "\n";
        signature_origin += "GET /v2/iat HTTP/1.1";

        // 使用hmac-sha256算法结合apiSecret对signature_origin签名，获得签名后的摘要signature_sha
        string signature_sha = get_hmac_sha256(signature_origin, APISecret);

        // 使用base64编码对signature_sha进行编码获得最终的signature
        string signature = get_base64_encode(signature_sha);

        // 拼接authorization的原始字符串
        char buf[1024];
        sprintf(buf, "api_key=\"%s\", algorithm=\"%s\", headers=\"%s\", signature=\"%s\"",
                this->APIKey.c_str(), "hmac-sha256", "host date request-line", signature.c_str());
        string authorization_origin = string(buf);

        // 再对authorization_origin进行base64编码获得最终的authorization参数
        string authorization = get_base64_encode(authorization_origin);

        // 对相关参数构成url，并进行url编码，生成最终鉴权url
        sprintf(buf, "authorization=%s&date=%s&host=%s",
                authorization.c_str(), date.c_str(), "iat-api.xfyun.cn");
        string url = "ws://iat-api.xfyun.cn/v2/iat?" + get_url_encode(string(buf));

        return url;
    }

private:
    string APPID;
    string APISecret;
    string APIKey;
    string AudioFile;
};

/*
* 与服务器进行websocket通信的客户端类
*/
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
    /*
    * 测试websocket通信
    */
    // WSClient c;

    // string uri = "ws://localhost:9002";
    // if (argc == 2)
    // {
    //     uri = argv[1];
    // }

    // c.run(uri);

    /*
    * 测试获取鉴权url
    */
    string APPID = "5dde2318";
    string APISecret = "4d3dcdf6184d4aa4d987cecca7de3896";
    string APIKey = "5d5fe0d6177df2afc8a03909b71259eb";
    string AudioFile = "../../../resource/iat_pcm_16k.pcm";
    WSParam wsParam(APPID, APISecret, APIKey, AudioFile);
    string wsUrl = wsParam.create_url();

    cout << wsUrl << endl;

    return 0;
}
