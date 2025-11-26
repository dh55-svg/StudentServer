#pragma once
#include <stdint.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <iostream>
#include <cstring>
#include "json.hpp"
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif
using namespace std;
using json = nlohmann::json;
const uint32_t MY_PROTO_MAX_SIZE = 1024 * 1024*10;//10M协议中数据最大
const uint32_t MY_PROTO_HEAD_SIZE = 14;//协议头大小
extern const uint16_t CRC_INITIAL_VALUE;
extern const uint16_t CRC_POLYNOMIAL;    // CRC多项式
// 协议头字段偏移量常量
extern const uint32_t VERSION_OFFSET;   // 版本字段偏移量
extern const uint32_t SERVER_OFFSET;    // 服务号字段偏移量
extern const uint32_t LEN_OFFSET;       // 长度字段偏移量
extern const uint32_t CRC_OFFSET;       // CRC字段偏移量
extern const uint32_t SEQUENCE_OFFSET;  // 序列号字段偏移量
extern const uint32_t TYPE_OFFSET;      // 消息类型字段偏移量
typedef enum MyProtoParserStatus
{
	ON_PARSER_INIT=0,
	ON_PARSER_HEAD,
	ON_PARSER_BODY,
}MyProtoParserStatus;
typedef enum MyProtoMsgType
{
	MY_PROTO_TYPE_DATA = 0,        // 数据消息
	MY_PROTO_TYPE_ACK = 1,         // 确认消息
	MY_PROTO_TYPE_HEARTBEAT = 2,   // 心跳请求消息
	MY_PROTO_TYPE_HEARTBEAT_ACK = 3 // 心跳响应消息
}MyProtoMsgType;
#pragma pack(push, 1)
struct MyProtoHead
{
	uint8_t version; //协议版本号
	uint16_t server; //协议复用的服务号
	uint32_t len; //协议长度
	uint16_t crc; //CRC校验值
	uint32_t sequence; //协议序列号
	uint8_t type; //协议类型 0-数据 1-确认消息 2-心跳请求 3-心跳响应
};
#pragma pack(pop)
struct MyProtoMsg
{
	MyProtoHead head;
	json body;
};
// 增加CRC计算函数声明
uint16_t calculateCRC(const uint8_t* data, size_t length);
bool validateJsonContent(const json& j);
//公共函数
//打印协议数据信息
void printMyProtoMsg(MyProtoMsg& msg);
//协议封装类
class MyProtoEncode
{
public:
	//协议消息体封装函数：传入的pMsg里面只有部分数据，比如Json协议体，服务号，我们对消息编码后会修改长度信息，这时需要重新编码协议
	uint8_t* encode(MyProtoMsg* pMsg, uint32_t& len); //返回长度信息，用于后面socket发送数据
private:
	//协议头封装函数
	void headEncode(uint8_t* pData, MyProtoMsg* pMsg);
};
class MyProtoDecode
{
private:
	MyProtoMsg mCurMsg;
	queue<shared_ptr<MyProtoMsg>> mMsgQ;//解析好的协议消息队列
	vector<uint8_t> mCurReserved; //未解析的网络字节流，可以缓存所有没有解析的数据（按字节）
	MyProtoParserStatus mCurParserStatus; //当前接受方解析状态
public:
	void init();
	void clear();//清空解析好的消息队列
	bool empty();//判断解析好的消息队列是否为空
	void pop();
	shared_ptr<MyProtoMsg> front();//获取一个解析好的消息
	bool parser(void *data,size_t len);//从网络字节流中解析出来协议消息，len是网络中的字节流长度，通过socket可以获取
private:
	bool parserHead(uint8_t** curData,uint32_t& curLen, uint32_t& parserLen, bool& parserBreak);
	bool parserBody(uint8_t** curData, uint32_t& curLen,
		uint32_t& parserLen, bool& parserBreak); //用于解析消息体
};

