/**
 * @Copyright: https://www.xfyun.cn/
 * 
 * @Author: iflytek
 * 
 * @Data: 2019-12-23
 * 
 * 本demo测试运行时环境为：Ubuntu18.04
 * 本demo测试运行时所安装的第三方库及其版本如下：
 * boost 1.69.0
 * websocketpp 0.8.1
 * libssl-dev 1.1.1
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 
 * 实时语音转写 API 文档：https://www.xfyun.cn/doc/asr/rtasr/API.html
 * 错误码链接：https://www.xfyun.cn/document/error-code （code返回错误码时必看）
 */

#include "wssclient.hpp"

// 编译运行前，请填写相关参数
// g++ iat_ws_cpp_demo.cpp -lboost_system -lpthread -lcrypto -lssl
const API_IFNO API{
    APISecret : "4d3dcdf6184d4aa4d987cecca7de3896",
    APIKey : "bd14b970ca48422fffa5b824eec4ad30"
};

const COMMON_INFO COMMON{
    APPID : "5dde2318"
};

const OTHER_INFO OTHER{
    audio_file : "../../../bin/audio/test.pcm",
};

int main(int argc, char *argv[])
{
    time_t start_time = clock();

    try
    {
        WSSClient wssc(API, COMMON, OTHER);
        wssc.start_client();
    }
    catch (string str)
    {
        cout << "## str error ## " << endl;
        cout << str << endl;
    }
    catch (...)
    {
        cout << "## other exception ## " << endl;
    }

    time_t end_time = clock();
    cout << "Time used: " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}