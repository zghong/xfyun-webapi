/**
 * @Copyright: https://www.xfyun.cn/
 * @Author: iflytek
 * @Data: 2019-12-20
 * 
 * 本文件包含“讯飞开放平台”的WebAPI接口，音频编解码类定义及实现，iflytek_codec实现过程中采用了如下开源的音频编解码框架:
 * 1. opus 1.3.1, http://www.opus-codec.org/
 * 2. speex 1.2.0, https://www.speex.org/
 * 3. 待续
 * 
 * iflytek_codec在实现过程中，加入了“讯飞开放平台”定制的编码策略，具体参考“引擎音频编码规范”，本Demo测试通过的音频格式如下：
 * | 格式       | 字段名称 | 编码率 | 声道 | 位数 | 帧时长            | 帧头                                                             |
 * | ---------- | -------- | ------ | ---- | ---- | ----------------- | ---------------------------------------------------------------- |
 * | 原始 PCM   | raw      | 16000  | 1    | 16   |                   |                                                                  |
 * | Speex 窄带 | speex    | 8000   | 1    | 16   |                   | 每一帧前用**1 Byte**保存这一帧的数据长度。                       |
 * | Speex 宽带 | speex-wb | 16000  | 1    | 16   |                   | 每一帧前用**1 Byte**保存这一帧的数据长度。                       |
 * | Opus 窄带  | opus     | 8000   | 1    | 16   | 20ms 的数据为一帧 | 每一帧前用**2 Byte**保存这一帧的数据长度，保存的格式为**大端**。 |
 * | Opus 宽带  | opus-wb  | 16000  | 1    | 16   | 20ms 的数据为一帧 | 每一帧前用**2 Byte**保存这一帧的数据长度，保存的格式为**大端**。 |
 */

#ifndef _IFLYTEK_CODEC_HPP
#define _IFLYTEK_CODEC_HPP

#include <string>

#include "opus/opus.h"
#include "speex/speex.h"

/**
 * @brief 音频编解码抽象类
 * 
 * [public]
 * @func encode_create [纯虚函数]创建编码器
 * @func encode [纯虚函数]编码函数
 * @func encode_destroy [纯虚函数]销毁编码器
 * @func decode_create [纯虚函数]创建解码器
 * @func decode [纯虚函数]解码函数
 * @func decode_destroy [纯虚函数]销毁解码器
 */
class iflytek_codec
{
public:
    // 派生类需要重载如下成员函数
    virtual int encode_create(const std::string type) = 0;
    virtual int encode(const unsigned char *source, const int source_length, unsigned char *dest) = 0;
    virtual void encode_destroy() = 0;
    virtual int decode_create(const std::string type) = 0;
    virtual int decode(const unsigned char *dest, const int dest_length, unsigned char *source) = 0;
    virtual void decode_destroy() = 0;
};

/**
 * @brief opus音频编解码类
 * 
 * [public]
 * @func encode_create 创建opus编码器
 * @func encode opus编码函数
 * @func encode_destroy 销毁opus编码器
 * @func decode_create 创建opus解码器
 * @func decode opus解码函数
 * @func decode_destroy 销毁opus解码器
 * 
 * [private]
 * @member enc opus编码器实例
 * @member dec opus解码器实例
 * @member sample_rate 待编解码音频的采样率
 * @member encoded_bitrate 待编码音频后的编码码率
 * @member frame_size 待编解码音频的帧大小，其与帧长度的区分请查阅相关资料
 */
class opus_codec : public iflytek_codec
{
public:
    int encode_create(const std::string type);
    int encode(const unsigned char *source, const int source_length, unsigned char *dest);
    void encode_destroy();
    int decode_create(const std::string type);
    int decode(const unsigned char *dest, const int dest_length, unsigned char *source);
    void decode_destroy();

private:
    OpusEncoder *enc;
    OpusDecoder *dec;
    int sample_rate, encoded_bitrate, frame_size;
};

/**
 * @brief speex音频编解码类
 * 
 * [public]
 * @func encode_create 创建speex编码器
 * @func encode speex编码函数
 * @func encode_destroy 销毁speex编码器
 * @func decode_create 创建speex解码器
 * @func decode speex解码函数
 * @func decode_destroy 销毁speex解码器
 * 
 * [private]
 * @member enc_state, dec_state Speex编码状态
 * @member enc_bits, dec_bits Speex位封装结构
 * @member sample_rate 待编解码音频的采样率
 * @member quality 质量参数，控制编码后音频的质量和比特率
 * @member frame_size 待编解码音频的帧大小，其与帧长度的区分请查阅相关资料
 */
