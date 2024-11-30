#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "Acceptor.h"
#include "EventLoop.h"

class TcpServer
{
public:
    TcpServer(const string &ip, unsigned short port);
    ~TcpServer();

    //服务器的启停
    void start();
    void stop();

    //三个回调的设置
    void setAllCallback(TcpConnectionCallback &&cb1
                        , TcpConnectionCallback &&cb2
                        , TcpConnectionCallback &&cb3);

private:
    Acceptor _acceptor;
    EventLoop _loop;

};

#endif
