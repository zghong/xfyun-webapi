/**
 * @Copyright: https://www.xfyun.cn/
 * 
 * @Author: iflytek
 * 
 * @Data: 2019-12-23
 * 
 * 本文件包含“讯飞开放平台-语音合成-在线语音合成”的WebAPI接口Demo相关类及参数定义
 * WSSClient类基于websocketpp 0.8.1开源框架实现，具体查看：https://github.com/zaphoyd/websocketpp
 * 
 * WSSlient类实现了向“讯飞开放平台-语音合成-在线语音合成”服务器发送基于WSS的websocket请求
 * 如果需要实现WS的websocket请求请参考websoketpp帮助文档：https://www.zaphoyd.com/websocketpp
 */

#ifndef _WSSCLIENT_HPP
#define _WSSCLIENT_HPP

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

#include "speex/speex.h"

#include "utils.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

/**
 * WebAPI接口常量及相关参数定义
 */
// 接口鉴权参数
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
	string aue;
	string vcn;
	string tte;
	// 更多个性化参数可在官网查看
};

// 业务数据流参数
struct DATA_INFO
{
	string text;
	// int status; // 数据状态，由于流式合成的文本只能一次性传输，不支持多次分段传输，此处status必须为2
};

// 其他参数
struct OTHER_INFO
{
	string text_file;
	string audio_file;
};

/**
 * 定义WSSClient类，与服务器进行websocket通信的wss客户端
 * 
 * wssclient，websocketpp对象
 * API，接口鉴权参数
 * COMMON，公共参数
 * BUSINESS，业务参数
 * DATA，业务数据流参数
 * OTHER，其他参数
 */
class WSSClient
{
public:
	WSSClient(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER);

	void start_client();
	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
	void send_data(websocketpp::connection_hdl hdl);
	string get_url();
	static context_ptr on_tls_init();

private:
	client wssclient;
	API_IFNO API;
	COMMON_INFO COMMON;
	BUSINESS_INFO BUSINESS;
	DATA_INFO DATA;
	OTHER_INFO OTHER;
};

/**
 * 实现WSSClient
 */
// 构造函数
WSSClient::WSSClient(API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA, OTHER_INFO OTHER)
	: API(API), COMMON(COMMON), BUSINESS(BUSINESS), DATA(DATA), OTHER(OTHER)
{
	// 开启/关闭相关日志
	// wssclient.set_access_channels(websocketpp::log::alevel::all);
	wssclient.clear_access_channels(websocketpp::log::alevel::all);
	// wssclient.set_error_channels(websocketpp::log::elevel::all);sssss
	wssclient.clear_error_channels(websocketpp::log::alevel::all);

	// 初始化Asio
	wssclient.init_asio();

	// 绑定事件
	using websocketpp::lib::bind;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	wssclient.set_open_handler(bind(&WSSClient::on_open, this, _1));
	wssclient.set_close_handler(bind(&WSSClient::on_close, this, _1));
	wssclient.set_fail_handler(bind(&WSSClient::on_fail, this, _1));
	wssclient.set_message_handler(bind(&WSSClient::on_message, this, _1, _2));
	wssclient.set_tls_init_handler(bind(&WSSClient::on_tls_init)); // tls初始化，用于wss
}

// 启动客户端
void WSSClient::start_client()
{
	// 获取鉴权url
	string url = this->get_url();
	// cout << url << endl;

	// 创建一个新的连接请求
	websocketpp::lib::error_code ec;
	client::connection_ptr con = wssclient.get_connection(url, ec);
	if (ec)
	{
		throw "Get Connection Error: " + ec.message();
	}

	// 连接到url
	wssclient.connect(con);
	// 运行
	wssclient.run();
}

// 打开连接事件的回调函数
void WSSClient::on_open(websocketpp::connection_hdl hdl)
{
	cout << "on_open" << endl;

	// 开启线程，发送文本数据
	websocketpp::lib::thread send_data_thread(&WSSClient::send_data, this, hdl);
	send_data_thread.detach();
}

// 关闭连接事件的回调函数
void WSSClient::on_close(websocketpp::connection_hdl hdl)
{
	cout << "on_close" << endl;

	// client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
	// cout << con->get_ec() << " - " << con->get_ec().message() << endl;
}

// 发生错误事件的回调函数
void WSSClient::on_fail(websocketpp::connection_hdl hdl)
{
	cout << "on_fail" << endl;

	client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
	stringstream ss;
	ss << "[websocketpp info] " << con->get_ec() << " - " << con->get_ec().message() << endl;
	ss << "[response info] " << con->get_response_code() << " - " << con->get_response_msg();
	throw ss.str();
}

