/**
 * @Copyright: https://www.xfyun.cn/
 * @Author: iflytek
 * @Data: 2019-12-20
 * 
 * 本demo测试运行时环境为：Ubuntu18.04
 * 本demo测试运行时所依赖的第三方库及其版本如下：
 * boost 1.69.0
 * libssl-dev 1.1.1
 * websocketpp 0.8.1
 * opus 1.3.1（本Demo编码方案采取的opus）
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 注：项目目录已安装websocket 0.8.1，opus 1.3.1；但是运行环境应该还要自行安装boost 0.8.1，libssl-dev 1.1.1
 * 
 * 语音听写（流式版）WebAPI 文档：https://www.xfyun.cn/doc/asr/voicedictation/API.html
 * 错误码链接：https://www.xfyun.cn/document/error-code
 * 
 * 注：本Demo发送的数据是对opus数据进行ogg封装后的数据，注意和"iat_wss_cpp_demo.cpp"进行区分
 */

// 编译运行前，请填写相关参数
// g++ iat_wss_cpp_ogg_demo.cpp -lboost_system -lpthread -lcrypto -lssl -lopus -I ../include/ -L ../lib/
#include "iflytek_wssclient.hpp"
#include "iflytek_ogg_opus.hpp"
#include "iflytek_utils.hpp"
#include "json.hpp"

#include "opus/opus.h"

using namespace std;
using json = nlohmann::json;

/***************************************************
 * 定义部分
 * 
 * 语音听写，所涉及参数的定义
 * 用于wss通信类iat_client的定义
 ***************************************************
 */
// 接口鉴权参数
struct API_IFNO
{
    string APISecret;
    string APIKey;
} API{
    APISecret : "",
    APIKey : ""
};

// 公共参数
struct COMMON_INFO
{
    string APPID;
} COMMON{
    APPID : ""
};

// 业务参数
struct BUSINESS_INFO
{
    string language;
    string domain;
    string accent;
    // 更多个性化参数可在官网查看
} BUSINESS{
    language : "zh_cn",
    domain : "iat",
    accent : "mandarin"
};

// 业务数据流参数
struct DATA_INFO
{
    // int status; // status程序运行中指定
    string format;
    string encoding;
    // string audio; // audio程序运行中指定
} DATA{
    format : "audio/L16;rate=16000",
    encoding : "opus-ogg" // opus-ogg或者opus-ogg;-1均可
};

// 其他参数
struct OTHER_INFO
{
    string audio_file;
} OTHER{
    audio_file : "../bin/audio/iat_pcm_16k.pcm"
};

// iat_client类，继承于iflytek_wssclient
// 用于与服务器进行websocket(wss)通信
class iat_client : public iflytek_wssclient
{
public:
    iat_client(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER);

protected:
    // 需要重写如下的iflytek_wssclient的纯虚函数
    string get_url();
    void send_data(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg);

private:
    API_IFNO API;
    COMMON_INFO COMMON;
    BUSINESS_INFO BUSINESS;
    DATA_INFO DATA;
    OTHER_INFO OTHER;
};

/***************************************************
 * 主函数部分
 * 
 * 定义iat_client类对象
 * 运行客户端
 ***************************************************
 */
