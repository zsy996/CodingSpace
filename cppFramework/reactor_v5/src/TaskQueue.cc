#include "TaskQueue.h"

TaskQueue::TaskQueue(int capa)
: _capacity(capa)
, _que()
, _mutex()
, _notFull()
, _notEmpty()
, _flag(true)
{

}

TaskQueue::~TaskQueue()
{

}

//生产数据与消费数据
void TaskQueue::push(ElemType &&task)
{
    unique_lock<mutex> ul(_mutex);
    while(full())
    {
        _notFull.wait(ul);
    }
    _que.push(std::move(task));
    _notEmpty.notify_one();
}

ElemType TaskQueue::pop()
{
    unique_lock<mutex> ul(_mutex);

    while(empty() && _flag)
    {
        _notEmpty.wait(ul);//睡眠
    }

    if(_flag)
    {
        ElemType tmp = _que.front();
        _que.pop();
        _notFull.notify_one();

        return tmp;
    }
    else
    {
        return nullptr;
    }
}

//判空与判满
bool TaskQueue::empty() const
{
    return 0 == _que.size(); 
}

bool TaskQueue::full() const
{
    return _capacity == _que.size();
}

void TaskQueue::wakeup()
{
    _flag = false;
    _notEmpty.notify_all();
}
