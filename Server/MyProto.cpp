#include "MyProto.h"
#include <iostream>
#include <stdlib.h>
#include <iomanip>
// Windows平台特定头文件
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif
using namespace std;

// 定义CRC相关常量
const uint16_t CRC_INITIAL_VALUE = 0xFFFF; // CRC-16/CCITT-FALSE初始值
const uint16_t CRC_POLYNOMIAL = 0x1021;    // CRC-16/CCITT-FALSE多项式

// 定义协议头字段偏移量常量
const uint32_t VERSION_OFFSET = 0;    // 版本字段偏移量
const uint32_t SERVER_OFFSET = 1;     // 服务号字段偏移量
const uint32_t LEN_OFFSET = 3;        // 长度字段偏移量
const uint32_t CRC_OFFSET = 7;        // CRC字段偏移量
const uint32_t SEQUENCE_OFFSET = 9;   // 序列号字段偏移量
const uint32_t TYPE_OFFSET = 13;      // 消息类型字段偏移量

// 添加CRC计算函数实现
// 这里使用CRC-16/CCITT-FALSE算法
uint16_t calculateCRC(const uint8_t* data, size_t length) {
    uint16_t crc = CRC_INITIAL_VALUE; // 初始值
    uint16_t polynomial = CRC_POLYNOMIAL; // 多项式

    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)(data[i] << 8);
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ polynomial;
            else
                crc <<= 1;
        }
    }

    return crc;
}
//----------------------------------公共函数----------------------------------
//打印协议数据信息
void printMyProtoMsg(MyProtoMsg& msg)
{
    string jsonStr = msg.body.dump(2);

    // 移除对已删除的magic字段的引用
    printf("Head[version=%d,server=%d,len=%d]\n"
        "Body:%s", msg.head.version, msg.head.server,
        msg.head.len, jsonStr.c_str());
}

//----------------------------------协议头封装函数----------------------------------
//pData指向一个新的内存，需要pMsg中数据对pData进行填充
void MyProtoEncode::headEncode(uint8_t* pData, MyProtoMsg* pMsg) {
    // version - 1字节
    *(pData + VERSION_OFFSET) = pMsg->head.version;

    // server - 2字节
    *(uint16_t*)(pData + SERVER_OFFSET) = htons(pMsg->head.server);

    // len - 4字节
    *(uint32_t*)(pData + LEN_OFFSET) = htonl(pMsg->head.len);

    // crc - 2字节 (暂时置0，后面会填充)
    *(uint16_t*)(pData + CRC_OFFSET) = 0;

    // sequence - 4字节
    *(uint32_t*)(pData + SEQUENCE_OFFSET) = htonl(pMsg->head.sequence);

    // type - 1字节
    *(pData + TYPE_OFFSET) = pMsg->head.type;
}

//协议消息体封装函数：传入的pMsg里面只有部分数据，比如Json协议体，服务号，版本号，我们对消息编码后会修改长度信息，这时需要重新编码协议
//len返回长度信息，用于后面socket发送数据
uint8_t* MyProtoEncode::encode(MyProtoMsg* pMsg, uint32_t& len)
{
    uint8_t* pData = NULL;
    string bodyStr = pMsg->body.dump();

    // 计算消息序列化以后的新长度
    len = MY_PROTO_HEAD_SIZE + (uint32_t)bodyStr.size();
    pMsg->head.len = len;

    // 申请内存
    pData = new uint8_t[len];

    // 编码协议头
    headEncode(pData, pMsg);

    // 打包协议体
    memcpy(pData + MY_PROTO_HEAD_SIZE, bodyStr.data(), bodyStr.size());

    // 计算并填充CRC值
    uint16_t crc = calculateCRC(pData, len);
    // 直接将主机字节序的CRC值写入到CRC_OFFSET位置
    *(uint16_t*)(pData + CRC_OFFSET) = crc;

    return pData;
}


//----------------------------------协议解析类----------------------------------
//初始化协议解析状态
void MyProtoDecode::init()
{
    mCurParserStatus = ON_PARSER_INIT;
}

