/**
 * @Copyright: https://www.xfyun.cn/
 * 
 * @Author: iflytek
 * 
 * @Data: 2019-12-23
 * 
 * 本文件包含“讯飞开放平台-语音扩展-性别年龄识别”的WebAPI接口Demo相关类及参数定义
 * WSSClient类基于websocketpp 0.8.1开源框架实现，具体查看：https://github.com/zaphoyd/websocketpp
 * 
 * WSSlient类实现了向“讯飞开放平台-语音扩展-性别年龄识别”服务器发送基于WSS的websocket请求
 * 如果需要实现WS的websocket请求请参考websoketpp帮助文档：https://www.zaphoyd.com/websocketpp
 */

#ifndef _WSSCLIENT_HPP
#define _WSSCLIENT_HPP

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

#include "utils.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

/**
 * WebAPI接口常量及相关参数定义
 */
// 帧标识
enum STATUS_INFO
{
	STATUS_FIRST_FRAME,	// 第一帧的标识
	STATUS_CONTINUE_FRAME, // 中间帧标识
	STATUS_LAST_FRAME,	 // 最后一帧的标识
};

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
	string rate;
};

// 业务数据流参数
struct DATA_INFO
{
	string audio;
	// int status; // status程序运行中指定
};

// 其他参数
struct OTHER_INFO
{
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

		cout << "sid: " << sid << "识别成功，以下为识别结果：" << endl;

		// 年龄
		string child_probability = result["result"]["age"]["child"];
		string middle_probability = result["result"]["age"]["middle"];
		string old_probability = result["result"]["age"]["old"];
		string age_type = "儿童";
		if (result["result"]["age"]["age_type"] == "0")
			age_type = "中年";
		else if (result["result"]["age"]["age_type"] == "2")
			age_type = "老年";
		printf("年龄：儿童(0~12岁)[%s]，中年(12~40岁)[%s]，老年(40岁以上)[%s]。概率最高为[%s]。\n", child_probability.c_str(), middle_probability.c_str(), old_probability.c_str(), age_type.c_str());

		// 性别
		string male_probability = result["result"]["gender"]["male"];
		string female_probability = result["result"]["gender"]["female"];
		string gender_type = "男声";
		if (result["result"]["gender"]["gender_type"] == "0")
			gender_type = "女声";
		printf("性别：男声[%s]，女声[%s]。概率最高为[%s]。\n", male_probability.c_str(), female_probability.c_str(), gender_type.c_str());

		if (result["status"] == 2)
		{
			// 客户端主动关闭连接
			client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
			if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
			{
				con->close(0, "receive over");
			}
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
	// 原始每一帧的音频大小(单位:byte)，sample_rate*bit_depth*channel*frame_time
	// int frame_len = this->OTHER.sample_rate * this->OTHER.bit_depth * this->OTHER.channel * this->OTHER.frame_time / 8;
	int frame_len = 1280;
	// 发送音频帧间隔(单位:s)
	// double interval = this->OTHER.frame_time;
	double interval = 0.04;
	// 音频的状态信息，标识音频是第一帧，还是中间帧、最后一帧
	STATUS_INFO current_status = STATUS_FIRST_FRAME;

	FILE *fp = fopen(this->OTHER.audio_file.c_str(), "rb");
	if (fp == NULL)
	{
		// 文件打开错误
		throw "open file: [" + this->OTHER.audio_file + "] failed!";
	}

	// 音频数据帧缓冲区
	int count = 0;
	unsigned char *pcm = new unsigned char[frame_len + 10];

	while (current_status != STATUS_LAST_FRAME)
	{
		int size = fread(pcm, sizeof(char), frame_len, fp);

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
				{"common", {{"app_id", this->COMMON.APPID}}},
				{"business", {{"aue", this->BUSINESS.aue}, {"rate", this->BUSINESS.rate}}},
				{"data", {
							 {"status", 0},
							 {"audio", get_base64_encode(string((char *)pcm, size))},
						 }}};

			this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);
			current_status = STATUS_CONTINUE_FRAME;
			break;
		}
		case STATUS_CONTINUE_FRAME:
		{
			// 中间帧处理
			json data = {
				{"data", {
							 {"status", 1},
							 {"audio", get_base64_encode(string((char *)pcm, size))},
						 }}};

			this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);
			break;
		}
		case STATUS_LAST_FRAME:
		{
			// 最后一帧处理
			json data = {
				{"data", {
							 {"status", 2},
							 {"audio", get_base64_encode(string((char *)pcm, size))},
						 }}};

			this->wssclient.send(hdl, data.dump(), websocketpp::frame::opcode::text);
			break;
		}
		}

		// 模拟音频采样间隔
		delay(interval);
		// 输出进度
		printf("\r第%d帧已发送...", ++count);
		fflush(stdout);
	}

	fclose(fp);
}

// 生成接口鉴权url
string WSSClient::get_url()
{
	// 生成RFC1123格式的时间戳，"Thu, 05 Dec 2019 09:54:17 GMT"
	string date = get_time_rfc1123();

	// 拼接signature的原始字符串
	string signature_origin = "host: ws-api.xfyun.cn\n";
	signature_origin += "date: " + date + "\n";
	signature_origin += "GET /v2/igr HTTP/1.1";

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
			authorization.c_str(), date.c_str(), "ws-api.xfyun.cn");
	string url = "wss://ws-api.xfyun.cn/v2/igr?" + get_url_encode(string(buf));

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