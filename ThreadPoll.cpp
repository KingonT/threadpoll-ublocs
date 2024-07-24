#include "ThreadPoll.h"

Thread::Thread(Thread::threadHander  func)
	:func_(func)
	,threadId_(generateId_++)
{}

Thread::~Thread() {}

void Thread::start()
{
	std::thread	thr(func_ , threadId_);
	thr.detach();
}
int Thread::generateId_ = 0;

int Thread::getId() const
{
	return threadId_;
}

ThreadPool::ThreadPool()
	: initThreadSize_(4), taskNum_(0) , taskQueMaxThreshHold_(MAXtaskHode)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRuning_(false)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, threadMaxThreshHold_(MAXthrHode)
{}

ThreadPool::~ThreadPool()
{
	isPoolRuning_ = false;
	std::unique_lock<std::mutex>	lock(taskQueMtx_);
	QueEmpty_.notify_all();
	exitcond_.wait(lock, [&]()->bool {return threadUMap_.size() == 0; });
	std::cout << "pool exit finally! \n";
}

Result ThreadPool::subMitTask(std::shared_ptr<Task> sp)
{
	std::unique_lock<std::mutex>	lock(taskQueMtx_);

	if (!QueFull_.wait_for(lock, std::chrono::seconds(1)
		                       ,[&]()->bool{ return taskQue_.size() < taskQueMaxThreshHold_; }))
	{
		std::cout << "submitTask failed! \n";
		return Result(sp,false);
	}

	taskQue_.emplace(sp);
	taskNum_++;
	QueEmpty_.notify_all();

	if (poolMode_ == PoolMode::MODE_CACHED 
			&& curThreadSize_ < threadMaxThreshHold_
			&& taskNum_ > curThreadSize_)
	{
		std::unique_ptr<Thread>	 ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threadUMap_.emplace(threadId, std::move(ptr));
		threadUMap_[threadId]->start();
		std::cout << "creat new thread...." << "\n";
		curThreadSize_++;
		idleThreadSize_++;
	}
	return Result(sp, true);
}

void ThreadPool::setMode(PoolMode mode) { 
	if (isPoolRuning_)	return;			//如果正在运行则推出
	poolMode_ = mode; 
}

bool ThreadPool::checkRuningState() const
{
	return isPoolRuning_;
}

void ThreadPool::start(size_t initSize)
{
	isPoolRuning_ = true;
	initThreadSize_ = initSize;
	curThreadSize_  = initSize;

	for (int i = 0; i < initThreadSize_; i++)
	{
		std::unique_ptr<Thread>	ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threadUMap_.emplace(threadId,std::move(ptr));
	}

	for (int i = 0; i < initThreadSize_; i++)
	{
		threadUMap_[i]->start();
		idleThreadSize_++;
	}
	std::cout << "threadUmap.size:"<< threadUMap_.size() << "\n";
}




void ThreadPool::threadFunc(int thrid)
{
	while(isPoolRuning_)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();
		std::shared_ptr<Task> ptask;
		{
			std::unique_lock<std::mutex>	lock(taskQueMtx_);
			while (isPoolRuning_ &&  taskQue_.size() == 0)
			{	
				std::cout << std::this_thread::get_id() << "尝试获取任务\n";			
				//cached 模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该把多余线程
				//结束回收掉（超过initThreadSize_数量的线程要进行回收）
				//当前时间 -  上一次线程执行的时间 > 60s
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					// 每一秒返回一次   怎么区分：超时返回 ？ 还是有			
					if (std::cv_status::timeout == QueEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							threadUMap_.erase(thrid);
							idleThreadSize_--;
							curThreadSize_--;
							std::cout << std::this_thread::get_id() << "->exit\n";
							return;
						}
					}
				} 
				else
				{
					QueEmpty_.wait(lock);
				}			
			}
			if (!isPoolRuning_)		break;
			idleThreadSize_--;
			std::cout << std::this_thread::get_id() << "获取任务成功\n";
			ptask = taskQue_.front();
			taskQue_.pop();
			taskNum_--;
			QueFull_.notify_all();
			if (taskQue_.size() > 0)    QueEmpty_.notify_all();
		}
		if(ptask.get()!= nullptr)		ptask->exec();					
			
		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();
	}
	
	if (!isPoolRuning_) {
		threadUMap_.erase(thrid);
		exitcond_.notify_all();
		std::cout << "pool exit..\n";
		return;
	}
}


/////////////////// Task
Task::Task():res_(nullptr){}

void Task::exec()
{
	if(res_ != nullptr)
		res_->setval(run());
}

void Task::setResult(Result* res)
{
	res_ = res;
}


