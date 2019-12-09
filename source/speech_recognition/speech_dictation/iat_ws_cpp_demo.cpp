/**
 * @author iflytek
 * 
 * 本demo测试时运行环境为：Ubuntu18.04
 * 本demo测试成功运行时所安装的第三方库及其版本如下：
 * boost 1.69.0
 * websocketpp 0.8.1
 * openssl 1.1.1
 * 注：测试时，websocketpp 0.8.1与boost 1.69.0不兼容，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 
 * 语音听写流式 WebAPI 接口调用示例 接口文档（必看）：https://doc.xfyun.cn/rest_api/语音听写（流式版）.html
 * webapi 听写服务参考帖子（必看）：http://bbs.xfyun.cn/forum.php?mod=viewthread&tid=38947&extra=
 * 语音听写流式WebAPI 服务，热词使用方式：登陆开放平台https://www.xfyun.cn/后，找到控制台--我的应用---语音听写---服务管理--上传热词
 * 设置热词
 * 注意：热词只能在识别的时候会增加热词的识别权重，需要注意的是增加相应词条的识别率，但并不是绝对的，具体效果以您测试为准。
 * 语音听写流式WebAPI 服务，方言试用方法：登陆开放平台https://www.xfyun.cn/后，找到控制台--我的应用---语音听写（流式）---服务管理--识别语种列表
 * 可添加语种或方言，添加后会显示该方言的参数值
 * 错误码链接：https://www.xfyun.cn/document/error-code （code返回错误码时必看）
 */

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "../../../include/utils.hpp"
#include "../../../include/json.hpp"

using namespace std;
using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_client> client;

// 帧状态
enum status_set
{
    STATUS_FIRST_FRAME,    // 第一帧的标识
    STATUS_CONTINUE_FRAME, // 中间帧标识
    STATUS_LAST_FRAME      // 最后一帧的标识
};

// 以下为WebAPI接口连接所需要的常量数据
// 测试时根据实际填写
struct API_IFNO
{
    string APISecret;
    string APIKey;
};

// 公共参数
struct COMMON_INFO
{
    string APPID;
};

// 业务参数
struct BUSINESS_INFO
{
    string language;
    string domain;
    string accent;
    // 更多个性化参数可在官网查看
};

// 业务数据流参数
struct DATA_INFO
{
    // int status; // status程序运行中指定
    string format;
    string encoding;
    // string audio; // audio程序运行中指定
};

const API_IFNO API{
    APISecret : "",
    APIKey : ""
};

const COMMON_INFO COMMON{
    APPID : ""
};

const BUSINESS_INFO BUSINESS{
    language : "zh_cn",
    domain : "iat",
    accent : "mandarin"
};

const DATA_INFO DATA{
    format : "audio/L16;rate=16000",
    encoding : "raw",
};

