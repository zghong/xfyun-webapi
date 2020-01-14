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
 * speex 1.2.0（本Demo解码方案采取的speex）
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 注：项目目录已安装websocket 0.8.1，speex 1.2.0；但是运行环境应该还要自行安装boost 0.8.1，libssl-dev 1.1.1
 * 
 * 语音合成（流式版）WebAPI 文档：https://www.xfyun.cn/doc/tts/online_tts/API.html
 * 错误码链接：https://www.xfyun.cn/document/error-code
 */

// 编译运行前，请填写相关参数
// g++ tts_wss_cpp_demo.cpp -lboost_system -lpthread -lcrypto -lssl -lspeex -I ../include/ -L ../lib/
#include "iflytek_wssclient.hpp"
#include "iflytek_codec.hpp"
#include "iflytek_utils.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

/***************************************************
 * 定义部分
 * 
 * 语音合成，所涉及参数的定义
 * 用于wss通信类tts_client的定义
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
    string aue;
    string auf;
    string vcn;
    string tte;
    // 更多个性化参数可在官网查看
} BUSINESS{
    aue : "speex-wb", // 测试raw, opus, opus-wb, speex, speex-wb通过
    auf : "audio/L16;rate=16000",
    vcn : "xiaoyan",
    tte : "UTF8",
    // 更多个性化参数可在官网查看
};

// 业务数据流参数
struct DATA_INFO
{
    string text;
    // int status; // 数据状态，由于流式合成的文本只能一次性传输，不支持多次分段传输，此处status必须为2
} DATA{
    // text或者text_file至少指定一个
    text : "长太息以掩涕兮，哀民生之多艰。余虽好修姱以鞿羁兮，謇朝谇而夕替。既替余以蕙纕兮，又申之以揽茝。亦余心之所善兮，虽九死其犹未悔。\
            怨灵修之浩荡兮，终不察夫民心。众女嫉余之蛾眉兮，谣诼谓余以善淫。固时俗之工巧兮，偭规矩而改错。背绳墨以追曲兮，竞周容以为度。\
            忳郁邑余侘傺兮，吾独穷困乎此时也。宁溘死以流亡兮，余不忍为此态也。鸷鸟之不群兮，自前世而固然。何方圜之能周兮，夫孰异道而相安？\
            屈心而抑志兮，忍尤而攘诟。伏清白以死直兮，固前圣之所厚。悔相道之不察兮，延伫乎吾将反。回朕车以复路兮，及行迷之未远。\
            步余马于兰皋兮，驰椒丘且焉止息。进不入以离尤兮，退将复修吾初服。制芰荷以为衣兮，集芙蓉以为裳。不吾知其亦已兮，苟余情其信芳。\
            高余冠之岌岌兮，长余佩之陆离。芳与泽其杂糅兮，唯昭质其犹未亏。忽反顾以游目兮，将往观乎四荒。佩缤纷其繁饰兮，芳菲菲其弥章。\
            民生各有所乐兮，余独好修以为常。虽体解吾犹未变兮，岂余心之可惩。"
};

// 其他参数
struct OTHER_INFO
{
    string text_file;
    string audio_file;
} OTHER{
    text_file : "",
    audio_file : "speex-wb.spx" // 生成的语音文件保存路径
};

// tts_client类，继承于iflytek_wssclient
// 用于与服务器进行websocket(wss)通信
class tts_client : public iflytek_wssclient
{
public:
    tts_client(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER);

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
 * 定义tts_client类对象
 * 运行客户端
 ***************************************************
 */
int main(int argc, char *argv[])
{
    time_t start_time = clock();

    tts_client client(API, COMMON, BUSINESS, DATA, OTHER);
    client.run_client();

    time_t end_time = clock();
    fprintf(stdout, "[INFO] Time used: %fs\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    return 0;
}

/***************************************************
 * tts_client类实现部分
 * 
 * 实现tts_client类的相关函数
 ***************************************************
 */
/**
 * @brief 构造函数
 * 对语音合成API所涉及参数的初始化
 */
tts_client::tts_client(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER)
    : API(API), COMMON(COMMON), BUSINESS(BUSINESS), DATA(DATA), OTHER(OTHER)
{
}

/**
 * @brief 获得建立连接的鉴权url
 * @return 鉴权url
 */
string tts_client::get_url()
{
    // 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 GMT"
    time_t rawtime = time(NULL);
    char buf[1024];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %X %Z", gmtime(&rawtime));
    string date = string(buf);

    // 拼接signature的原始字符串
    string signature_origin = "host: tts-api.xfyun.cn\n";
    signature_origin += "date: " + date + "\n";
    signature_origin += "GET /v2/tts HTTP/1.1";

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
            authorization.c_str(), date.c_str(), "tts-api.xfyun.cn");
    string url = "wss://tts-api.xfyun.cn/v2/tts?" + get_url_encode(string(buf));

    return url;
}

/**
 * @brief 向服务器发送数据
 * @param hdl 当前连接的句柄
 */
