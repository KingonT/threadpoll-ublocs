### c++thread Pool Base on C++17

### Project description:

- 基于可变参模板编程和引用折叠原理，实现线程池submitTask接口，支持任意任务函数和任意参数
的传递
- 使用future类型定制submitTask提交任务的返回值
- 使用map和queue容器管理线程对象和任务
- 基于条件变量condition_variable和互斥锁mutex实现任务提交线程和任务执行线程间的通信机制
- 支持fixed和cached模式的线程池定制

>fixed模式:
>线程池里面的线程个数是固定不变的，一般是ThreadPool创建时根据当前机器的CPU核心数量进行指定。

>cached模式线程池:
>线程池里面的线程个数是可动态增长的，根据任务的数量动态的增加线程的数量，但是会设置一个线程数量的阈值，任务处理完成，如果动态增长的线程空闲了60s还没有处理其它任务，那么关闭线程，保持池中最初数量的线程即可。

### Problems encountered when doing projects:
- 在ThreadPool的资源回收，等待线程池所有线程退出时，发生死锁问题，导致进程无法退出
- 在windows平台下运行良好的线程池，在linux平台下运行发生死锁问题，平台运行结果有差异化

### Analysis and positioning problems:
主要通过gdb attach到正在运行的进程，通过info threads，thread tid，bt等命令查看各个线程的调用
堆栈信息，结合项目代码，定位到发生死锁的代码片段，分析死锁问题发生的原因.