#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

using std::vector;
using std::map;
using std::shared_ptr;
using std::function;
using std::mutex;
using std::lock_guard;

class Acceptor;
class TcpConnection;

using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr &)>;
using Functor = function<void()>;

class EventLoop
{
public:
    EventLoop(Acceptor &acceptor);
    ~EventLoop();

    //循环与否
    void loop();
    void unloop();
private:

    //里面封装了epoll_wait
    void waitEpollFd();

    //处理新的连接请求
    void handleNewConnection();

    //处理老的连接上的数据收发
    void handleMessage(int fd);

    //封装epoll_create
    int createEpollFd();

    //将文件描述符放在红黑树上进行监听
    void addEpollReadFd(int fd);
    //
    //将文件描述符从红黑树上取消监听
    void delEpollReadFd(int fd);

public:
    //注册三个回调函数
    void setNewConnectionCallback(TcpConnectionCallback &&cb);
    void setMessageCallback(TcpConnectionCallback &&cb);
    void setCloseCallback(TcpConnectionCallback &&cb);

public:
    //创建用于通知的文件描述符
    int createEventFd();
    //封装read与write
    void handleRead();
    void wakeup();
    //遍历vector，执行所有的任务
    void doPendingFunctors();
    void runInLoop(Functor &&cb);


private:
    int _epfd;//通过epoll_create创建的文件描述符
    vector<struct epoll_event> _evtList;//存放就绪的文件描述符的数据结构
    bool _isLooping;//标识循环是否进行的标志
    Acceptor &_acceptor;//使用该对象的引用调用accept函数
    map<int, TcpConnectionPtr> _conns;//存放文件描述符与TcpConnection对象的键值对

    TcpConnectionCallback _onNewConnection;//连接建立
    TcpConnectionCallback _onMessage;//消息到达（文件描述符可读）
    TcpConnectionCallback _onClose;//连接断开
                                   
    int _evtfd;//用于通知的文件描述符
    vector<Functor> _pendings;//存放需要执行的任务
    mutex _mutex;//互斥锁

};

#endif