//清空解析好的消息队列
void MyProtoDecode::clear()
{
    while (!mMsgQ.empty())
    {
        mMsgQ.pop();
    }
}

//判断解析好的消息队列是否为空
bool MyProtoDecode::empty() {
    return mMsgQ.empty();
}

//出队一个消息
void MyProtoDecode::pop()
{
    mMsgQ.pop();
}

//获取一个解析好的消息
std::shared_ptr<MyProtoMsg> MyProtoDecode::front()
{
    return mMsgQ.front();
}

//从网络字节流中解析出来协议消息,len由socket函数recv返回
// 修复parser方法中的消息边界处理逻辑
bool MyProtoDecode::parser(void* data, size_t len) {
    try {
        if (len <= 0)
            return false;

        uint32_t curLen = 0; // 用于保存未解析的网络字节流长度
        uint32_t parserLen = 0; // 保存vector中已经被解析完成的字节流
        uint8_t* curData = NULL; // 指向data,当前未解析的网络字节流

        curData = (uint8_t*)data;

        // 将当前要解析的网络字节流写入到vector中    
        while (len--) {
            mCurReserved.push_back(*curData);
            ++curData;
        }

        curLen = mCurReserved.size();
        curData = (uint8_t*)&mCurReserved[0]; // 获取数据首地址

        // 只要还有未解析的网络字节流，就持续解析
        while (curLen > 0) {
            bool parserBreak = false;//用于标记解析过程是否需要中断等待更多数据
            parserLen = 0; // 重置已解析长度

            // 解析头部
            if (ON_PARSER_INIT == mCurParserStatus) {
                if (!parserHead(&curData, curLen, parserLen, parserBreak)) {
                    // 解析头部失败，清理已解析的数据
                    if (parserLen > 0) {
                        mCurReserved.erase(mCurReserved.begin(), mCurReserved.begin() + parserLen);
                    }
                    return false;
                }
                if (parserBreak) {
                    break; // 退出循环，等待下一次数据到达
                }
            }

            // 解析完成协议头，开始解析协议体
            if (ON_PARSER_HEAD == mCurParserStatus) {
                if (!parserBody(&curData, curLen, parserLen, parserBreak)) {
                    // 解析体部失败，清理已解析的数据
                    if (parserLen > 0) {
                        mCurReserved.erase(mCurReserved.begin(), mCurReserved.begin() + parserLen);
                    }
                    return false;
                }
                if (parserBreak) {
                    break;
                }
                // 成功解析完消息体后，设置状态为解析完成
                mCurParserStatus = ON_PARSER_BODY;
            }

            // 如果成功解析了消息，就把他放入消息队列
            if (ON_PARSER_BODY == mCurParserStatus) {
                std::shared_ptr<MyProtoMsg> pMsg = std::make_shared<MyProtoMsg>();
                *pMsg = mCurMsg;
                mMsgQ.push(pMsg);

                // 重置解析状态，准备解析下一条消息
                mCurParserStatus = ON_PARSER_INIT;


                // 计算已解析的总长度并从缓冲区移除
                uint32_t totalParsed = MY_PROTO_HEAD_SIZE + (mCurMsg.head.len - MY_PROTO_HEAD_SIZE);
                if (totalParsed <= mCurReserved.size()) {
                    mCurReserved.erase(mCurReserved.begin(), mCurReserved.begin() + totalParsed);
                }

                // 更新剩余数据
                curLen = mCurReserved.size();
                curData = (curLen > 0) ? (uint8_t*)&mCurReserved[0] : NULL;
            }
        }
    }
    catch (const std::exception& e) {
        // 记录异常并重置解析状态，防止程序崩溃
        cerr << "Parser exception: " << e.what() << endl;
        init(); // 重置解析状态
        return false;
    }
    catch (...) {
        cerr << "Unknown parser exception" << endl;
        init();
        return false;
    }
    return true;
}

