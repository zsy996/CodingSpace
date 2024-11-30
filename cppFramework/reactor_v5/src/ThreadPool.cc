#include "ThreadPool.h"
#include <iostream>

using std::endl;
using std::cout;

ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)
, _queSize(queSize)
, _taskQue(_queSize)
, _isExit(false)
{

}

ThreadPool::~ThreadPool()
{

}

//线程池的启动与停止（创建线程与回收线程）
void ThreadPool::start()
{
    //创建线程对象,并且存放在容器中
    for(size_t idx = 0; idx != _threadNum; ++idx)
    {
        _threads.push_back(thread(&ThreadPool::doTask, this));
    }
}

void ThreadPool::stop()
{
    //保证任务执行完，也就是任务队列为空
    while(!_taskQue.empty())
    {
        //为了让出cpu的控制权，也就是防止cpu空转，那么主线程睡眠
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    //线程池要退出了，标志位需要修改
    _isExit = true;

    //唤醒所有等待在非空条件变量上的线程（唤醒所有工作线程）
    /* _notEmpty.notify_all(); */
    _taskQue.wakeup();

    //回收所有的子线程
    for(auto &th : _threads)
    {
        th.join();
    }
}

//添加任务与获取任务
void ThreadPool::addTask(Task &&task)
{
    if(task)
    {
        _taskQue.push(std::move(task));
    }
}

Task ThreadPool::getTask()
{
    return _taskQue.pop();
}

//线程池交给工作线程执行的任务（线程入口函数）
void ThreadPool::doTask()
{
    while(!_isExit)
    {
        //获取任务
        Task taskcb = getTask();
        if(taskcb)
        {
            //执行任务
            /* ptask->process();//多态 */
            taskcb();//执行回调
        }
        else
        {
            cout << "nullptr == ptask" << endl;
        }
    }
}
