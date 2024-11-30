#include "EchoServer.h"
#include "TcpConnection.h"
#include <iostream>
#include <functional>

using std::cout;
using std::endl;
using std::bind;

MyTask::MyTask(const string &msg, const TcpConnectionPtr &con)
: _msg(msg)
, _con(con)
{

}

void MyTask::process()
{
    _con->sendInLoop(_msg);
}

EchoServer::EchoServer(size_t threadNum, size_t queSize
                       , const string &ip
                       , unsigned short port)
: _pool(threadNum, queSize)
, _server(ip, port)
{

}

EchoServer::~EchoServer()
{

}

//服务器的启动与停止
void EchoServer::start()
{
    _pool.start();

    using namespace std::placeholders;
    _server.setAllCallback(std::bind(&EchoServer::onNewConnection, this, _1)
                           , std::bind(&EchoServer::onMessage, this, _1)
                           , std::bind(&EchoServer::onClose, this, _1));
    _server.start();
}
void EchoServer::stop()
{
    _pool.stop();
    _server.stop();
}

//三个回调函数
void EchoServer::onNewConnection(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has connected!!!" << endl;
}

void EchoServer::onMessage(const TcpConnectionPtr &con)
{
    string msg = con->receive();
    cout << ">>recv msg from client: "  << msg << endl;

    //接收到客户端的数据之后，可以将数据做业务逻辑的处理
    //将从客户端接受的数据交给线程池进行处理
    MyTask task(msg, con);

    _pool.addTask(bind(&MyTask::process, task));

}

void EchoServer::onClose(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has closed!!!" << endl;
}
