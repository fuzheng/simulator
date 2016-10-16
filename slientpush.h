#include "./thirdpartserver/sendContain.h"

class SlientPush
{
	SenderWrapper a;

};



class SlientPushServer {

private:
	SenderServer* GetSenderServer()
	{

	}

public:
	//msg handler
	void JobServerPutMsgCmd()
	{
		//解析jobserver传入的消息
		TagTask msgTask;
		if(ParseMsg() == false)
		{
			LOG_WARN << "parse msg from jobserver error, discard it!";
			return;
		}
		//解析成功，调用senderserver将tagtask结构push到tagtasklist中
		GetSenderServer()->PushTagTask(msgTask);
	}


	bool ParseMsg(const char* msg, const int size, TagTask& parsedTask)
	{
		//
	}


}

class SenderServer
{
typedef std::vector<TagTask> TagTaskList;
typedef std::vector<TagTask>::iterator TagTaskListIter;


//该class应该放在task handle类中，每发出一个消息，计数并判断
//当前只是单线程使用，不用考虑锁等方面，提高效率
class TaskCount
{
private:
	int taskSum;//限制发出多少个消息
	int taskAlready;//对于某个msgid/task,已经发出了多少了消息
public:
	TaskCount(int sum)
	{
		taskSum = sum;
	}

	void Incr()
	{
		taskAlready++;
	}

	bool IsSendFinish()
	{

		return taskSum == 0 ? false : (taskAlready >= taskSum);
	}

}
???
class SenderThrooling
{}

private：
	TagTaskList taskList;

	//用于处理tagtasklist的eventloopthread
	EventLoopThread* handleTagTaskThread;
	//锁
	MutexLock taskListLock;

public:
	//主线程中写入
	void PushTagTask(const TagTask& task)
	{
		//将tag结构push到list中
		MutexLockGuard(taskListLock);
		taskList.push_back(task);
	}

	SenderServer()
	{

	}
	void Init()
	{
		//预先分配一些内存
		taskList.reserve(10);
		//初始化handleTagTaskThread
		handleTagTaskThread = new EventLoopThread(NULL, "Senderserver");

		//启动一个线程
		handleTagTaskThread.startLoop();
	}

	//runInloop做

	void HanldeTaskList()
	{
		TagTaskList tmpTaskList;
		{
			MutexLockGuard(taskListLock);
			if(taskList.size() != 0)
			{
				tmpTaskList.swap(taskList);
			}
		}

		for(TagTaskListIter iter = tmpTaskList.begin(); iter != tmpTaskList.end(); iter++)
		{

		}

	}
}


class ThirtPartServer
{

	
}