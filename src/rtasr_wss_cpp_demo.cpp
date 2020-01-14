/**
 * @Copyright: https://www.xfyun.cn/
 * @Author: iflytek
 * @Data: 2019-12-23
 * 
 * 本demo测试运行时环境为：Ubuntu18.04
 * 本demo测试运行时所安装的第三方库及其版本如下：
 * boost 1.69.0
 * libssl-dev 1.1.1
 * websocketpp 0.8.1
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 注：项目目录已安装websocket 0.8.1；但是运行环境应该还要自行安装boost 0.8.1，libssl-dev 1.1.1
 * 
 * 实时语音转写 API 文档：https://www.xfyun.cn/doc/asr/rtasr/API.html
 * 错误码链接：https://www.xfyun.cn/document/error-code
 */

// 编译运行前，请填写相关参数
// g++ rtasr_wss_cpp_demo.cpp -lboost_system -lpthread -lcrypto -lssl -lopus -lspeex -I ../include/ -L ../lib/
#include "iflytek_wssclient.hpp"
#include "iflytek_codec.hpp"
#include "iflytek_utils.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

/***************************************************
 * 定义部分
 * 
 * 实时语音转写，所涉及参数的定义
 * 用于wss通信类rtasr_client的定义
 ***************************************************
 */
// 接口鉴权参数
struct API_IFNO
{
    string APIKey;
} API{
    APIKey : ""
};

// 公共参数
struct COMMON_INFO
{
    string APPID;
} COMMON{
    APPID : ""
};

// 其他参数
struct OTHER_INFO
{
    string audio_file;
} OTHER{
    audio_file : "../bin/audio/iat_pcm_16k.pcm"
};

// rtasr_client类，继承于iflytek_wssclient
// 用于与服务器进行websocket(wss)通信
class rtasr_client : public iflytek_wssclient
{
public:
    rtasr_client(API_IFNO API, COMMON_INFO COMMON, OTHER_INFO OTHER);

protected:
    // 需要重写如下的iflytek_wssclient的纯虚函数
    string get_url();
    void send_data(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg);

private:
    API_IFNO API;
    COMMON_INFO COMMON;
    OTHER_INFO OTHER;
};

/*********************************************
 * 主函数部分
 * 
 * 定义rtasr_client类对象
 * 运行客户端
 *********************************************
 */
int main(int argc, char *argv[])
{
    time_t start_time = clock();

    rtasr_client client(API, COMMON, OTHER);
    client.run_client();

    time_t end_time = clock();
    fprintf(stdout, "[INFO] Time used: %fs\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    return 0;
}

/*********************************************
 * rtasr_client类实现部分
 * 
 * 实现rtasr_client类的相关函数
 *********************************************
 */
/**
 * @brief 构造函数
 * 对实时语音转写API所涉及参数的初始化
 */
rtasr_client::rtasr_client(API_IFNO API, COMMON_INFO COMMON, OTHER_INFO OTHER)
    : API(API), COMMON(COMMON), OTHER(OTHER)
{
}

/**
 * @brief 获得建立连接的鉴权url
 * @return 鉴权url
 */
string rtasr_client::get_url()
{
    // 生成当前时间戳
    time_t rawtime = time(NULL);
    string ts = to_string(rawtime);

    // 拼接signa的base_string
    string base_string = this->COMMON.APPID + ts;

    // 使用md5算法结合对base_string签名，获得签名后的摘要base_md5
    string base_md5 = get_md5(base_string);

    // 以apiKey为key对base_md5进行hmac_sha1加密
    string base_sha = get_hmac_sha1(base_md5, this->API.APIKey);

    // 使用base64编码对base_sha进行编码获得最终的signa
    string signa = get_base64_encode(base_sha);

    // 对相关参数构成url，并进行url编码，生成最终鉴权url
    char buf[1024];
    sprintf(buf, "appid=%s&ts=%s&signa=%s",
            this->COMMON.APPID.c_str(), ts.c_str(), signa.c_str());
    string url = "wss://rtasr.xfyun.cn/v1/ws?" + get_url_encode(string(buf));

    return url;
}

/**
 * @brief 向服务器发送数据
 * @param hdl 当前连接的句柄
 */
void rtasr_client::send_data(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] Sending audio data to server...\n");

    FILE *fin = fopen(this->OTHER.audio_file.c_str(), "rb");
    if (fin == NULL)
    {
        // 文件打开错误
        fprintf(stderr, "[ERROR] Failed to open the file \"%s\"\n", this->OTHER.audio_file.c_str());
        exit(1);
    }

    // 音频数据帧缓冲区
    int pcm_length = 1280;
    unsigned char *pcm = new unsigned char[pcm_length];

    int cnt = 0;
    while (1)
    {
        int size;
        if (size = fread(pcm, sizeof(char), pcm_length, fin))
        {
            this->wssclient.send(hdl, string((char *)pcm, size), websocketpp::frame::opcode::binary);
            fprintf(stdout, "\r[INFO] No.%d frame sent...", ++cnt);
            delay(0.04); // 模拟音频采样间隔
        }
        else
        {
            // 上传结束标志
            this->wssclient.send(hdl, "{\"end\": true}", websocketpp::frame::opcode::text);
            fprintf(stdout, "\r[SUCCESS] No.%d frame sent，OVER\n", ++cnt);
            break;
        }
        fflush(stdout);
    }

    delete[] pcm;
    fclose(fin);
}

/**
 * @brief websocket收到服务器数据时的回调函数
 * @param hdl 当前连接的句柄
 * @param msg 服务器数据的句柄
 */
void rtasr_client::on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg)
{
    static int cnt = 0;
    fprintf(stdout, "\r[INFO] WebSocket's STATE is ON_MESSAGE, No.%d frame received", ++cnt);
    fflush(stdout);

    // FIXME: 返回的json对象中，'data'值为字符串，会导致json引擎解析失败，待解决
    cout << msg->get_payload() << endl;
}