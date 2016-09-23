#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sys/prctl.h>
#include <vector>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>

#include <muduo/base/Logging.h>
//#include "Daemon.h"
//#include "InitLog.h"
#include <unistd.h>

#include "MsgIdDefine.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "CommonServer/CommUtil.h"



//g++ main.cc -L/export/newjpush/commonlibs/lib -lmuduo -I/export/newjpush/commonlibs/include -o main  -DMUDUO_STD_STRING  -lpthread -lrt

using namespace std;
using namespace muduo;
using namespace muduo::net;


std::vector<string> vsdt;

string app_ver;
int badge;
//iphone: 1, ipad: 4
string msg = "{\\\"ALERT\\\":\\\"test\\\",\\\"ANDROID_FIELDS\\\":{\\\"0\\\":\\\"containerType\\\",\\\"1\\\":\\\"businessCategoryId\\\",\\\"2\\\":\\\"notifyTemplateId\\\",\\\"3\\\":\\\"msgId\\\",\\\"4\\\":\\\"landPageUrl\\\",\\\"5\\\":\\\"isNewService\\\",\\\"6\\\":\\\"shareFlag\\\",\\\"7\\\":\\\"landPageId\\\",\\\"8\\\":\\\"msgId\\\"},\\\"EXTRAS\\\":{\\\"businessCategoryId\\\":\\\"7\\\",\\\"containerType\\\":\\\"6\\\",\\\"isNewService\\\":\\\"1\\\",\\\"landPageId\\\":\\\"1\\\",\\\"landPageUrl\\\":\\\"http://sale.jd.com/app/act/rQ3Sv0GEHkx.html\\\",\\\"msgId\\\":\\\"11_5715a24ec500057d8edb1198\\\",\\\"notifyTemplateId\\\":\\\"1\\\",\\\"shareFlag\\\":\\\"0\\\"},\\\"IOS_FIELDS\\\":{\\\"0\\\":\\\"businessCategoryId\\\",\\\"1\\\":\\\"msgId\\\",\\\"2\\\":\\\"landPageUrl\\\",\\\"3\\\":\\\"isNewService\\\",\\\"4\\\":\\\"shareFlag\\\",\\\"5\\\":\\\"landPageId\\\",\\\"6\\\":\\\"msgId\\\"},\\\"IOS_PUSH_ALL\\\":1,\\\"TITLE\\\":\\\"test\\\"}";

string msgfmt_iphone = "{\"ECHO\":\"{\\\"BIZ_TYPE\\\":-1,\\\"PLANID\\\":\\\"dddddddddddd5bda0db983c6ad7d6g\\\"}\",\"MSGID\":951179,\"APPID\":8,\"DEVTYPE\":1,\"DEVTOKEN\":\"%s\",\"MSG\":\"%s\",\"MSGTYPE\":0,\"MSGSEQ\":\"7a146a6b74bb7ee82d5d2a17edc30ac7d2df949175742ae8b593d46329257d6a_951179\",\"APP_VER\":\"%s\"}";
string msgfmt_ipad = "{\"ECHO\":\"{\\\"BIZ_TYPE\\\":-1,\\\"PLANID\\\":\\\"dddddddddddd5bda0db983c6ad7d6g\\\"}\",\"MSGID\":951179,\"APPID\":8,\"DEVTYPE\":4,\"DEVTOKEN\":\"%s\",\"MSG\":\"%s\",\"MSGTYPE\":0,\"MSGSEQ\":\"7a146a6b74bb7ee82d5d2a17edc30ac7d2df949175742ae8b593d46329257d6a_951179\",\"APP_VER\":\"%s\"}";
string msgfmt_Msgserver = "{\"BIZ_TYPE\":-1,\"PLANID\":\"dddddddddddd5bda0db983c6ad7d6g\",\"MSGID\":951179,\"APPID\":8,\"DEVTYPE\":%d,\"DEVTOKEN\":\"%s\",\"MSG\":\"%s\",\"MSGTYPE\":0,\"MSGSEQ\":\"7a146a6b74bb7ee82d5d2a17edc30ac7d2df949175742ae8b593d46329257d6a_951179\",\"APP_VER\":\"%s\", \"EXPIRETIME\":1483200000, \"DEV_CATEGORY\":%d, \"DEV_SRC\":%d, \"SENDCNT\":1}";
int cnt = 0;


