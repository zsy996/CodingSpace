#include "TcpConnection.h"
#include "EventLoop.h"
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::ostringstream;

TcpConnection::TcpConnection(int fd, EventLoop *loop)
: _loop(loop)
, _sockIO(fd)
, _sock(fd)
, _localAddr(getLocalAddr())
, _peerAddr(getPeerAddr())
{

}

TcpConnection::~TcpConnection()
{

}

void TcpConnection::send(const string &msg)
{
    _sockIO.writen(msg.c_str(), msg.size());
}

//这个函数要怎么写，到底要实现什么?
//即使将处理好之后的msg发送个Reactor/EventLoop,但是EventLoop
//也没有发送数据的能力，所以我们需要将发送数据的连接对象
//TcpConnection以及msg以及发送数据的函数send都发给EventLoop
//
//“任务”：此处的任务其实就是线程池处理好之后的msg，以及
//发送数据的连接对象TcpConnection，也就是隐含的this指针
//以及发送数据能力的函数send
void TcpConnection::sendInLoop(const string &msg)
{
    if(_loop)
    {
    /* TcpConnection send msg; */
    /* function<void()> f = bind(&TcpConnection::send, this, msg); */
    /* _loop->runInLoop(f); */
        //void runInLoop(function<void()> &&);
        _loop->runInLoop(bind(&TcpConnection::send, this, msg));
    }
}
string TcpConnection::receive()
{
    char buff[65535] = {0};
    _sockIO.readLine(buff, sizeof(buff));

    return string(buff);
}

//判断客户端有没有断开
bool TcpConnection::isClosed() const
{
    char buf[10] = {0};
    int ret = ::recv(_sock.fd(), buf, sizeof(buf), MSG_PEEK);
    return (0 == ret);
}

string TcpConnection::toString()
{
    ostringstream oss;
    oss << _localAddr.ip() << ":"
        << _localAddr.port() << "---->"
        << _peerAddr.ip() << ":"
        << _peerAddr.port();

    return oss.str();
}

//获取本端的网络地址信息
InetAddress TcpConnection::getLocalAddr()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr );
    //获取本端地址的函数getsockname
    int ret = getsockname(_sock.fd(), (struct sockaddr *)&addr, &len);
    if(-1 == ret)
    {
        perror("getsockname");
    }

    return InetAddress(addr);
}

//获取对端的网络地址信息
InetAddress TcpConnection::getPeerAddr()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr );
    //获取对端地址的函数getpeername
    int ret = getpeername(_sock.fd(), (struct sockaddr *)&addr, &len);
    if(-1 == ret)
    {
        perror("getpeername");
    }

    return InetAddress(addr);
}

//注册三个回调函数
void TcpConnection::setNewConnectionCallback(const TcpConnectionCallback &cb)
{
    _onNewConnection = cb;
}

void TcpConnection::setMessageCallback(const TcpConnectionCallback &cb)
{
    _onMessage = cb;
}

void TcpConnection::setCloseCallback(const TcpConnectionCallback &cb)
{
    _onClose = cb;
}

//三个回调的执行
void TcpConnection::handleNewConnectionCallback()
{
    if(_onNewConnection)
    {
        /* _onNewConnection(shared_ptr<TcpConnection>(this)); */
        _onNewConnection(shared_from_this());
    }
    else
    {
        cout << "_onNewConnection == nullptr" << endl;
    }
}

void TcpConnection::handleMessageCallback()
{
    if(_onMessage)
    {
        _onMessage(shared_from_this());
    }
    else
    {
        cout << "_onMessage== nullptr" << endl;
    }
}

void TcpConnection::handleCloseCallback()
{
    if(_onClose)
    {
        _onClose(shared_from_this());
    }
    else
    {
        cout << "_onClose == nullptr" << endl;
    }
}
