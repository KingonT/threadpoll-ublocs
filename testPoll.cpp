#include "ThreadPoll.h"

class Mytask :public Task
{
public:
	Mytask(int begin , int end) :begin_(begin), end_(end){}
	~Mytask() = default;
	Any run()
	{
		/*'
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::cout << "tid:" << std::this_thread::get_id() << "end\n";*/
		int sum = 0;
		for (int i = begin_; i <= end_; i++)
		{
			sum = sum + i;
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
		return sum;
	}
private:
	int begin_;
	int end_;
};



int main()
{
	{
		ThreadPool  tPool;
		tPool.setMode(PoolMode::MODE_CACHED);
		tPool.start(3);

		Result re1 = tPool.subMitTask(std::make_shared<Mytask>(1, 1000));
		Result re2 = tPool.subMitTask(std::make_shared<Mytask>(1001, 2000));
		Result re3 = tPool.subMitTask(std::make_shared<Mytask>(2001, 3000));

		Result re4 = tPool.subMitTask(std::make_shared<Mytask>(2001, 3000));
		Result re5 = tPool.subMitTask(std::make_shared<Mytask>(2001, 3000));

		int n1 = re1.get().cast<int>();
		int n2 = re2.get().cast<int>();
		int n3 = re3.get().cast<int>();
		int n4 = re4.get().cast<int>();
		int n5 = re5.get().cast<int>();

		std::cout << "main thread strat " << "\n";

		int s = 0;
		for (int i = 1; i <= 3000; i++)
		{
			s += i;
		}

		std::cout << "s resutl is:" << s << "all result is :" << n1 + n2 + n3 << std::endl;
	}

	while (1);
	return 0;
}