/*
* 与服务器进行websocket通信的客户端类
* m_client，websocketpp对象
* AudioFile，音频文件路径
*/
class WSClient
{
public:
    WSClient(string AudioFile) : AudioFile(AudioFile)
    {
        // 开启相关日志
        m_client.set_access_channels(websocketpp::log::alevel::all);
        m_client.clear_access_channels(websocketpp::log::alevel::all);
        m_client.set_error_channels(websocketpp::log::elevel::all);
        m_client.clear_error_channels(websocketpp::log::alevel::all);

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
    void run()
    {
        // 创建一个新的连接请求
        websocketpp::lib::error_code ec;

        // 获取鉴权url
        string url = this->get_url();
        // cout << url << endl;

        client::connection_ptr con = m_client.get_connection(url, ec);
        if (ec)
        {
            cout << "Get Connection Error: " + ec.message() << endl;
            return;
        }

        // 连接到url
        m_client.connect(con);
        // 运行
        m_client.run();
    }

    // 连接事件回调函数
    void on_open(websocketpp::connection_hdl hdl)
    {
        // cout << "on_open" << endl;

        // 开启线程，发送音频数据
        websocketpp::lib::thread send_data_thread(&WSClient::send_data, this, hdl);
        send_data_thread.join();
    }

    // 关闭事件回调函数
    void on_close(websocketpp::connection_hdl hdl)
    {
        cout << "on_close" << endl;

        client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
    }

    // 错误事件回调函数
    void on_fail(websocketpp::connection_hdl hdl)
    {
        cout << "on_fail" << endl;

        client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
    }

    // 收到消息事件回调函数
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
    {
        // cout << "on_message" << endl;

        cout << msg->get_payload() << endl;
    }

    // 发送音频数据
    void send_data(websocketpp::connection_hdl hdl)
    {
        int frameSize = 1280;                           // 每一帧的音频大小
        double intervel = 0.04;                         // 发送音频间隔(单位:s)
        status_set current_status = STATUS_FIRST_FRAME; // 音频的状态信息，标识音频是第一帧，还是中间帧、最后一帧

        FILE *fp = fopen(this->AudioFile.c_str(), "rb");
        if (fp == NULL)
        {
            // 文件打开错误
            cout << "open file: [" + this->AudioFile + "] failed!" << endl;
            return;
        }

        // 音频数据帧缓冲区
        char *buffer = new char[frameSize + 10];
        while (current_status != STATUS_LAST_FRAME)
        {
            int size = fread(buffer, sizeof(char), frameSize, fp);

            // 读到的字节数为0，说明当前是最后一帧
            if (!size)
            {
                current_status = STATUS_LAST_FRAME;
            }

            // 发送相应的数据给服务器
            switch (current_status)
            {
            case STATUS_FIRST_FRAME:
            {
                // 第一帧处理
                json data = {
                    {"common", {{"app_id", COMMON.APPID}}},
                    {"business", {{"language", BUSINESS.language}, {"domain", BUSINESS.domain}, {"accent", BUSINESS.accent}}},
                    {"data", {
                                 {"status", 0},
                                 {"format", DATA.format},
                                 {"encoding", DATA.encoding},
                                 {"audio", get_base64_encode(string(buffer, size))},
                             }}};

                this->m_client.send(hdl, data.dump(), websocketpp::frame::opcode::binary);
                current_status = STATUS_CONTINUE_FRAME;
                break;
            }
            case STATUS_CONTINUE_FRAME:
            {
                // 中间帧处理
                json data = {
                    {"data", {
                                 {"status", 1},
                                 {"format", DATA.format},
                                 {"encoding", DATA.encoding},
                                 {"audio", get_base64_encode(string(buffer, size))},
                             }}};

                this->m_client.send(hdl, data.dump(), websocketpp::frame::opcode::binary);
                break;
            }
            case STATUS_LAST_FRAME:
            {
                // 最后一帧处理
                json data = {
                    {"data", {
                                 {"status", 2},
                                 {"format", DATA.format},
                                 {"encoding", DATA.encoding},
                                 {"audio", get_base64_encode(string(buffer, size))},
                             }}};

                this->m_client.send(hdl, data.dump(), websocketpp::frame::opcode::binary);
                break;
            }
            }

            // 模拟音频采样间隔
            delay(intervel);
        }

        fclose(fp);
    }

    // 生成接口鉴权url
    string get_url()
    {
        // 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 CST"
        string date = get_time_rfc1123();
        // string date = "Thu, 05 Dec 2019 09:54:17 CST";

        // 拼接signature的原始字符串
        string signature_origin = "host: iat-api.xfyun.cn\n";
        signature_origin += "date: " + date + "\n";
        signature_origin += "GET /v2/iat HTTP/1.1";

        // 使用hmac-sha256算法结合apiSecret对signature_origin签名，获得签名后的摘要signature_sha
        string signature_sha = get_hmac_sha256(signature_origin, API.APISecret);

        // 使用base64编码对signature_sha进行编码获得最终的signature
        string signature = get_base64_encode(signature_sha);

        // 拼接authorization的原始字符串
        char buf[1024];
        sprintf(buf, "api_key=\"%s\", algorithm=\"%s\", headers=\"%s\", signature=\"%s\"",
                API.APIKey.c_str(), "hmac-sha256", "host date request-line", signature.c_str());
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
    client m_client;
    string AudioFile;
};

int main(int argc, char *argv[])
{
    string AudioFile = "";

    clock_t start_time = clock();

    WSClient c(AudioFile);
    c.run();

    clock_t end_time = clock();
    cout << "Time used: " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}