int main(int argc, char *argv[])
{
    time_t start_time = clock();

    iat_client client(API, COMMON, BUSINESS, DATA, OTHER);
    client.run_client();

    time_t end_time = clock();
    fprintf(stdout, "[INFO] Time used: %fs\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    return 0;
}

/***************************************************
 * iat_client类实现部分
 * 
 * 实现iat_client类的相关函数
 *********************************************
 */
/**
 * @brief 构造函数
 * 对语音听写API所涉及参数的初始化
 */
iat_client::iat_client(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER)
    : API(API), COMMON(COMMON), BUSINESS(BUSINESS), DATA(DATA), OTHER(OTHER)
{
}

/**
 * @brief 获得建立连接的鉴权url
 * @return 鉴权url
 */
string iat_client::get_url()
{
    // 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 GMT"
    time_t rawtime = time(NULL);
    char buf[1024];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %X %Z", gmtime(&rawtime));
    string date = string(buf);

    // 拼接signature的原始字符串
    string signature_origin = "host: iat-api.xfyun.cn\n";
    signature_origin += "date: " + date + "\n";
    signature_origin += "GET /v2/iat HTTP/1.1";

    // 使用hmac-sha256算法结合apiSecret对signature_origin签名，获得签名后的摘要signature_sha
    string signature_sha = get_hmac_sha256(signature_origin, API.APISecret);

    // 使用base64编码对signature_sha进行编码获得最终的signature
    string signature = get_base64_encode(signature_sha);

    // 拼接authorization的原始字符串
    sprintf(buf, "api_key=\"%s\", algorithm=\"%s\", headers=\"%s\", signature=\"%s\"",
            API.APIKey.c_str(), "hmac-sha256", "host date request-line", signature.c_str());
    string authorization_origin = string(buf);

    // 再对authorization_origin进行base64编码获得最终的authorization参数
    string authorization = get_base64_encode(authorization_origin);

    // 对相关参数构成url，并进行url编码，生成最终鉴权url
    sprintf(buf, "authorization=%s&date=%s&host=%s",
            authorization.c_str(), date.c_str(), "iat-api.xfyun.cn");
    string url = "wss://iat-api.xfyun.cn/v2/iat?" + get_url_encode(string(buf));

    return url;
}

/**
 * @brief 向服务器发送数据
 * @param hdl 当前连接的句柄
 */
void iat_client::send_data(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] Sending audio data to server...\n");

    FILE *fin = fopen(this->OTHER.audio_file.c_str(), "rb");
    if (fin == NULL)
    {
        // 文件打开错误
        fprintf(stderr, "[ERROR] Failed to open the file \"%s\"\n", this->OTHER.audio_file.c_str());
        exit(1);
    }

    char temp[27 + 255 + 255 * 255];
    int sample_rate = 16000;
    int channel = 1;
    // 经测试，目前讯飞云引擎opus编解码只支持帧时长为20ms的数据
    // 通过表达式可以看出，对于位深16，单声道的音频来说，source_length = frame_size * 2
    int frame_size = sample_rate * 0.02;
    int pcm_length = sample_rate / 8 * 16 * channel * 0.02;
    json data;

    ogg_logic_stream os;
    ogg_page op;
    init_ogg_logic_stream(os);
    // init_ogg_page(op);

    /* send page of opushead */
    init_ogg_page(op);
    opus_id_header id_header{1, (__uint8_t)channel, 312, (__uint32_t)sample_rate, 0, 0};
    ogg_page_put_id_header(op, id_header);
    ogg_page_encapsulate(os, op);
    os.page_flag = 0x00;
    memcpy(temp, op.header, op.header_length);
    memcpy(temp + op.header_length, op.body, op.body_length);
    // 第一帧处理
    data = {
        {"common", {{"app_id", this->COMMON.APPID}}},
        {"business", {{"language", this->BUSINESS.language}, {"domain", this->BUSINESS.domain}, {"accent", this->BUSINESS.accent}}},
        {"data", {
                     {"status", 0},
                     {"format", this->DATA.format},
                     {"encoding", this->DATA.encoding},
                     {"audio", get_base64_encode(string(temp, op.header_length + op.body_length))},
                 }}};
    this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);

    /* send page of opustags */
    init_ogg_page(op);
    char *encoder_info = (char *)"libopus 1.3.1";
    char *comments[] = {
        (char *)"ARTIST=zghong",
        (char *)"TITLE=iflytek_ogg"};
    opus_comment_header comment_header{(__uint32_t)strlen(encoder_info), encoder_info, 2, comments};
    ogg_page_put_comment_header(op, comment_header);
    ogg_page_encapsulate(os, op);
    memcpy(temp, op.header, op.header_length);
    memcpy(temp + op.header_length, op.body, op.body_length);
    // 中间帧处理
    data = {
        {"data", {
                     {"status", 1},
                     {"format", this->DATA.format},
                     {"encoding", this->DATA.encoding},
                     {"audio", get_base64_encode(string(temp, op.header_length + op.body_length))},
                 }}};
    this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);

    /* send page of data */
    init_ogg_page(op);
    // 创建opus编码器
    int err;
    OpusEncoder *enc = opus_encoder_create(sample_rate, channel, OPUS_APPLICATION_VOIP, &err);
    if (OPUS_OK != err)
    {
        fprintf(stderr, "[ERROR] Failed to create OPUS Encoder\n");
        fclose(fin);
        exit(1);
    }
    // 音频数据帧缓冲区
    unsigned char *pcm = new unsigned char[pcm_length];
    unsigned char *opus = new unsigned char[pcm_length];

    int size;
    while (!feof(fin))
    {
        delay(0.02); // 模拟音频采样间隔
        if ((size = fread(pcm, sizeof(char), pcm_length, fin)) == 0)
        {
            break;
        }
        // 音频编解码
        opus_int32 nbytes = opus_encode(enc, (opus_int16 *)pcm, frame_size, opus, pcm_length);
        if (nbytes < 0)
        {
            fprintf(stderr, "[ERROR] Failed to opus_encode raw data\n");
            opus_encoder_destroy(enc);
            delete[] pcm;
            delete[] opus;
            fclose(fin);
            exit(1);
        }

        if (ogg_page_put_packet(os, op, (char *)opus, nbytes) == 0)
        {
            continue;
        }
        // send the page to server
        ogg_page_encapsulate(os, op);
        memcpy(temp, op.header, op.header_length);
        memcpy(temp + op.header_length, op.body, op.body_length);
        // 中间帧处理
        data = {
            {"data", {
                         {"status", 1},
                         {"format", this->DATA.format},
                         {"encoding", this->DATA.encoding},
                         {"audio", get_base64_encode(string(temp, op.header_length + op.body_length))},
                     }}};
        this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);
        // init a new page
        init_ogg_page(op);
        ogg_page_put_packet(os, op, (char *)opus, nbytes);
    }
    os.page_flag = 0x04;
    ogg_page_encapsulate(os, op);
    memcpy(temp, op.header, op.header_length);
    memcpy(temp + op.header_length, op.body, op.body_length);
    // 最后一帧处理
    data = {
        {"data", {
                     {"status", 2},
                     {"format", this->DATA.format},
                     {"encoding", this->DATA.encoding},
                     {"audio", get_base64_encode(string(temp, op.header_length + op.body_length))},
                 }}};
    this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);

    opus_encoder_destroy(enc);
    delete[] pcm;
    delete[] opus;
    fclose(fin);
}

