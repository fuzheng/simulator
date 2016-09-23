#include "MsgInterface.h"
#include <errno.h>

MsgInterface::MsgInterface(void)
{
}


MsgInterface::~MsgInterface(void)
{
}

MsgInterface::MsgInterface(uint16_t cmd, string fileName)
{
	this->msgCmd = cmd;
	this->fileName = fileName;
	this->buffer = "";
	this->msg = "";
}

void MsgInterface::AddMsgHead()
{
	int msgLen = sizeof(BaseHead) + jsonMsg.size() + 1;
	BaseHead head(msgLen, this->msgCmd);
	char* buffer = (char*) malloc(msgLen);
	memset(buffer, 0, msgLen);
	char* tmp = buffer;
	memcpy(tmp, &head, sizeof(head));
	tmp += sizeof(head);
	memcpy(tmp, jsonMsg.c_str(), jsonMsg.size() +1);
	this->buffer = buffer;
	free(buffer);
}

bool MsgInterface::ReadStringFromFile()
{
	struct stat buf;
	LOG_DEBUG << "file name is :" << this->fileName.c_str();
	int fd = open(this->fileName.c_str(),O_RDONLY);
	fstat(fd, &buf);
	int errno_ = 0;
	if(fd == -1)
	{
		errno_ = errno;
		LOG_ERROR << "errno: " << errno_ ;
		return false;
	}
	char *fContent =reinterpret_cast<char *>( malloc(buf.st_size + 1));
	int rlen = read(fd, fContent, static_cast<int>(buf.st_size));
	fContent[buf.st_size] = '\0';
	LOG_DEBUG << "rlen:" << rlen << " buf.st_size" << buf.st_size;
	errno_ = errno;
	LOG_DEBUG << "errno:" << errno_ << "errInfo:" << muduo::strerror_tl(errno_);
	if(rlen != buf.st_size)
	{
		LOG_ERROR << "the file data isn't entire";
		close(fd);
		free(fContent);
		return false;
	}
	
	msg = fContent;
	LOG_DEBUG << "data.file is: " << msg;
	free(fContent);
	return true;
}

bool MsgInterface::PackMsgFromString()
{
	//step1: read json string from file
	
	if(!ReadStringFromFile())
	{
		LOG_ERROR << "Read json msg from string error";
		return false;
	}
	//step2: unpack msg from json file
	if(!UnpackMsgFromString())
	{
		LOG_ERROR << "parse json string error";
		return false;
	}
	//step3: pack stuct to json string
	if (!PackMsgtoString())
	{
		LOG_ERROR << "write json to string error";
		return false;
	}
	//step4: add msg head to buffer
	AddMsgHead();
}

