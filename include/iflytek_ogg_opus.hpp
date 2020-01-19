/**
 * @Copyright: https://www.xfyun.cn/
 * @Author: iflytek
 * @Data: 2019-1-19
 * 
 * 本文件包含“讯飞开放平台”的WebAPI接口，对opus编码后的音频文件进行ogg封装工具的实现，iflytek_ogg_opus实现过程中采用及参考了如下开源的音频编解码框架:
 * 1. opus 1.3.1, http://www.opus-codec.org/
 * 2. libopusenc 0.2.1, https://www.opus-codec.org/release/dev/2018/10/09/libopusenc-0_2_1.html
 * 3. RFC 3533, which defines the ogg transport bitstream and file format, https://wiki.xiph.org/Ogg
 * 4. 待续
 * 
 * 注：本文件仅根据opus的ogg封装格式对opus进行封装，假定一个包作为一段，不考虑对包分割，故opus单个压缩数据应该限制在255字节
 * 注：如有其他需求，请参考ogg官方文档及libopusenc源码对本文件将进行修改
 */
#ifndef _IFLYTEK_OGG_OPUS_HPP
#define _IFLYTEK_OGG_OPUS_HPP

#include <stdio.h>
#include <cstring>
#include <ctime>
#include <random>

// 每ogg页最多存放10个段，可以自由设置，最大为255段
#define MAX_SEGMENTS 25

