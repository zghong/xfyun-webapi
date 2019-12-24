/**
 * @Copyright: https://www.xfyun.cn/
 * 
 * @Author: iflytek
 * 
 * @Data: 2019-12-23
 * 
 * 本文件包含“讯飞开放平台-语音识别-实时语音转写”的WebAPI接口Demo相关类及参数定义
 * WSSClient类基于websocketpp 0.8.1开源框架实现，具体查看：https://github.com/zaphoyd/websocketpp
 * 
 * WSSlient类实现了向“讯飞开放平台-语音识别-实时语音转写”服务器发送基于WSS的websocket请求
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
// 接口鉴权参数
struct API_IFNO
{
	string APIKey;
};

// 公共参数
struct COMMON_INFO
{
	string APPID;
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
 * API, 接口授权参数
 * COMMON，公共参数
 * OTHER，其他参数
 */
class WSSClient
{
public:
	WSSClient(API_IFNO API, COMMON_INFO COMMON, OTHER_INFO OTHER);

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
	OTHER_INFO OTHER;
};

/**
 * 实现WSSClient
 */
// 构造函数
WSSClient::WSSClient(API_IFNO API, COMMON_INFO COMMON, OTHER_INFO OTHER)
	: API(API), COMMON(COMMON), OTHER(OTHER)
{
	// 开启/关闭相关日志
	// this->wssclient.set_access_channels(websocketpp::log::alevel::all);
	this->wssclient.clear_access_channels(websocketpp::log::alevel::all);
	// this->wssclient.set_error_channels(websocketpp::log::elevel::all);sssss
	this->wssclient.clear_error_channels(websocketpp::log::alevel::all);

	// 初始化Asio
	this->wssclient.init_asio();

	// 绑定事件
	using websocketpp::lib::bind;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	this->wssclient.set_open_handler(bind(&WSSClient::on_open, this, _1));
	this->wssclient.set_close_handler(bind(&WSSClient::on_close, this, _1));
	this->wssclient.set_fail_handler(bind(&WSSClient::on_fail, this, _1));
	this->wssclient.set_message_handler(bind(&WSSClient::on_message, this, _1, _2));
	this->wssclient.set_tls_init_handler(bind(&WSSClient::on_tls_init)); // tls初始化，用于wss
}

// 启动客户端
void WSSClient::start_client()
{
	// 获取鉴权url
	string url = this->get_url();
	// cout << url << endl;

	// 创建一个新的连接请求
	websocketpp::lib::error_code ec;
	client::connection_ptr con = this->wssclient.get_connection(url, ec);
	if (ec)
	{
		throw "Get Connection Error: " + ec.message();
	}

	// 连接到url
	this->wssclient.connect(con);
	// 运行
	this->wssclient.run();
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

	cout << msg->get_payload() << endl;

	// string temp = msg->get_payload();

	// temp.erase(remove(temp.begin(), temp.end(), '\\'), temp.end());

	// cout << temp << endl;

	// 返回的json字符串有错误，data为首个json字符串，但是服务器封装是没有解析之，而是直接返回，导致客户端解析失败

	// ------------------------------------------------------------------------------------------
	// TODO: 服务器返回的json有误，待解决

	// json recv_data = json::parse(temp);

	// static string sid = recv_data["sid"];
	// int code = recv_data["code"];

	// cout << "解析错误" << endl;

	// // 拼接结果
	// if (code == 0 && recv_data["action"] == "result")
	// {
	// 	cout << "获得结果" << endl;
	// 	string result_str;

	// 	json result = recv_data["data"]["cn"]["st"]["rt"][0]["ws"];
	// 	for (auto &temp : result)
	// 	{
	// 		result_str += temp["cw"][0]["w"];
	// 	}

	// 	cout << result_str << endl;

	// 	if (recv_data["data"]["cn"]["st"]["type"] == "1")
	// 	{
	// 		// 中间结果
	// 		printf("\r%s", result_str.c_str());
	// 		fflush(stdout);
	// 	}
	// 	else
	// 	{
	// 		// 最终结果
	// 		printf("\n最终结果:%s", result_str.c_str());
	// 		// 客户端主动关闭连接
	// 		client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
	// 		if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
	// 		{
	// 			con->close(0, "receive over");
	// 		}
	// 	}
	// }
	// else
	// {
	// 	// 客户端主动关闭连接
	// 	client::connection_ptr con = this->wssclient.get_con_from_hdl(hdl);
	// 	if (con != NULL && con->get_state() == websocketpp::session::state::value::open)
	// 	{
	// 		con->close(0, "receive over");
	// 	}

	// 	stringstream ss;
	// 	ss << "[response info] "
	// 	   << "sid: " << sid << " call error, code: " << code << ", errMsg: " << recv_data["message"];
	// 	throw ss.str();
	// }
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

	FILE *fp = fopen(this->OTHER.audio_file.c_str(), "rb");
	if (fp == NULL)
	{
		// 文件打开错误
		throw "open file: [" + this->OTHER.audio_file + "] failed!";
	}

	// 音频数据帧缓冲区
	int count = 0;
	unsigned char *pcm = new unsigned char[frame_len + 10];

	while (!feof(fp))
	{
		int size = fread(pcm, sizeof(char), frame_len, fp);

		this->wssclient.send(hdl, string((char *)pcm, size), websocketpp::frame::opcode::binary);

		// 模拟音频采样间隔
		delay(interval);
		// 输出进度
		printf("\r第%d帧已发送...", ++count);
		fflush(stdout);
	}

	fclose(fp);

	// 上传结束标志
	this->wssclient.send(hdl, "{\"end\": true}", websocketpp::frame::opcode::text);
}

// 生成接口鉴权url
string WSSClient::get_url()
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