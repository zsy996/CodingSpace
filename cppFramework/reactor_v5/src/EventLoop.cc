#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <iostream>

using std::cout;
using std::endl;
using std::cerr;

EventLoop::EventLoop(Acceptor &acceptor)
: _epfd(createEpollFd())
, _evtList(1024)
, _isLooping(false)
, _acceptor(acceptor)
, _evtfd(createEventFd())
, _pendings()
, _mutex()
{
    //监听listenfd
    int listenfd = _acceptor.fd();
    addEpollReadFd(listenfd);
    //还需要监听用于通知的文件描述符
    addEpollReadFd(_evtfd);
}

EventLoop::~EventLoop()
{
    close(_epfd);
    close(_evtfd);
}

//循环与否
void EventLoop::loop()
{
    _isLooping = true;
    while(_isLooping)
    {
        waitEpollFd();
    }
}

void EventLoop::unloop()
{
    _isLooping = false;
}

//里面封装了epoll_wait
void EventLoop::waitEpollFd()
{
    int nready = 0;
    do
    {
        nready = ::epoll_wait(_epfd, &*_evtList.begin(), _evtList.size(), 3000);
    }while(-1 == nready && errno == EINTR);

    if(-1 == nready)
    {
        cerr << "-1 == nready" << endl;
        return;
    }
    else if(0 == nready)
    {
        cout << ">>epoll_wait timeout!!!" << endl;
    }
    else
    {
        //手动扩容
        if(nready == (int)_evtList.size())
        {
            _evtList.resize(2 * nready);
        }

        for(int idx = 0; idx < nready; ++idx)
        {
            //是不是listenfd就绪
            int listenfd = _acceptor.fd();
            int fd = _evtList[idx].data.fd;
            if(listenfd == fd)//有新的连接请求
            {
                //处理新的连接请求
                handleNewConnection();
            }
            else if(_evtfd == fd)//用于通知的文件描述符就绪
            {
                handleRead();
                //遍历vector,执行所有的任务
                doPendingFunctors();
            }
            else//老的连接上有数据发过来了
            {
                handleMessage(fd);
            }
        }
    }
}

//处理新的连接请求
void EventLoop::handleNewConnection()
{
    //connfd正常，那么就表明三次握手已经建立成功了
    int connfd = _acceptor.accept();
    if(connfd < 0)
    {
        perror("handleNewConnection");
        return;
    }

    //将connfd放在红黑树上进行监听
    addEpollReadFd(connfd);

    //创建TcpConnection连接的对象,并且将其存放在map中
    TcpConnectionPtr con(new TcpConnection(connfd, this));

    //将三个半事件注册给TcpConnection的对象
    con->setNewConnectionCallback(_onNewConnection);//连接建立
    con->setMessageCallback(_onMessage);//消息到达（文件描述符可读）
    con->setCloseCallback(_onClose);//连接断开

    _conns.insert({connfd, con});

    //连接既然己经建立的，就可以执行连接建立的事件
    con->handleNewConnectionCallback();
}

//处理老的连接上的数据收发
void EventLoop::handleMessage(int fd)
{
    auto it = _conns.find(fd);
    if(it != _conns.end())
    {
        //如何判断有没有断开呢
        bool flag = it->second->isClosed();//判断是不是返回结果为0
        if(flag)
        {
            //处理连接断开的事件
            it->second->handleCloseCallback();
            //将文件描述符从红黑树删除
            delEpollReadFd(fd);
            //还需要将文件描述符与连接的对象从map删除
            /* _conns.erase(fd);//ok */
            _conns.erase(it);
        }
        else
        {
            //处理消息到达（文件描述符可读）
            it->second->handleMessageCallback();
        }
    }
    else
    {
        cout << "该连接不存在 " << endl;
        return;
    }
}

//封装epoll_create
int EventLoop::createEpollFd()
{
    int fd = ::epoll_create(1);
    if(fd < 0)
    {
        perror("createEpollFd");
        return -1;
    }
    return fd;
}

//将文件描述符放在红黑树上进行监听
void EventLoop::addEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.events = EPOLLIN;
    evt.data.fd = fd;

    int ret = ::epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt);
    if(ret < 0)
    {
        perror("addEpollReadFd");
        return;
    }
}

//将文件描述符从红黑树上取消监听
void EventLoop::delEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.events = EPOLLIN;
    evt.data.fd = fd;

    int ret = ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &evt);
    if(ret < 0)
    {
        perror("delEpollReadFd");
        return;
    }
}

//注册三个回调函数
void EventLoop::setNewConnectionCallback(TcpConnectionCallback &&cb)
{
    _onNewConnection = std::move(cb);
}

void EventLoop::setMessageCallback(TcpConnectionCallback &&cb)
{
    _onMessage = std::move(cb);
}

void EventLoop::setCloseCallback(TcpConnectionCallback &&cb)
{
    _onClose = std::move(cb);
}
//创建用于通知的文件描述符
int EventLoop::createEventFd()
{
    int fd = eventfd(10, 0);
    if(fd < 0)
    {
        perror("createEventFd");
        return -1;
    }

    return fd;
}

//封装read与write
void EventLoop::handleRead()
{
    uint64_t one = 1;
    int ret = read(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        perror("handleRead");
        return;
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    int ret = write(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        perror("wakeup");
        return;
    }

}

//还可以不可以优化呢？
//遍历vector，执行所有的任务
void EventLoop::doPendingFunctors()
{
    vector<Functor> tmp;
    {
        lock_guard<mutex> lg(_mutex);
        tmp.swap(_pendings);
    }

    //此处的"任务"就是TcpConnection中sendInLoop
    //函数传递进来给EventLoop的
    for(auto &cb : tmp)
    {
        cb();//执行所有的任务
    }
}

//该函数实现完毕了没有？
void EventLoop::runInLoop(Functor &&cb)
{
    //缩小锁的范围（降低锁的粒度）
    {
        lock_guard<mutex> lg(_mutex);
        _pendings.push_back(std::move(cb));
    }

    //通知
    wakeup();
}
