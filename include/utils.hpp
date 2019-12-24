/**
 * @Copyright: https://www.xfyun.cn/
 * 
 * @Author: iflytek
 * 
 * @Data: 2019-12-20
 * 
 * 本文件包含“讯飞开放平台”的WebAPI接口Demo相关工具类函数实现
 */

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <string>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

/**
 * 获取当前时间戳，并格式化成rfc1123格式
 * 输入：
 * 输出：当前时间戳的rfc1123格式字符串
 */
std::string get_time_rfc1123()
{
	time_t rawtime = time(NULL);
	char date[64];
	strftime(date, sizeof(date), "%a, %d %b %Y %X %Z", gmtime(&rawtime));

	return std::string(date);
}

/**
 * hmac_sha256算法，利用key对data进行加密认证处理
 * 输入：data，待hmac_sha256的数据；key，mac的密钥
 * 输出：256位加密字符，为方便处理，输出时转换为string
 */
std::string get_hmac_sha256(const std::string &data, const std::string &key)
{
	unsigned char *result = new unsigned char[40];
	unsigned int result_len = 0;

	HMAC_CTX *ctx = HMAC_CTX_new();
	HMAC_Init_ex(ctx, key.c_str(), key.size(), EVP_sha256(), NULL);
	HMAC_Update(ctx, (unsigned char *)data.c_str(), data.size());
	HMAC_Final(ctx, result, &result_len);
	HMAC_CTX_free(ctx);

	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	// 不可以return string(result)，string的构造函数不接受unsigned char *
	// 不可以直接 return string((char *)result)
	// 因为result后面还有空余字符，加密的值要严格控制在result_len的长度范围内
	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	return std::string((char *)result, result_len);
}

/**
 * base64编码算法
 * 输入：data，待编码数据
 * 输出：data被base64编码后的字符串
 */
std::string get_base64_encode(const std::string &data)
{
	typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8>> Base64EncodeIterator;

	std::stringstream result;

	std::copy(Base64EncodeIterator(data.begin()), Base64EncodeIterator(data.end()), std::ostream_iterator<char>(result));
	size_t equal_count = (3 - data.length() % 3) % 3;
	for (size_t i = 0; i < equal_count; i++)
	{
		result.put('=');
	}

	return result.str();
}

/**
 * base64解码算法
 * 输入：data，待解码数据
 * 输出：data被base64解码后的字符串
 */
std::string get_base64_decode(const std::string &data)
{
	typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6> Base64DecodeIterator;

	std::stringstream result;

	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	// Base64编码原理是把3字节的二进制数据编码为4字节的文本数据，长度增加33%。
	// 如果要编码的二进制数据不是3的倍数，会在最后剩下1个或2个字节用\x00字节在末尾补足，然后在编码的末尾加上1个或2个=号。
	// 所以在Base64解码中会将=号解码为\0，这对文本无影响，但是对音频有影响，所以需要手动将=去除再解码。
	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	std::string temp = data;
	while (temp[temp.size() - 1] == '=')
	{
		temp.erase(temp.size() - 1);
	}

	std::copy(Base64DecodeIterator(temp.begin()), Base64DecodeIterator(temp.end()), std::ostream_iterator<char>(result));

	return result.str();
}

/**
 * url编码算法，转义url中的特殊字符和中文字符等
 * 输入：url，待编码的url
 * 输出：被编码转义后的url
 */
std::string get_url_encode(const std::string &url)
{
	typedef unsigned char BYTE;
	std::string result;
	size_t len = url.length();
	unsigned char *pBytes = (unsigned char *)url.c_str();
	char szAlnum[2];
	char szOther[4];
	for (size_t i = 0; i < len; i++)
	{
		// 由于迅飞在解码url时以'='获得参数值，以'&'分割参数，故这两个参数不可转义
		if (isalnum((BYTE)url[i]) || url[i] == '&' || url[i] == '=')
		{
			sprintf(szAlnum, "%c", url[i]);
			result.append(szAlnum);
		}
		else
		{
			sprintf(szOther, "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
			result.append(szOther);
		}
	}
	return result;
}

/**
 * 延迟函数，因为Windows和Linux下的延迟函数各异，开发者可以自定义
 * 输入：t，延迟秒数
 * 输出：
 */
void delay(double t)
{
	clock_t start_time = clock();
	while ((clock() - start_time) < t * CLOCKS_PER_SEC)
	{
	}
}

/**
 * md5算法，对data进行md5认证处理
 * 输入：data，待md5的数据
 * 输出：128位字符的散列值，此处不可简单转成string，讯飞要求md5后为小写十六进制数据
 */
std::string get_md5(const std::string &data)
{
	unsigned char *result = new unsigned char[20];

	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, data.c_str(), data.size());
	MD5_Final(result, &ctx);

	char *buf = new char[32];
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
	{
		sprintf(buf + 2 * i, "%02x", result[i]);
	}

	return std::string(buf, MD5_DIGEST_LENGTH * 2);
}

/**
 * hmac_sha1算法，利用key对data进行加密认证处理
 * 输入：data，待hmac_sha1的数据；key，mac的密钥
 * 输出：160位加密字符，为方便处理，输出时转换为string
 */
std::string get_hmac_sha1(const std::string &data, const std::string &key)
{
	unsigned char *result = new unsigned char[25];
	unsigned int result_len = 0;

	HMAC_CTX *ctx = HMAC_CTX_new();
	HMAC_Init_ex(ctx, key.c_str(), key.size(), EVP_sha1(), NULL);
	HMAC_Update(ctx, (unsigned char *)data.c_str(), data.size());
	HMAC_Final(ctx, result, &result_len);
	HMAC_CTX_free(ctx);

	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	// 不可以return string(result)，string的构造函数不接受unsigned char *
	// 不可以直接 return string(char *)result)
	// 因为result后面还有空余字符，加密的值要严格控制在result_len的长度范围内
	// ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	return std::string((char *)result, result_len);
}

#endif