class speex_codec : public iflytek_codec
{
public:
    int encode_create(const std::string type);
    int encode(const unsigned char *source, const int source_length, unsigned char *dest);
    void encode_destroy();
    int decode_create(const std::string type);
    int decode(const unsigned char *dest, const int dest_length, unsigned char *source);
    void decode_destroy();

private:
    void *enc_state, *dec_state;
    SpeexBits enc_bits, dec_bits;
    int sample_rate, quality, frame_size;
};

/**
 * @brief 创建opus编码器
 * @param type 编码器类型
 * 目前可选值有opus, opus-wb
 * @return 成功时返回opus编码器接受原始数据的每帧长度，失败时返回-1
 */
int opus_codec::encode_create(const std::string type)
{

    if ("opus" == type)
    {
        this->sample_rate = 8000;
        // this->encoded_bitrate = 16000;
    }
    else if ("opus-wb" == type)
    {
        this->sample_rate = 16000;
        // this->encoded_bitrate = 24000;
    }
    else
    {
        fprintf(stderr, "[ERROR] Unsupported encoding format \"%s\"\n", type.c_str());
        return -1;
    }

    // 经测试，目前讯飞云引擎opus编解码只支持帧时长为20ms的数据
    // 通过表达式可以看出，对于位深16，单声道的音频来说，source_length = frame_size * 2
    this->frame_size = this->sample_rate * 0.02;
    int source_length = this->sample_rate / 8 * 16 * 1 * 0.02;

    int err = 0;
    this->enc = opus_encoder_create(this->sample_rate, 1, OPUS_APPLICATION_VOIP, &err);
    if (OPUS_OK != err)
    {
        fprintf(stderr, "[ERROR] Failed to create OPUS Encoder\n");
        return -1;
    }
    else
    {
        // 可以指定固定编码，opus默认使用可变编码
        // opus_encoder_ctl(this->enc, OPUS_SET_VBR(0));                         // 固定码率
        // opus_encoder_ctl(this->enc, OPUS_SET_BITRATE(this->encoded_bitrate)); // 编码码率
        return source_length;
    }
}

/**
 * @brief opus编码函数
 * @param source 原始数据
 * @param source_length 原始数据字节长度
 * @param dest 目的数据
 * @return 成功时返回目的数据的字节长度，失败时返回-1
 */
int opus_codec::encode(const unsigned char *source, const int source_length, unsigned char *dest)
{
    opus_int32 nbytes = opus_encode(this->enc, (opus_int16 *)source, this->frame_size, dest + 2, source_length);
    if (nbytes < 0)
    {
        fprintf(stderr, "[ERROR] Failed to opus_encode raw data\n");
        return -1;
    }
    else
    {
        // 两字节的opus头信息，存储opus编码数据长度，大端
        dest[0] = (nbytes & 0xFF00) >> 8;
        dest[1] = nbytes & 0x00FF;
        return nbytes + 2;
    }
}

/**
 * @brief 销毁opus编码器
 */
void opus_codec::encode_destroy()
{
    opus_encoder_destroy(enc);
}

/**
 * @brief 创建opus解码器
 * @param type 解码器类型
 * 目前可选值有opus, opus-wb
 * @return 成功时返回原始数据的字节长度，失败时返回-1
 */
int opus_codec::decode_create(const std::string type)
{
    if ("opus" == type)
    {
        this->sample_rate = 8000;
    }
    else if ("opus-wb" == type)
    {
        this->sample_rate = 16000;
    }
    else
    {
        fprintf(stderr, "[ERROR] Unsupported decoding format \"%s\"\n", type.c_str());
        return -1;
    }

    // 经测试，目前讯飞云引擎opus编解码只支持帧时长为20ms的数据
    this->frame_size = this->sample_rate * 0.02;

    int err = 0;
    this->dec = opus_decoder_create(this->sample_rate, 1, &err);
    if (OPUS_OK != err)
    {
        fprintf(stderr, "[ERROR] Failed to create OPUS Decoder\n");
        return -1;
    }
    else
    {
        return this->frame_size * 2;
    }
}

/**
 * @brief opus解码函数
 * @param dest 目的数据
 * @param dest_length 目的数据字节长度
 * @param source 原始数据
 * @return 成功时返回原始数据的字节长度，失败时返回-1
 */
int opus_codec::decode(const unsigned char *dest, const int dest_length, unsigned char *source)
{
    int samples = opus_decode(this->dec, dest, dest_length, (opus_int16 *)source, this->frame_size, 0);
    if (samples < 0)
    {
        fprintf(stderr, "[ERROR] Failed to opus_decode opus data\n");
        return -1;
    }
    else
    {
        return samples * 2;
    }
}

