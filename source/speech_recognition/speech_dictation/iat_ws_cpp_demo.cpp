/**
 * @author iflytek
 * 
 * 本demo测试时运行环境为：Ubuntu18.04
 * 本demo测试成功运行时所安装的第三方库及其版本如下：
 * boost 1.69.0
 * websocketpp 0.8.1
 * openssl 1.1.1
 * 注：测试时，websocketpp 0.8.1最高兼容的版本是1.69.0，具体查看：https://github.com/zaphoyd/websocketpp/issues
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

#include "wssclient.hpp"

// 编译运行前，请填写相关参数
// g++ iat_ws_cpp_demo.cpp -lboost_system -lpthread -lcrypto -lssl
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
    // 更多个性化参数可在官网查看
};

const DATA_INFO DATA{
    format : "audio/L16;rate=16000",
    encoding : "raw",
};

const string AudioFile = "";

int main(int argc, char *argv[])
{
    time_t start_time = clock();

    try
    {
        WSSClient wssc(AudioFile, API, COMMON, BUSINESS, DATA);
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