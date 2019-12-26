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
 * speex 1.2.0
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
 * 注：项目目录已安装websocket 0.8.1，speex 1.2.0；但是运行环境应该还要自行安装boost 0.8.1，libssl-dev 1.1.1
 * 
 * 语音合成（流式版）WebAPI 文档：https://www.xfyun.cn/doc/tts/online_tts/API.html
 * 错误码链接：https://www.xfyun.cn/document/error-code
 */

#include "wssclient.hpp"

// 编译运行前，请填写相关参数
// g++ iat_ws_cpp_demo.cpp -lboost_system -lpthread -lcrypto -lssl -lspeex -I ../../../include/ -L ../../../lib/
const API_IFNO API{
    APISecret : "",
    APIKey : "",
};

const COMMON_INFO COMMON{
    APPID : "",
};

const BUSINESS_INFO BUSINESS{
    aue : "speex-wb", // 样例采用speex-wb编码
    vcn : "xiaoyan",
    tte : "UTF8",
    // 更多个性化参数可在官网查看
};

// text或者text_file至少指定一个
const DATA_INFO DATA{
    text : "这是语音合成样例，哈哈哈哈哈哈。",
};

const OTHER_INFO OTHER{
    text_file : "",
    audio_file : "haha.spx", // 生成的语音文件保存路径
};

int main(int argc, char *argv[])
{
    time_t start_time = clock();

    try
    {
        WSSClient wssc(API, COMMON, BUSINESS, DATA, OTHER);
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