// 用于解析消息头
bool MyProtoDecode::parserHead(uint8_t** curData, uint32_t& curLen, uint32_t& parserLen, bool& parserBreak) {
    // 检查当前数据长度是否足够解析头部
    if (curLen < MY_PROTO_HEAD_SIZE) {
        cout << "[DEBUG] Not enough data for header, need " << MY_PROTO_HEAD_SIZE << ", have " << curLen << endl;
        parserBreak = true;
        return true;
    }

    // 获取当前解析位置
    uint8_t* pData = *curData;

    // 添加调试日志，打印原始字节数据
    cout << "[DEBUG] Raw header bytes (first 14 bytes): ";
    for (uint32_t i = 0; i < MY_PROTO_HEAD_SIZE && i < curLen; i++) {
        printf("%02X ", pData[i]);
    }
    cout << endl;

    // 解析版本号（头部第一个字节）
    mCurMsg.head.version = pData[VERSION_OFFSET];

    // 验证版本号是否支持
    if (mCurMsg.head.version != 0 && mCurMsg.head.version != 1) {
        cerr << "Unsupported protocol version: " << static_cast<int>(mCurMsg.head.version) << endl;
        return false;
    }

    // 解析服务号（接下来的两个字节）
    mCurMsg.head.server = ntohs(*(uint16_t*)(pData + SERVER_OFFSET));
    cout << "[DEBUG] Parsed server: " << mCurMsg.head.server << endl;

    // 解析协议消息体长度（接下来的四个字节）
    mCurMsg.head.len = ntohl(*(uint32_t*)(pData + LEN_OFFSET));

    // 解析CRC校验值（接下来的两个字节）
    mCurMsg.head.crc = *(uint16_t*)(pData + CRC_OFFSET);

    // 解析序列号（接下来的四个字节）
    mCurMsg.head.sequence = ntohl(*(uint32_t*)(pData + SEQUENCE_OFFSET));
    cout << "[DEBUG] Parsed sequence: " << mCurMsg.head.sequence << endl;

    // 解析消息类型（最后一个字节）
    mCurMsg.head.type = pData[TYPE_OFFSET];
    cout << "[DEBUG] Parsed type: " << static_cast<int>(mCurMsg.head.type) << " (0x" << hex << static_cast<int>(mCurMsg.head.type) << dec << ")" << endl;

    // 合并消息类型验证逻辑，允许更多类型值
    if (mCurMsg.head.type != 0 && mCurMsg.head.type != 1) {
        cout << "[WARNING] Non-standard message type: " << static_cast<int>(mCurMsg.head.type) << ", but continuing processing" << endl;
        // 不再返回false，而是继续处理消息
    }

    // 判断数据长度是否超过指定的最大大小，防止缓冲区溢出
    if (mCurMsg.head.len > MY_PROTO_MAX_SIZE) {
        cerr << "Message length exceeds maximum size: " << mCurMsg.head.len << endl;
        return false;
    }

    // 验证消息长度是否合法（至少包含头部）
    if (mCurMsg.head.len < MY_PROTO_HEAD_SIZE) {
        cerr << "Invalid message length: " << mCurMsg.head.len << endl;
        return false;
    }

    // 更新解析指针位置，移动到头部之后的下一个位置
    *curData = pData + MY_PROTO_HEAD_SIZE;
    curLen -= MY_PROTO_HEAD_SIZE;
    parserLen += MY_PROTO_HEAD_SIZE;

    // 设置解析状态为解析头部完成
    mCurParserStatus = ON_PARSER_HEAD;

    return true;
}