// crc_checksum table
static const __uint32_t crc_lookup[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

// ogg页
typedef struct
{
    char header[27 + 255];
    __uint64_t header_length;
    char body[255 * 255];
    __uint64_t body_length;
} ogg_page;

// ogg逻辑流的状态信息
typedef struct
{
    __uint8_t page_flag;
    __uint64_t granule_position;
    __uint32_t serial_number;
    __uint32_t page_counter;
} ogg_logic_stream;

// opus的id头
typedef struct
{
    __uint8_t version;
    __uint8_t channel;
    __uint16_t pre_skip;
    __uint32_t sample_rate;
    __uint16_t gain;
    __uint8_t channel_mapping_family;
    // no set channel mapping table
} opus_id_header;

// opus的comment头
typedef struct
{
    __uint32_t encoder_info_length;
    char *encoder_info;
    __uint32_t comment_number;
    char **comments;
} opus_comment_header;

/**
 * @brief 写16位比特
 * @param p 写入的首地址
 * @param val 待写入的16位数据
 */
void write_uint16(char *p, __uint16_t val)
{
    *p = (val)&0xff;
    *(p + 1) = (val >> 8) & 0xff;
}

/**
 * @brief 写32位比特
 * @param p 写入的首地址
 * @param val 待写入的32位数据
 */
void write_uint32(char *p, __uint32_t val)
{
    for (int i = 0; i < 4; i++)
    {
        *(p + i) = val & 0xff;
        val >>= 8;
    }
}

/**
 * @brief 写64位比特
 * @param p 写入的首地址
 * @param val 待写入的64位数据
 */
void write_uint64(char *p, __uint64_t val)
{
    for (int i = 0; i < 8; i++)
    {
        *(p + i) = val & 0xff;
        val >>= 8;
    }
}

/**
 * @brief 初始化新的ogg页
 * @param op 待初始化的ogg页结构体
 */
void init_ogg_page(ogg_page &op)
{
    // op.header = new char[27 + 255];
    // op.body = new char[255 * 255];
    memset(op.header, 0, (27 + 255));
    memset(op.body, 0, (255 * 255));
    op.header_length = 27;
    op.body_length = 0;
}

/**
 * @brief 初始化新的ogg逻辑流
 * @param os 待初始化的ogg逻辑流结构体
 */
void init_ogg_logic_stream(ogg_logic_stream &os)
{
    // default, the first stream is the begin of (logic) stream
    os.page_flag = 0x02;
    // pcm position = 0
    os.granule_position = 0;
    // a random number, differ from other ogg_stream
    srand(time(NULL));
    os.serial_number = rand();
    // page counter starts with 0
    os.page_counter = 0;
}

/**
 * @brief 将opus的id头放入单独的一个页
 * @param op ogg页
 * @param id_header opus的id头信息
 */
void ogg_page_put_id_header(ogg_page &op, const opus_id_header id_header)
{
    /* page header */
    // segment_number
    op.header[26] = 1;
    // segment tabel, assign in later
    // op.header[27] = op.body_length;
    op.header_length++;

    /* page body */
    int pos = 0;
    // magic signature
    memcpy(op.body + pos, "OpusHead", 8);
    pos += 8;
    // version number
    op.body[pos++] = id_header.version;
    // channel count
    op.body[pos++] = id_header.channel;
    // pre skip
    write_uint16(op.body + pos, id_header.pre_skip);
    pos += 2;
    // input sample rate
    write_uint32(op.body + pos, id_header.sample_rate);
    pos += 4;
    // output gain
    write_uint16(op.body + pos, id_header.gain);
    pos += 2;
    // channel mapping family
    op.body[pos++] = id_header.channel_mapping_family;

    // update body_length of page
    op.body_length += pos;
    // assign segment tabel
    op.header[27] = op.body_length;
}

/**
 * @brief 将opus的comment头放入单独的一个页
 * @param op ogg页
 * @param comment_header opus的comment头信息
 */
void ogg_page_put_comment_header(ogg_page &op, const opus_comment_header comment_header)
{
    /* page header */
    // segment_number
    op.header[26] = 1;
    // segment tabel, assign in later
    // op.header[27] = op.body_length;
    op.header_length++;

    /* page body */
    int pos = 0;
    // magic signature
    memcpy(op.body + pos, "OpusTags", 8);
    pos += 8;
    // encoder_info length
    write_uint32(op.body + pos, comment_header.encoder_info_length);
    pos += 4;
    // encoder_info
    memcpy(op.body + pos, comment_header.encoder_info, comment_header.encoder_info_length);
    pos += comment_header.encoder_info_length;
    // comment_number
    write_uint32(op.body + pos, comment_header.comment_number);
    pos += 4;
    for (int i = 0; i < comment_header.comment_number; i++)
    {
        int comment_length = strlen(comment_header.comments[i]);
        write_uint32(op.body + pos, comment_length);
        pos += 4;
        memcpy(op.body + pos, comment_header.comments[i], comment_length);
        pos += comment_length;
    }

    // update body_length of page
    op.body_length += pos;
    // assign segment tabel
    op.header[27] = op.body_length;
}

/**
 * @brief 将opus编码数据放入ogg页
 * @parma os ogg逻辑流
 * @param op ogg页
 * @param data opus编码数据
 * @param data_length opus编码数据长度
 * @retrun 成功时返回0，该页存放的段大于最大段长时返回错误-1
 */
int ogg_page_put_packet(ogg_logic_stream &os, ogg_page &op, const char *data, const __uint8_t data_length)
{
    __uint8_t filled_segments = __uint8_t(op.header[26]);
    if (filled_segments >= MAX_SEGMENTS)
        return -1;
    // add a new segment
    op.header[26] = ++filled_segments;
    op.header[26 + filled_segments] = data_length;
    memcpy(op.body + op.body_length, data, data_length);

    op.header_length++;
    op.body_length += data_length;

    // update the logic stream state
    // The duration of an Opus packet may be any multiple of 2.5 ms, up to a maximum of 120 ms.
    // This duration is encoded in the TOC sequence at the beginning of each packet.
    // The number of samples returned by a decoder corresponds to this duration exactly, even for the first few packets.
    // For example, a 20 ms packet fed to a decoder running at 48 kHz will always return 960 samples.
    os.granule_position += 960;

    return 0;
}

/**
 * @brief 对ogg页的头部和数据体进行crc校验
 * @parma op ogg页
 */
void ogg_page_crc_checksum(ogg_page &op)
{
    __uint32_t crc_reg = 0;

    /* safety; needed for API behavior, but not framing code */
    op.header[22] = 0;
    op.header[23] = 0;
    op.header[24] = 0;
    op.header[25] = 0;

    // must be unsigned char, error if char
    for (int i = 0; i < op.header_length; i++)
        crc_reg = (crc_reg << 8) ^ crc_lookup[((crc_reg >> 24) & 0xff) ^ (unsigned char)op.header[i]];
    for (int i = 0; i < op.body_length; i++)
        crc_reg = (crc_reg << 8) ^ crc_lookup[((crc_reg >> 24) & 0xff) ^ (unsigned char)op.body[i]];

    op.header[22] = (unsigned char)(crc_reg & 0xff);
    op.header[23] = (unsigned char)((crc_reg >> 8) & 0xff);
    op.header[24] = (unsigned char)((crc_reg >> 16) & 0xff);
    op.header[25] = (unsigned char)((crc_reg >> 24) & 0xff);
}

/**
 * @brief 对ogg页进行封装，完善头信息
 * @param os ogg逻辑流
 * @param op ogg页
 */
void ogg_page_encapsulate(ogg_logic_stream &os, ogg_page &op)
{
    // 32 bits, capture pattern
    memcpy(op.header, "OggS", 4);
    // 8 bits, version
    op.header[4] = 0x00;
    // 8 bits, header type. continued page? first page? last page?
    op.header[5] = os.page_flag;
    // 64 bits, PCM position
    write_uint64(op.header + 6, os.granule_position);
    // 32 bits, serial number
    write_uint32(op.header + 14, os.serial_number);
    // 32 bits, page counter
    write_uint32(op.header + 18, os.page_counter);
    // 32 bits, crc checksum, assign in <func ogg_page_crc_checksum>
    // 8 bits, segment number, assign in <func ogg_page_put_packet>
    // (8 bits * segment number), segment table, assign in <func ogg_page_put_packet>
    // crc checksum, check header and body of the page
    ogg_page_crc_checksum(op);

    // update ogg_logic_stream data
    os.page_counter++;
}

/**
 * @brief 对opus进行ogg封装示例
 * @param src opus压缩数据文件路径（16kHz, 16bit/sample, 1channel）
 * @param dest ogg文件保存路径
 */
void ogg_opus_example(char *src, char *dest)
{
    FILE *fin = fopen(src, "rb");
    if (NULL == fin)
    {
        fprintf(stderr, "[ERROR] Failed to open file %s\n", src);
        exit(1);
    }
    FILE *fout = fopen(dest, "wb");

    ogg_logic_stream os;
    ogg_page op;
    init_ogg_logic_stream(os);
    // init_ogg_page(op);

    // send page of opushead
    init_ogg_page(op);
    opus_id_header id_header{1, 1, 312, 16000, 0, 0};
    ogg_page_put_id_header(op, id_header);
    ogg_page_encapsulate(os, op);
    fwrite(op.header, 1, op.header_length, fout);
    fwrite(op.body, 1, op.body_length, fout);
    os.page_flag = 0x00;

    // send page of opustags
    init_ogg_page(op);
    char *encoder_info = (char *)"libopus 1.3.1";
    char *comments[] = {
        (char *)"ARTIST=iflytek",
        (char *)"TITLE=zghong"};
    opus_comment_header comment_header{(__uint32_t)strlen(encoder_info), encoder_info, 2, comments};
    ogg_page_put_comment_header(op, comment_header);
    ogg_page_encapsulate(os, op);
    fwrite(op.header, 1, op.header_length, fout);
    fwrite(op.body, 1, op.body_length, fout);

    // send page of data
    char opus[60];
    int size;
    init_ogg_page(op);
    while (!feof(fin))
    {
        // 60 Bytes, one frame
        if ((size = fread(opus, 1, 60, fin)) == 0)
        {
            break;
        }
        if (ogg_page_put_packet(os, op, opus, size) == 0)
        {
            continue;
        }
        // send the page to server
        ogg_page_encapsulate(os, op);
        fwrite(op.header, 1, op.header_length, fout);
        fwrite(op.body, 1, op.body_length, fout);
        // init a new page
        init_ogg_page(op);
        ogg_page_put_packet(os, op, opus, 60);
    }
    os.page_flag = 0x04;
    ogg_page_encapsulate(os, op);
    fwrite(op.header, 1, op.header_length, fout);
    fwrite(op.body, 1, op.body_length, fout);
    fclose(fin);
    fclose(fout);
}

#endif