/**
 * @brief websocket收到服务器数据时的回调函数
 * @param hdl 当前连接的句柄
 * @param msg 服务器数据的句柄
 */
void iat_client::on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg)
{
    static int cnt = 0;
    fprintf(stdout, "\r[INFO] WebSocket's STATE is ON_MESSAGE, No.%d frame received", ++cnt);
    fflush(stdout);

    json recv_data = json::parse(msg->get_payload());
    static string sid = recv_data["sid"];
    static string result_str = "";
    int code = recv_data["code"];

    // 拼接结果
    if (code == 0)
    {
        json result = recv_data["data"]["result"]["ws"];
        for (auto &temp : result)
        {
            result_str += temp["cw"][0]["w"];
        }

        // 是否是最后一片结果
        if (recv_data["data"]["result"]["ls"])
        {
            // 客户端主动关闭连接
            asio_tls_client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
            if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
            {
                con->close(0, "receive over");
            }

            // 输出最终结果
            fprintf(stdout, "\n[SUCCESS] sid: \"%s\" call success. Result is \"%s\"\n", sid.c_str(), result_str.c_str());
        }
    }
    else
    {
        // 客户端主动关闭连接
        asio_tls_client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
        if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
        {
            con->close(0, "receive over");
        }

        cout << "\n[ERROR] sid: \"" << sid << "\" call error. ERROR_CODE: \"" << code << "\", ERROR_MSG: " << recv_data["message"] << endl;
        exit(1);
    }
}