bool SenderMsg::UnpackMsgFromString()
{
	LOG_DEBUG << "parse data size is:" << msg.size() << " data is: " << msg;
	rapidjson::Document document;

	if (document.Parse(msg.c_str()).HasParseError())
	{
		LOG_ERROR << "parseSenderMsgPushMsgContentCmd fail, GetParseError = " << document.GetParseError();
		return false;
	}
	if (document.IsObject())
	{
		rapidjson::Value::MemberIterator iter;

		iter = document.FindMember("APPID");
		if (iter != document.MemberEnd())
		{
			pushMsg.appID = iter->value.GetUint();
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
			pushMsg.devType = iter->value.GetUint();
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
			pushMsg.msgType = iter->value.GetUint();
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
			pushMsg.msgID = iter->value.GetUint64();
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
			pushMsg.expireTime = iter->value.GetUint64();
		}
		else 
		{
			LOG_WARN << "EXPIRETIME is absent";
			////free(data);
			return false;
		}

		pushMsg.sendCnt = 0;

		iter = document.FindMember("DEVTOKEN");
		if (iter != document.MemberEnd())
		{
			strcpy(pushMsg.devToken, iter->value.GetString());
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
			strcpy(pushMsg.msgSeq, iter->value.GetString());
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
				strcpy(pushMsg.msg, iter->value.GetString()); 
			}
			else
			{
				LOG_WARN << "the size of msg content is over limit: " << msg;
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
			pushMsg.devCategory = iter->value.GetInt();
		}
		else
		{
			LOG_WARN << "DEV_CATEGORY is absend";
			return false;
		}

		iter = document.FindMember("DEV_SRC");
		if (iter != document.MemberEnd())
		{
			pushMsg.devSrc = iter->value.GetInt();
		}
		else
		{
			LOG_WARN << "DEV_SRC is absend";
			return false;
		}
		iter = document.FindMember("SENDCNT");
		if (iter != document.MemberEnd())
		{
			pushMsg.sendCnt = iter->value.GetUint();
		}

		iter = document.FindMember("BIZ_TYPE");
		if (iter != document.MemberEnd())
		{
			pushMsg.bizType= iter->value.GetInt();
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
				strcpy(pushMsg.planId, iter->value.GetString()); 
			}
			else
			{
				LOG_WARN << "the size of planid is over limit: " << msg;
				return false;
			}

		}
		else 
		{
			LOG_WARN << "PLANID is absent";
			return false;
		}

		//add by fuzheng 2016Äê9ÔÂ19ÈÕ, transforming appversion for ios badge
		iter = document.FindMember("APP_VER");
		if (iter != document.MemberEnd())
		{
			strcpy(pushMsg.appVer, iter->value.GetString());
		}
		else
		{
			LOG_WARN << "APP_VER is absent";
			////free(data);
			return false;
		}
	}
	//free(data);

	LOG_DEBUG << pushMsg.ToString();
	return true;
}


bool SenderMsg::PackMsgtoString()
{

	try
	{
		/*
		rapidjson::StringBuffer echo;
		rapidjson::Writer<rapidjson::StringBuffer> echowriter(echo);
		echowriter.StartObject();
		echowriter.Key("TEST1");
		echowriter.String("hello");
		echowriter.Key("TEST2");
		echowriter.Uint(3);
		echowriter.EndObject();
		*/
		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);       
		writer.StartObject();
		writer.Key("APPID");
		writer.Uint(pushMsg.appID);
		writer.Key("DEVTYPE");
		writer.Uint(pushMsg.devType);
		writer.Key("MSGTYPE");
		writer.Uint(pushMsg.msgType);
		writer.Key("MSGID");
		writer.Uint(pushMsg.msgID);
		writer.Key("EXPIRETIME");
		writer.Uint(pushMsg.expireTime);
		writer.Key("DEVTOKEN");
		writer.String(pushMsg.devToken);
		writer.Key("MSGSEQ");
		writer.String(pushMsg.msgSeq);
		writer.Key("MSG");
		writer.String(pushMsg.msg);
		writer.Key("DEV_CATEGORY");
		writer.Uint(pushMsg.devCategory);
		writer.Key("DEV_SRC");
		writer.Uint(pushMsg.devSrc);
		writer.Key("SENDCNT");
		writer.Uint(pushMsg.sendCnt);
		writer.Key("BIZ_TYPE");
		writer.Uint(pushMsg.bizType);
		writer.Key("PLANID");
		writer.String(pushMsg.planId);
		writer.Key("APP_VER");
		writer.String(pushMsg.appVer);
		/*
		writer.Key("EHCO");
		writer.String(echo.GetString());
		*/
		writer.EndObject();



		jsonMsg= s.GetString();
	}
	catch(...)
	{
		LOG_ERROR << "make json data failed";
		return false;
	}
	 LOG_DEBUG << "transform json is: " << jsonMsg << " [jsonsize is:]" << jsonMsg.size();
	return true;
}


int main(int argc, char* argv[])
{

	LOG_ERROR << muduo::Logger::logLevel()  << " DEBUG is: " << muduo::Logger::DEBUG;
	muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
	MsgInterface* msgSender = new SenderMsg(100, "send.file");
	msgSender->PackMsgFromString();
	string msg = msgSender->GetMsg();
}