void tts_client::send_data(websocketpp::connection_hdl hdl)
{
    fprintf(stdout, "[INFO] Sending text data to server...\n");

    if (this->DATA.text == "" && this->OTHER.text_file == "")
    {
        fprintf(stderr, "[ERROR] Provide at least one, between \"DATA.text\" and \"OTHER.text_file\"\n");
        exit(1);
    }
    else if (this->OTHER.text_file != "")
    {
        FILE *fin = fopen(this->OTHER.text_file.c_str(), "rb");
        if (fin == NULL)
        {
            // 文件打开错误
            fprintf(stderr, "[ERROR] Failed to open the file \"%s\"\n", this->OTHER.text_file.c_str());
            exit(1);
        }
        fseek(fin, 0, SEEK_END);
        int size = ftell(fin);
        fseek(fin, 0, SEEK_SET);

        char *text = new char[size];
        fread(text, sizeof(char), size, fin);
        this->DATA.text = string(text);

        fclose(fin);
        delete[] text;
    }

    json data = {
        {"common", {{"app_id", this->COMMON.APPID}}},
        {"business", {{"aue", this->BUSINESS.aue}, {"auf", this->BUSINESS.auf}, {"vcn", this->BUSINESS.vcn}, {"tte", this->BUSINESS.tte}}},
        {"data", {
                     {"status", 2},
                     {"text", get_base64_encode(this->DATA.text)},
                 }}};

    this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);

    fprintf(stdout, "[SUCCESS] No.1 frame sent，OVER\n"); // 只需要发送一帧数据
}

/**
 * @brief websocket收到服务器数据时的回调函数
 * @param hdl 当前连接的句柄
 * @param msg 服务器数据的句柄
 */
void tts_client::on_message(websocketpp::connection_hdl hdl, asio_tls_client::message_ptr msg)
{
    static int cnt = 0;
    // 判断保存音频文件是否存在，存在则删除之
    if (cnt == 0)
    {
        if (FILE *file = fopen(this->OTHER.audio_file.c_str(), "r"))
        {
            fclose(file);
            remove(this->OTHER.audio_file.c_str());
        }
        if (FILE *file = fopen((this->OTHER.audio_file + ".out.pcm").c_str(), "r"))
        {
            fclose(file);
            remove(this->OTHER.audio_file.c_str());
        }
    }
    fprintf(stdout, "\r[INFO] WebSocket's STATE is ON_MESSAGE, No.%d frame received", ++cnt);
    fflush(stdout);

    json recv_data = json::parse(msg->get_payload());
    static string sid = recv_data["sid"];
    int code = recv_data["code"];

    // 拼接结果
    if (code == 0)
    {
        json result = recv_data["data"];

        // 保存原始speex数据
        FILE *fout = fopen(this->OTHER.audio_file.c_str(), "ab");
        string temp = get_base64_decode(result["audio"]);
        fwrite(temp.c_str(), sizeof(char), temp.size(), fout);
        fclose(fout);

        if (result["status"] == 2)
        {
            iflytek_codec *codec = new speex_codec;
            int pcm_length = codec->decode_create(this->BUSINESS.aue);
            if (pcm_length == -1)
            {
                delete codec;
                exit(1);
            }

            // 将保存好的speex数据解码成pcm数据
            FILE *fin = fopen(this->OTHER.audio_file.c_str(), "rb");
            FILE *fout = fopen((this->OTHER.audio_file + ".out.pcm").c_str(), "wb");

            unsigned char *head = new unsigned char;
            unsigned char *speex = new unsigned char[pcm_length];
            unsigned char *pcm = new unsigned char[pcm_length];

            // 开始解码
            while (1)
            {
                // 读取1字节头部
                fread(head, sizeof(char), 1, fin);
                // short *length = (short *)head;
                // *length = (*length << 8) | (*length >> 8);
                // int speex_length = int(*length);
                int speex_length = int(*head);
                if ((speex_length = fread(speex, sizeof(char), speex_length, fin)) == 0)
                {
                    break;
                }

                if ((pcm_length = codec->decode(speex, speex_length, pcm)) == -1)
                {
                    codec->decode_destroy();
                    delete codec;
                    delete head;
                    delete[] speex;
                    delete[] pcm;
                    fclose(fin);
                    fclose(fout);
                    exit(1);
                }
                fwrite(pcm, sizeof(char), pcm_length, fout);
            }

            codec->decode_destroy();
            delete codec;
            delete head;
            delete[] speex;
            delete[] pcm;
            fclose(fin);
            fclose(fout);

            // 客户端主动关闭连接
            asio_tls_client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
            if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
            {
                con->close(0, "receive over");
            }

            // 输出最终结果
            fprintf(stdout, "\n[SUCCESS] sid: \"%s\" call success. The original file (.spx) is saved in \"%s\", and the decoded file (.pcm) is saved in \"%s.out.pcm\"\n",
                    sid.c_str(), this->OTHER.audio_file.c_str(), this->OTHER.audio_file.c_str());
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