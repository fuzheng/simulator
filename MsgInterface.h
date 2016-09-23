#pragma once

#include <string>
#include <muduo/base/Logging.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <sstream>
using namespace std;

struct BaseHead
{
	uint16_t msgLen;
	uint16_t msgCmd;

	BaseHead(uint16_t wLen, uint16_t wType)
	{
		msgLen = htons(wLen);
		msgCmd = htons(wType);
	}
};

class MsgInterface
{
public:
	MsgInterface(void);
	~MsgInterface(void);
	MsgInterface(uint16_t cmd, string fileName);
	
protected:
	void AddMsgHead();
	bool ReadStringFromFile();
	virtual bool UnpackMsgFromString() = 0;
	virtual bool PackMsgtoString() = 0;
protected:
	//中间json串
	string msg;
	//jsonMsg串
	string jsonMsg;
	//最终的消息流
	string buffer;
	//消息的命令字
	uint16_t msgCmd;
	//读取的msg文件名
	string fileName;
public:
	//对外提供的接口
	string GetMsg()
	{
		
		return buffer;
	}

	bool PackMsgFromString();

};

const uint16_t kMsgLen = 1024 * 5;
const uint16_t kMsgSeqLen = 128;
const uint16_t kDevTokenLen = 128;
const uint16_t kUuidLen = 128;
const uint16_t kClientIdLen = 128;
const uint16_t kConnLen = 32;
const uint16_t kPlanIdLen = 128;
const uint16_t kVerLen = 32;

class SenderMsg : public MsgInterface
{

	struct SendMsg 
	{
		uint16_t msgServNum;
		uint16_t devType;
		uint16_t msgType;
		uint16_t stat;
		uint32_t retryCnt;
		uint32_t fd;
		uint32_t appID;
		uint32_t sendCnt;
		uint64_t msgID;
		uint64_t expireTime;
		uint64_t sendTime;
		uint64_t recvTime;
		char clientID[kClientIdLen];
		char devToken[kDevTokenLen];
		char msgSeq[kMsgSeqLen];
		char msg[kMsgLen];
		int32_t bizType;
		char planId[kPlanIdLen];
		int32_t devCategory;
		int32_t devSrc;
		char appVer[kVerLen];

		string ToString()
		{
			std::stringstream tmp;
			tmp << "devType: " << devType << " msgType: " << msgType << " appID is: " << appID << " msgID: " << msgID;
			string result;
			result = tmp.str();
			return result;
		}
		
	};

public:
	SenderMsg(uint16_t cmd, string fileName) : MsgInterface(cmd, fileName)
	{
		
	}
private:
	//中间结构体，存储消息使用
	SendMsg pushMsg;

protected:
	virtual bool UnpackMsgFromString();
	virtual bool PackMsgtoString();

};


class Msgserver : public MsgInterface
{

	struct SendMsg 
	{
		uint16_t msgServNum;
		uint16_t devType;
		uint16_t msgType;
		uint16_t stat;
		uint32_t retryCnt;
		uint32_t fd;
		uint32_t appID;
		uint32_t sendCnt;
		uint64_t msgID;
		uint64_t expireTime;
		uint64_t sendTime;
		uint64_t recvTime;
		char clientID[kClientIdLen];
		char devToken[kDevTokenLen];
		char msgSeq[kMsgSeqLen];
		char msg[kMsgLen];
		int32_t bizType;
		char planId[kPlanIdLen];
		int32_t devCategory;
		int32_t devSrc;
		char appVer[kVerLen];

		string ToString()
		{
			std::stringstream tmp;
			tmp << "devType: " << devType << " msgType: " << msgType << " appID is: " << appID << " msgID: " << msgID;
			string result;
			result = tmp.str();
			return result;
		}

	};

public:
	Msgserver(uint16_t cmd, string fileName) : MsgInterface(cmd, fileName)
	{

	}
private:
	//中间结构体，存储消息使用
	SendMsg pushMsg;

protected:
	virtual bool UnpackMsgFromString();
	virtual bool PackMsgtoString();

};