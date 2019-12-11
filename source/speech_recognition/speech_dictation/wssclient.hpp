/**
 * @author iflytek
 * 
 * 本文件包含“讯飞开放平台-语音识别-语音听写”的WebAPI接口Demo相关类及参数定义
 * WSSClient类基于websocketpp 0.8.1开源框架实现，具体查看：https://github.com/zaphoyd/websocketpp
 * 
 * WSSSlient类实现了向“讯飞开放平台-语音识别-语音听写”服务器发送基于WSS的websocket请求
 * 如果需要实现WS的websocket请求请参考websoketpp帮助文档：https://www.zaphoyd.com/websocketpp
 */

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include "../../../include/utils.hpp"
#include "../../../include/json.hpp"

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

/**
 * 与服务器进行websocket通信的wss客户端类
 * m_client，websocketpp对象
 * AudioFile，音频文件路径
 * COMMON，公共参数
 * BUSINESS，业务参数
 * DATA，业务数据流参数
 */
class WSSClient
{
public:
	WSSClient(string AudioFile, API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA);
	void start_client();
	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
	void send_data(websocketpp::connection_hdl hdl);
	string get_url();
	static context_ptr on_tls_init();

private:
	client m_client;
	string AudioFile;
	API_IFNO API;
	COMMON_INFO COMMON;
	BUSINESS_INFO BUSINESS;
	DATA_INFO DATA;
};

/**
 * 对WSSClient的实现
 */
WSSClient::WSSClient(string AudioFile, API_IFNO API, COMMON_INFO COMMON, BUSINESS_INFO BUSINESS, DATA_INFO DATA)
	: AudioFile(AudioFile), API(API), COMMON(COMMON), BUSINESS(BUSINESS), DATA(DATA)
{
	// 开启/关闭相关日志
	// m_client.set_access_channels(websocketpp::log::alevel::all);
	m_client.clear_access_channels(websocketpp::log::alevel::all);
	// m_client.set_error_channels(websocketpp::log::elevel::all);
	m_client.clear_error_channels(websocketpp::log::alevel::all);

	// 初始化Asio
	m_client.init_asio();

	// 绑定事件
	using websocketpp::lib::bind;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	m_client.set_open_handler(bind(&WSSClient::on_open, this, _1));
	m_client.set_close_handler(bind(&WSSClient::on_close, this, _1));
	m_client.set_fail_handler(bind(&WSSClient::on_fail, this, _1));
	m_client.set_message_handler(bind(&WSSClient::on_message, this, _1, _2));
	m_client.set_tls_init_handler(bind(&WSSClient::on_tls_init)); // tls初始化，用于wss
}

// 启动客户端
void WSSClient::start_client()
{
	// 获取鉴权url
	string url = this->get_url();
	// cout << url << endl;

	// 创建一个新的连接请求
	websocketpp::lib::error_code ec;
	client::connection_ptr con = m_client.get_connection(url, ec);
	if (ec)
	{
		throw "Get Connection Error: " + ec.message();
	}

	// 连接到url
	m_client.connect(con);
	// 运行
	m_client.run();
}

// 打开连接事件的回调函数
void WSSClient::on_open(websocketpp::connection_hdl hdl)
{
	cout << "on_open" << endl;

	// 开启线程，发送音频数据
	websocketpp::lib::thread send_data_thread(&WSSClient::send_data, this, hdl);
	send_data_thread.detach();
}

// 关闭连接事件的回调函数
void WSSClient::on_close(websocketpp::connection_hdl hdl)
{
	cout << "on_close" << endl;

	// client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
	// cout << con->get_ec() << " - " << con->get_ec().message() << endl;
}

// 发生错误事件的回调函数
void WSSClient::on_fail(websocketpp::connection_hdl hdl)
{
	cout << "on_fail" << endl;

	client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
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
			client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
			if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
			{
				con->close(0, "receive over");
			}

			// 输出最终结果
			cout << "sid: " << sid << " call success. The result is: \n"
				 << result_str << endl;
		}
	}
	else
	{
		// 客户端主动关闭连接
		client::connection_ptr con = this->m_client.get_con_from_hdl(hdl);
		if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
		{
			con->close(0, "receive over");
		}

		stringstream ss;
		ss << "[response info] " << "sid: " << sid << " call error, code: " << code << ", errMsg: " << recv_data["message"];
		throw ss.str();
	}
}

// 发送音频数据给服务器
void WSSClient::send_data(websocketpp::connection_hdl hdl)
{
	int frameSize = 1280 * 2;						 // 每一帧的音频大小
	double intervel = 0.04 * 2;						 // 发送音频间隔(单位:s)
	STATUS_INFO current_status = STATUS_FIRST_FRAME; // 音频的状态信息，标识音频是第一帧，还是中间帧、最后一帧

	FILE *fp = fopen(this->AudioFile.c_str(), "rb");
	if (fp == NULL)
	{
		// 文件打开错误
		throw "open file: [" + this->AudioFile + "] failed!";
	}

	// 音频数据帧缓冲区
	// int count = 0;
	char *buffer = new char[frameSize + 10];
	while (current_status != STATUS_LAST_FRAME)
	{
		int size = fread(buffer, sizeof(char), frameSize, fp);
		// TODO: 音频编解码

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
		// // 输出进度
		// printf("\r第%d帧已发送...", ++count);
		// fflush(stdout);
	}

	fclose(fp);
}

// 生成接口鉴权url
string WSSClient::get_url()
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
	string url = "wss://iat-api.xfyun.cn/v2/iat?" + get_url_encode(string(buf));

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