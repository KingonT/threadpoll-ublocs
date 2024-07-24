#ifndef THREADPOLL_H
#define THREADPOLL_H

#include	<iostream>
#include	<vector>
#include	<queue>
#include	<memory>
#include	<condition_variable>
#include	<functional>
#include	<thread>
#include	<time.h>
#include	<unordered_map>
#include 	<atomic>
const size_t	MAXtaskHode = 1024;
const size_t	MAXthrHode = 32;
const size_t	THREAD_MAX_IDLE_TIME = 10;

enum class PoolMode
{
	MODE_FIXED,
	MODE_CACHED,
};


class Semaphore
{
public:
	Semaphore(uint32_t num = 0) :resLimit_(num) {}
	~Semaphore() = default;
	

	void post() { 
		std::unique_lock<std::mutex>  lock(mtx_); 
		resLimit_++;
		cond_.notify_all();
	}
	void wait() { 
		std::unique_lock<std::mutex>  lock(mtx_);
		cond_.wait(lock,[&]()->bool { return resLimit_ > 0; });
		resLimit_--;
	}

private:
	std::mutex					mtx_;
	std::condition_variable		cond_;
	uint32_t					resLimit_;
};
class Any
{
public:
	Any() = default;
	~Any() = default;

	Any(const Any&) = delete;
	Any& operator = (const Any&) = delete;

	Any(Any&&) = default;
	Any& operator = (Any&&) = default;
	template<typename T> Any(T data) :uQBase_(new Drive<T>(data)) { }

	template<typename T> T  cast()
	{
		Drive<T>* bp = dynamic_cast<Drive<T>*>(uQBase_.get());
		if (bp == nullptr)	throw "type  is unmatch";
		return bp->data_;
	}

private:
	class Base
	{
	public:
		virtual ~Base() {}
	};

	template<typename T> class Drive :public Base
	{
	public:
		Drive(T data) :data_(data) {}
		T	data_;
	};

private:
	std::unique_ptr<Base>   uQBase_;
};

class Result;

class Task
{
public:
	Task();
	~Task() = default;

	void setResult(Result* res);
	void exec();
	virtual Any run() = 0;
private:
	Result* res_;
};

class Result
{
public:
	Result() = default;
	~Result() = default;

	/*Result(Any any) :any_(std::move(any)) {}*/
	Result(std::shared_ptr<Task> task, bool isvaild = true)
		:sptask_(task), isVaild_(isvaild) 
	{ 
		sptask_->setResult(this) ;
	}

	// setval , 获取任务执行完的返回值
	void setval(Any any) {
		this->any_ = std::move(any);
		sem_.post();
	}

	// get , 用户调用这个方法获取task的值
	Any get(){
		if (!isVaild_) return "";
		sem_.wait();
		return std::move(any_);
	}

private:
	Any						any_;
	Semaphore				sem_;
	std::shared_ptr<Task>	sptask_;
	std::atomic_bool		isVaild_;
};



class Thread
{
	typedef std::function<void(int)> threadHander;
	
public:
	Thread(threadHander func);
	~Thread();

	int getId() const;
	void start();
private:
	threadHander	func_;
	static	int		generateId_;
	int				threadId_;
};



class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	//void setThreadSize(size_t size = 4);
	//void setTaskMaxHode(size_t size = MAXtaskHode);
	void setMode(PoolMode mode = PoolMode::MODE_FIXED);

	Result subMitTask(std::shared_ptr<Task> sp);
	void start(size_t initSize = std::thread::hardware_concurrency());

	void threadFunc(int );

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	bool checkRuningState() const;

private:
	//std::vector<std::unique_ptr<Thread>>  Vthreads_;
	std::unordered_map<int, std::unique_ptr<Thread>>	 threadUMap_;

	size_t								  initThreadSize_;
	size_t								  threadMaxThreshHold_;
	std::atomic_uint					  idleThreadSize_;
	std::atomic_uint					  curThreadSize_;

	std::queue<std::shared_ptr<Task>>     taskQue_;
	std::atomic_uint					  taskNum_;				
	size_t							      taskQueMaxThreshHold_;
	std::mutex							  taskQueMtx_;
	std::condition_variable				  QueFull_;
	std::condition_variable				  QueEmpty_;
	PoolMode							  poolMode_;
	std::atomic_bool					  isPoolRuning_;
	std::condition_variable				  exitcond_;
};



#endif