// 从服务器收到数据的回调函数
void WSSClient::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
{
	// cout << "on_message" << endl;

	json recv_data = json::parse(msg->get_payload());

	static string sid = recv_data["sid"];
	int code = recv_data["code"];

	// 拼接结果
	if (code == 0)
	{
		json result = recv_data["data"];

		string temp = get_base64_decode(result["audio"]);

		// 保存原始speex数据
		FILE *fout = fopen(this->OTHER.audio_file.c_str(), "ab");
		fwrite(temp.c_str(), sizeof(char), temp.size(), fout);
		fclose(fout);

		if (result["status"] == 2)
		{
			// 将保存好的speex数据解码成pcm数据
			FILE *fin = fopen(this->OTHER.audio_file.c_str(), "rb");
			FILE *fout = fopen((this->OTHER.audio_file + ".out.pcm").c_str(), "wb");

			char *spx = new char[640];
			char *out_pcm = new char[640];

			// 定义解码器
			void *dec_state;
			SpeexBits dec_bits;

			speex_bits_init(&dec_bits);
			dec_state = speex_decoder_init(&speex_wb_mode);

			// 开始解码
			while (!feof(fin))
			{
				// 读取1字节头部
				fread(spx, sizeof(char), 1, fin);
				int len = int(*spx);
				int size = fread(spx, sizeof(char), len, fin);

				speex_bits_reset(&dec_bits);
				speex_bits_read_from(&dec_bits, spx, len);
				speex_decode_int(dec_state, &dec_bits, (spx_int16_t *)out_pcm);
				fwrite(out_pcm, sizeof(char), 640, fout);
			}

			speex_bits_destroy(&dec_bits);
			speex_decoder_destroy(dec_state);

			fclose(fin);
			fclose(fout);

			// 客户端主动关闭连接
			client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
			if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
			{
				con->close(0, "receive over");
			}

			// 输出最终结果
			cout << "sid: " << sid << " call success. The original file (.spx) is saved in [" << this->OTHER.audio_file
				 << "], and the decoded file (.pcm) is saved in [" << this->OTHER.audio_file << ".out.pcm]." << endl;
		}
	}
	else
	{
		// 客户端主动关闭连接
		client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
		if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
		{
			con->close(0, "receive over");
		}

		stringstream ss;
		ss << "[response info] "
		   << "sid: " << sid << " call error, code: " << code << ", errMsg: " << recv_data["message"];
		throw ss.str();
	}
}

// 发送音频数据给服务器
void WSSClient::send_data(websocketpp::connection_hdl hdl)
{
	if (this->DATA.text == "" && this->OTHER.text_file == "")
	{
		throw "合成的文本(DATA.text)和合成的文本路径(OTHER.text_file)至少指定一个";
	}
	else if (this->OTHER.text_file != "")
	{
		FILE *fin = fopen(this->OTHER.text_file.c_str(), "rb");
		fseek(fin, 0, SEEK_END);
		int size = ftell(fin);
		fseek(fin, 0, SEEK_SET);

		char *text = new char[size];
		fread(text, sizeof(char), size, fin);

		this->DATA.text = string(text);

		fclose(fin);
	}

	json data = {
		{"common", {{"app_id", this->COMMON.APPID}}},
		{"business", {{"aue", this->BUSINESS.aue}, {"vcn", this->BUSINESS.vcn}, {"tte", this->BUSINESS.tte}}},
		{"data", {
					 {"status", 2},
					 {"text", get_base64_encode(this->DATA.text)},
				 }}};

	this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);
}

// 生成接口鉴权url
string WSSClient::get_url()
{
	// 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 GMT"
	string date = get_time_rfc1123();

	// 拼接signature的原始字符串
	string signature_origin = "host: tts-api.xfyun.cn\n";
	signature_origin += "date: " + date + "\n";
	signature_origin += "GET /v2/tts HTTP/1.1";

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
			authorization.c_str(), date.c_str(), "tts-api.xfyun.cn");
	string url = "wss://tts-api.xfyun.cn/v2/tts?" + get_url_encode(string(buf));

	return url;
}

// tls初始化，建立一个SSL连接
context_ptr WSSClient::on_tls_init()
{
	// 建立一个SSL连接
	context_ptr ctx = make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	try
	{
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
						 boost::asio::ssl::context::no_sslv2 |
						 boost::asio::ssl::context::no_sslv3 |
						 boost::asio::ssl::context::single_dh_use);
	}
	catch (exception &e)
	{
		throw "Error in context pointer: " + string(e.what());
	}
	return ctx;
}

#endif