//	PUSH_TYPE_DEVTOKEN = 1, PUSH_TYPE_CLIENTID = 2
string msgfmt_Senderserver = "{\"MSGTYPE\":22, \"PUSHTYPE\":1, \"PLATFORMTYPE\":0, \"APPID\":0, \"MSGID\":1000,\"EXPIRETIME\":1483200000,\"TASK\":\"%s\",\"MSG\":\"%s\", \"OS_VERSION\":\"1.0.0\",\"APP_VERSION\":\"5.0.1\", \"APP_VERSION_OPER\":\"GE\", \"BIZ_TYPE\":-1,\"PLANID\":\"dddddddddddd5bda0db983c6ad7d6g\",\"OS_VERSION_OPER\":\"GE\"}";

const uint16_t kMsgLen = 1024 * 5;
const uint16_t kMsgSeqLen = 128;
const uint16_t kDevTokenLen = 128;
const uint16_t kUuidLen = 128;
const uint16_t kClientIdLen = 128;
const uint16_t kConnLen = 32;
const uint16_t kPlanIdLen = 128;
//add by fuzheng 2016年9月19日, transforming appversion for ios badge
const uint16_t kVerLen = 32;

struct PushMsg
{
	//uint16_t setID;
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
	char msgSeq[kMsgSeqLen];/*uint64_t*/
	char msg[kMsgLen];//
	int32_t bizType;
	char planId[kPlanIdLen];
	//add by fuzheng for xiaomi push
	int32_t devCategory;
	int32_t devSrc;
	//add by fuzheng 2016年9月19日, transforming appversion for ios badge
	char appVer[kVerLen];
};
bool parseSenderMsgPushMsgContentCmd(char *data, size_t size, PushMsg *pushMsg)
{
	rapidjson::Document document;

	if (document.Parse(data).HasParseError())
	{
		LOG_ERROR << "parseSenderMsgPushMsgContentCmd fail, GetParseError = " << document.GetParseError();
		return false;
	}
	else
	{
		{
			LOG_ERROR << "success parse json str";
		}
	}
	if (document.IsObject())
	{
		rapidjson::Value::MemberIterator iter;

		iter = document.FindMember("APPID");
		if (iter != document.MemberEnd())
		{
			pushMsg->appID = iter->value.GetUint();
		}
		else 
		{
			LOG_WARN << "APPID is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("DEVTYPE");
		if (iter != document.MemberEnd())
		{
			pushMsg->devType = iter->value.GetUint();
		}
		else 
		{
			LOG_WARN << "DEVTYPE is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("MSGTYPE");
		if (iter != document.MemberEnd())
		{
			pushMsg->msgType = iter->value.GetUint();
		}
		else 
		{
			LOG_WARN << "MSGTYPE is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("MSGID");
		if (iter != document.MemberEnd())
		{
			pushMsg->msgID = iter->value.GetUint64();
		}
		else 
		{
			LOG_WARN << "MSGID is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("EXPIRETIME");
		if (iter != document.MemberEnd())
		{
			pushMsg->expireTime = iter->value.GetUint64();
		}
		else 
		{
			LOG_WARN << "EXPIRETIME is absent";
			////free(data);
			return false;
		}

		pushMsg->sendCnt = 0;

		iter = document.FindMember("DEVTOKEN");
		if (iter != document.MemberEnd())
		{
			strcpy(pushMsg->devToken, iter->value.GetString());
		}
		else 
		{
			LOG_WARN << "DEVTOKEN is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("MSGSEQ");
		if (iter != document.MemberEnd())
		{
			strcpy(pushMsg->msgSeq, iter->value.GetString());
		}
		else 
		{
			LOG_WARN << "MSGSEQ is absent";
			//free(data);
			return false;
		}

		iter = document.FindMember("MSG");
		if (iter != document.MemberEnd())
		{
			if (strlen(iter->value.GetString()) < kMsgLen)
			{
				strcpy(pushMsg->msg, iter->value.GetString()); 
			}
			else
			{
				LOG_WARN << "the size of msg content is over limit: " << data;
				//free(data);
				return false;
			}

		}
		else 
		{
			LOG_WARN << "MSG is absent";
			//free(data);
			return false;
		}

		//add devCategory for xiaomi push.  fuzheng
		iter = document.FindMember("DEV_CATEGORY");
		if (iter != document.MemberEnd())
		{
			pushMsg->devCategory = iter->value.GetInt();
		}
		else
		{
			LOG_WARN << "DEV_CATEGORY is absend";
			return false;
		}

		iter = document.FindMember("DEV_SRC");
		if (iter != document.MemberEnd())
		{
			pushMsg->devSrc = iter->value.GetInt();
		}
		else
		{
			LOG_WARN << "DEV_SRC is absend";
			return false;
		}
		iter = document.FindMember("SENDCNT");
		if (iter != document.MemberEnd())
		{
			pushMsg->sendCnt = iter->value.GetUint();
		}

		iter = document.FindMember("BIZ_TYPE");
		if (iter != document.MemberEnd())
		{
			pushMsg->bizType= iter->value.GetInt();
		}
		else 
		{
			LOG_WARN << "BIZ_TYPE is absent";
			////free(data);
			return false;
		}

		iter = document.FindMember("PLANID");
		if (iter != document.MemberEnd())
		{
			if (strlen(iter->value.GetString()) < kPlanIdLen)
			{
				strcpy(pushMsg->planId, iter->value.GetString()); 
			}
			else
			{
				LOG_WARN << "the size of planid is over limit: " << data;
				return false;
			}

		}
		else 
		{
			LOG_WARN << "PLANID is absent";
			return false;
		}

		//add by fuzheng 2016年9月19日, transforming appversion for ios badge
		iter = document.FindMember("APP_VER");
		if (iter != document.MemberEnd())
		{
			strcpy(pushMsg->appVer, iter->value.GetString());
		}
		else
		{
			LOG_WARN << "APP_VER is absent";
			////free(data);
			return false;
		}
	}
	//free(data);
	return true;
}


void testRepaidJson()
{
	char buf[10240] = {0};
	std::string dt = "xuwei";
	std::string app_ver = "5.4.1";
	int devCategory = 0;
	int devSrc = 0;
	int len = snprintf(buf, 10240, msgfmt_Msgserver.c_str(), 0, dt.c_str(), msg.c_str(), app_ver.c_str(), devCategory, devSrc);

	cout <<len << buf << endl;

	PushMsg pushMsg;
	parseSenderMsgPushMsgContentCmd(buf, len, &pushMsg);


}
EventLoop loop;

void sendMsgToMsgserver(const TcpConnectionPtr &conn, const std::string& dt, const std::string& app_ver, const int devCategory, const int devSrc, const int devType)
{
	
	char buf[10240] = {0};

	*(uint16_t*)(buf+2) = htons(SENDER_MSG_PUSH_MSG_CONTENT_CMD);
	int len = snprintf(buf+4, 10240, msgfmt_Msgserver.c_str(), devType, dt.c_str(), msg.c_str(), app_ver.c_str(), devCategory, devSrc);
	*(uint16_t*)(buf) = htons(4+len +1);
	string jsonStr(buf);
	cout << "string size is:" << jsonStr.size() << " len is: " << len << endl;	
	cout << buf << endl;
	conn->send(buf, len+4 +1);
	
	/*
	char buf[10240] = {0};
	int len = snprintf(buf, 10240, msgfmt_Msgserver.c_str(), devType, dt.c_str(), msg.c_str(), app_ver.c_str(), devCategory, devSrc);
	string jsonStr(buf);
	CommServUtil::SendMsgUseStack sendTool;
	sendTool.sendJsonMessage(conn, SENDER_MSG_PUSH_MSG_CONTENT_CMD, true, jsonStr);
	*/
}


void sendMsgConcrete_iphone(const TcpConnectionPtr &conn, const std::string& dt, const std::string& app_ver)
{
	char buf[10240];
	*(uint16_t*)(buf+2) = htons(7503);
	int len = snprintf(buf+4, 10240, msgfmt_iphone.c_str(), dt.c_str(), msg.c_str(), app_ver.c_str());
	*(uint16_t*)(buf) = htons(4+len);

	cout << buf << endl;
	conn->send(buf, len+4);

	if((cnt % 10000) == 0)
	{
		cout << "send " << cnt << endl;
		//usleep(1000000);   //delay 0.2s
	}
	cnt++;
}

void sendMsgConcrete_ipad(const TcpConnectionPtr &conn, const std::string& dt, const std::string& app_ver)
{
	char buf[10240];
	*(uint16_t*)(buf+2) = htons(7503);
	int len = snprintf(buf+4, 10240, msgfmt_ipad.c_str(), dt.c_str(), msg.c_str(), app_ver.c_str());
	*(uint16_t*)(buf) = htons(4+len);
	cout << buf << endl;
	conn->send(buf, len+4);

	if((cnt % 10000) == 0)
	{
		cout << "send " << cnt << endl;
		//usleep(1000000);   //delay 0.2s
	}
	cnt++;
}

void sendMsgToSender(const TcpConnectionPtr &conn, const std::string& dt)
{
	char buf[10240] = {0};

	*(uint16_t*)(buf+2) = htons(ACCESS_SENDER_ANALYSE_SIG_MSG_CMD);
	int len = snprintf(buf+4, 10240, msgfmt_Senderserver.c_str(), dt.c_str(), msg.c_str());
	*(uint16_t*)(buf) = htons(4+len +1);
	string jsonStr(buf);
	cout << "string size is:" << jsonStr.size() << " len is: " << len << endl;	
	cout << buf << endl;
	conn->send(buf, len+4 +1);
}

void sendMsg(const TcpConnectionPtr &conn)
{

		if(conn->connected())
		{

			string dt2 = "ba698982ded309c3f39895949df03888081d62482cf8796241ce5ecab41525f4";  //澶х殑娴嬭瘯iphone

			char devtoken[100] = {0};
			int devType = 1;
			int appId = 8;
			int sessIndex = 5;
			//snprintf(devtoken, sizeof(devtoken), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa%02d%04d%06d", devType, appId, sessIndex);

			devType = 4;
			//snprintf(devtoken, sizeof(devtoken), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa%02d%04d%06d", devType, appId, sessIndex);
			//sendMsgConcrete_ipad(conn,devtoken);
			testRepaidJson();

			strcpy(devtoken, "xuwei");
			//sendMsgConcrete_iphone(conn, devtoken, "5.4.2");

			devType = 2;//and
			//sendMsgToMsgserver(conn, devtoken, "5.4.1", 0, 0,devType);
			sendMsgToSender(conn, devtoken);

		}
	//}
}

void onConnection(const TcpConnectionPtr &conn)
{

    if(conn->connected())
    {
     cout << "connected" << endl;
     cout <<  msgfmt_iphone << endl;
     //loop.runEvery(0.000001,boost::bind(&sendMsg, conn));
	 //loop.runEvery(0.0002,boost::bind(&sendMsg, conn));

	 //loop.runEvery(0.0002,boost::bind(&sendMsg, conn));
     loop.runInLoop(boost::bind(&sendMsg, conn));
	 //loop.runEvery(0.09, boost::bind(&sendMsg1, conn));
     //sendMsg(conn);
    }
    else
    {
        cout << "not connected." << endl;
    }
}



int main(int argc, char* argv[])
{
	
    ifstream in("dt.txt");
    string s;
    while(in >> s)
    {
        vsdt.push_back(s);
    }
    cout << "there is " << s.size() << " devtokens" << endl;
    sleep(2);

	//connect to msgserver
    //TcpClient clt(&loop, InetAddress("192.168.144.71", /*8907*/5500), "");
	
	//connect to thirdpart
	//TcpClient clt(&loop, InetAddress("192.168.144.61", /*8907*/8903), "");

	//connect to sender
	TcpClient clt(&loop, InetAddress("192.168.144.71", /*8907*/6500), "");

    clt.setConnectionCallback(boost::bind(&onConnection, _1));
    clt.connect();
	// start
    loop.loop();
}