// 用于解析消息体
bool MyProtoDecode::parserBody(uint8_t** curData, uint32_t& curLen, uint32_t& parserLen, bool& parserBreak) {
    // 获取消息体长度
    uint32_t bodyLen = mCurMsg.head.len - MY_PROTO_HEAD_SIZE;

    // 检查当前数据长度是否足够解析消息体
    if (curLen < bodyLen) {
        parserBreak = true;
        return true;
    }

    try {
        // 将消息体内容解析为JSON
        string bodyStr((char*)*curData, bodyLen);

        // 验证bodyStr是否为空
        if (bodyStr.empty()) {
            mCurMsg.body = json::object(); // 空JSON对象
        }
        else {
            mCurMsg.body = json::parse(bodyStr);
        }

        // 验证JSON内容
        if (!validateJsonContent(mCurMsg.body)) {
            cerr << "Invalid JSON content in message body" << endl;
            return false;
        }

        // 完整的CRC校验实现
       // 1. 获取原始消息起始位置
        // 计算从保留缓冲区开始到当前数据的偏移量
        uint32_t offsetToCurrentData = mCurReserved.size() - curLen;
        uint8_t* originalMsgStart = (uint8_t*)&mCurReserved[0] + offsetToCurrentData - MY_PROTO_HEAD_SIZE;
        uint32_t totalMsgLen = MY_PROTO_HEAD_SIZE + bodyLen;
        // 2. 保存原始CRC值
        uint16_t originalCRC = *(uint16_t*)(originalMsgStart + CRC_OFFSET);
        // 3. 创建一个临时副本并将CRC字段置0
        vector<uint8_t> tempMsgData(originalMsgStart, originalMsgStart + totalMsgLen);
        memset(&tempMsgData[CRC_OFFSET], 0, sizeof(uint16_t)); // 将CRC字段置0

        // 4. 计算CRC
        uint16_t calculatedCRC = calculateCRC(tempMsgData.data(), totalMsgLen);

        // 5. 验证CRC
        if (calculatedCRC != originalCRC) {
            cerr << "CRC check failed! Expected: " << calculatedCRC
                << " (0x" << hex << calculatedCRC << dec << ")"
                << ", Received: " << originalCRC
                << " (0x" << hex << originalCRC << dec << ")" << endl;

            // 添加调试信息：打印消息头前几个字节
            cerr << "[DEBUG] First 16 bytes of message: ";
            for (int i = 0; i < min(16, (int)totalMsgLen); i++) {
                cerr << hex << setw(2) << setfill('0') << (int)originalMsgStart[i] << " ";
            }
            cerr << dec << endl;

            return false;
        }
        cout << "[DEBUG] CRC check passed. Calculated: " << calculatedCRC << ", Original: " << originalCRC << endl;
        // 如果是数据消息（type=0），则设置解析状态为解析完成
        if (mCurMsg.head.type == 0) {
            mCurParserStatus = ON_PARSER_BODY;
        }

        // 更新解析指针位置，移动到消息体之后的下一个位置
        (*curData) += bodyLen;
        // 更新剩余数据长度
        curLen -= bodyLen;
        // 更新已解析的数据长度
        parserLen += bodyLen;

        // 返回解析成功
        return true;
    }
    catch (const json::exception& e) {
        cerr << "JSON parse error: " << e.what() << endl;
        return false;
    }
    catch (const std::exception& e) {
        cerr << "Message body parse exception: " << e.what() << endl;
        return false;
    }
}

// 替换结构化绑定为传统的迭代方式
bool validateJsonContent(const json& j) {
    // 基本验证：确保JSON不是空的
    if (j.empty()) {
        cerr << "Empty JSON content" << endl;
        return false;
    }

    // 根据业务需求添加具体的验证规则
    // 这里只是示例，可以根据实际协议要求进行扩展

    // 1. 检查消息类型为数据消息时，是否包含必要字段
    // 注意：这里需要访问MyProtoMsg结构体的type字段，但当前函数无法直接访问
    // 可能需要修改函数参数，或者将此函数改为类成员函数

    // 2. 验证JSON中的数值范围、字符串长度等
    try {
        // 遍历JSON对象，检查是否包含非法值
        for (auto it = j.begin(); it != j.end(); ++it) {
            const string& key = it.key();
            const json& value = it.value();

            // 检查键名是否合法（例如不包含特殊字符）
            if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != string::npos) {
                cerr << "Invalid character in JSON key: " << key << endl;
                return false;
            }

            // 检查字符串值长度是否超过限制
            if (value.is_string() && value.get<string>().length() > 1024) { // 假设1024是最大长度
                cerr << "String value too long for key: " << key << endl;
                return false;
            }
        }
    }
    catch (const std::exception& e) {
        cerr << "JSON validation error: " << e.what() << endl;
        return false;
    }

    return true;
}