/**
 * @brief 销毁opus解码器
 */
void opus_codec::decode_destroy()
{
    opus_decoder_destroy(this->dec);
}

/**
 * @brief 创建speex编码器
 * @param type 编码器类型
 * 目前可选值有speex, speex-wb
 * @return 成功时返回speex编码器接受原始数据的每帧长度，失败时返回-1
 */
int speex_codec::encode_create(const std::string type)
{
    if ("speex" == type)
    {
        // this->sample_rate = 8000;
        this->enc_state = speex_encoder_init(&speex_nb_mode);
    }
    else if ("speex-wb" == type)
    {

        // this->sample_rate = 16000;
        this->enc_state = speex_encoder_init(&speex_wb_mode);
    }
    else
    {
        fprintf(stderr, "[ERROR] Unsupported encoding format \"%s\"\n", type.c_str());
        return -1;
    }

    if (this->enc_state == NULL)
    {
        fprintf(stderr, "[ERROR] Failed to create SPEEX Encoder\n");
        return -1;
    }
    else
    {
        int vbr = 1;
        speex_encoder_ctl(this->enc_state, SPEEX_SET_VBR, &vbr); // 设置为可变编码，默认关闭

        speex_bits_init(&this->enc_bits);
        speex_encoder_ctl(this->enc_state, SPEEX_GET_FRAME_SIZE, &this->frame_size);
        // 通过表达式可以看出，对于位深16，单声道的音频来说，source_length = frame_size * 2
        // int source_length = (this->sample_rate / 8) * 16 * 1 * (this->frame_size * 1.0 / this->sample_rate);
        int source_length = this->frame_size * 2;
        return source_length;
    }
}

/**
 * @brief speex编码函数
 * @param source 原始数据
 * @param source_length 原始数据字节长度
 * @param dest 目的数据
 * @return 成功时返回目的数据的字节长度，失败时返回-1
 */
int speex_codec::encode(const unsigned char *source, const int source_length, unsigned char *dest)
{
    speex_bits_reset(&this->enc_bits);
    speex_encode_int(this->enc_state, (spx_int16_t *)source, &this->enc_bits);
    int nbytes = speex_bits_write(&this->enc_bits, (char *)dest + 1, source_length);
    if (nbytes < 0)
    {
        fprintf(stderr, "[ERROR] Failed to speex_encode raw data\n");
        return -1;
    }
    else
    {
        // 一字节的speex头信息，存储speex编码数据长度
        dest[0] = nbytes;
        return nbytes + 1;
    }
}

/**
 * @brief 销毁speex编码器
 */
void speex_codec::encode_destroy()
{
    speex_bits_destroy(&this->enc_bits);
    speex_encoder_destroy(this->enc_state);
}

/**
 * @brief 创建speex解码器
 * @param type 解码器类型
 * 目前可选值有speex, speex-wb
 * @return 成功时返回原始数据的字节长度，失败时返回-1
 */
int speex_codec::decode_create(const std::string type)
{
    if ("speex" == type)
    {
        this->dec_state = speex_decoder_init(&speex_nb_mode);
    }
    else if ("speex-wb" == type)
    {
        this->dec_state = speex_decoder_init(&speex_wb_mode);
    }
    else
    {
        fprintf(stderr, "[ERROR] Unsupported decoding format \"%s\"\n", type.c_str());
        return -1;
    }

    if (this->dec_state == NULL)
    {
        fprintf(stderr, "[ERROR] Failed to create SPEEX Decoder\n");
        return -1;
    }
    else
    {
        speex_bits_init(&this->dec_bits);
        speex_decoder_ctl(this->dec_state, SPEEX_GET_FRAME_SIZE, &this->frame_size);
        return this->frame_size * 2;
    }
}

/**
 * @brief speex解码函数
 * @param dest 目的数据
 * @param dest_length 目的数据字节长度
 * @param source 原始数据
 * @return 成功时返回原始数据的字节长度，失败时返回-1
 */
int speex_codec::decode(const unsigned char *dest, const int dest_length, unsigned char *source)
{
    speex_bits_reset(&this->dec_bits);
    speex_bits_read_from(&this->dec_bits, (char *)dest, dest_length);
    if (speex_decode_int(dec_state, &dec_bits, (spx_int16_t *)source) < 0)
    {
        fprintf(stderr, "[ERROR] Failed to speex_decode speex data\n");
        return -1;
    }
    else
    {
        return this->frame_size * 2;
    }
}

/**
 * @brief 销毁speex解码器
 */
void speex_codec::decode_destroy()
{
    speex_bits_destroy(&this->dec_bits);
    speex_decoder_destroy(this->dec_state);
}

#endif